# cython: language_level=3
# python/mmuko_os/bios_firmware.pyx
# MMUKO-OS BIOS Firmware Interface — Cython Python bindings
# OBINexus Computing | Nnamdi Michael Okpala | 29 March 2026
#
# Exposes:
#   get_datetime()      → dict with year/month/day/hour/min/sec/century
#   SpinSystem          → cdef class wrapping SpinPair (step, snapshot, is_coherent)
#   MosaicMemoryView    → cdef class wrapping MosaicEntry[64] (encode, entries)
#   Phase1UIBridge      → combines SpinSystem + MosaicMemoryView, renders to stdout
#
# Build:  pip install -e .   (uses setup.py extension definition)
# Usage:
#   from mmuko_os.bios_firmware import get_datetime, SpinSystem, Phase1UIBridge

from .bios_firmware cimport (
    BIOSDateTime, SpinPair, SpinOperator, MosaicEntry,
    bios_datetime_now, bios_datetime_format,
    spin_pair_init, spin_pair_step, spin_valid_state,
    mosaic_encode, mosaic_color_for,
    uint8_t, uint32_t,
)
from .firmware cimport TrinaryState


# =========================================================================
# get_datetime — read host/BIOS RTC, return Python dict
# =========================================================================
def get_datetime():
    """Return the current date/time as a dict.

    Keys: century, year, month, day, hour, min, sec  (all int).
    Mirrors BIOS INT 0x1A layout; values are decimal (not BCD).
    """
    cdef BIOSDateTime dt = bios_datetime_now()
    return {
        'century': <int>dt.century,
        'year':    <int>dt.year,
        'month':   <int>dt.month,
        'day':     <int>dt.day,
        'hour':    <int>dt.hour,
        'min':     <int>dt.min,
        'sec':     <int>dt.sec,
    }


def format_datetime():
    """Return date/time as 'CCYY-MM-DD HH:MM:SS' string."""
    cdef BIOSDateTime dt = bios_datetime_now()
    cdef char buf[24]
    bios_datetime_format(&dt, buf, 24)
    return buf.decode('ascii')


# =========================================================================
# SpinSystem — wraps SpinPair, drives magnetic attraction simulation
# =========================================================================
cdef class SpinSystem:
    """Magnetic spin pair simulation (alpha=north, bravo=south)."""
    cdef SpinPair _pair

    def __cinit__(self):
        self._pair = spin_pair_init()

    def step(self):
        """Advance one attract step."""
        spin_pair_step(&self._pair)

    def reset(self):
        """Reset to initial state (alpha x=-10, bravo x=+10)."""
        self._pair = spin_pair_init()

    def snapshot(self):
        """Return current state as a dict."""
        return {
            'alpha_x':      <int>self._pair.alpha_magnet.position_x,
            'alpha_orient': <int>self._pair.alpha_magnet.orientation_x100,
            'alpha_elev':   <int>self._pair.alpha_magnet.elevation,
            'alpha_spin':   <int>self._pair.alpha_magnet.alpha,
            'bravo_x':      <int>self._pair.bravo_magnet.position_x,
            'bravo_orient': <int>self._pair.bravo_magnet.orientation_x100,
            'bravo_elev':   <int>self._pair.bravo_magnet.elevation,
            'bravo_spin':   <int>self._pair.bravo_magnet.alpha,
            'steps':        <int>self._pair.step_count,
            'work':         self._pair.work_done,
        }

    def is_coherent(self):
        """Check 2/3 + 2/3 + 1/4 coherence criterion (from transcript)."""
        return bool(spin_valid_state(&self._pair))

    @property
    def steps(self):
        return <int>self._pair.step_count

    @property
    def work_done(self):
        return self._pair.work_done


# =========================================================================
# MosaicMemoryView — wraps up to 64 MosaicEntry cells
# =========================================================================
cdef class MosaicMemoryView:
    """Color-coded hexadecimal memory model (mosaic from transcript)."""
    cdef MosaicEntry _entries[64]
    cdef int _count

    def __cinit__(self):
        self._count = 0

    def encode(self, addr_bytes, int data, int state_val):
        """Encode a 3-byte address into the mosaic.

        addr_bytes: sequence of 3 ints (e.g. [0xBC, 0x5F, 0xA9])
        data:       byte value (0-255)
        state_val:  TrinaryState as int (1=YES, 0=NO, -1=MAYBE, -2=MAYBE_NOT)
        """
        cdef uint8_t addr[3]
        if self._count >= 64:
            raise IndexError("mosaic memory full (64 entries max)")
        addr[0] = <uint8_t>(addr_bytes[0] & 0xFF)
        addr[1] = <uint8_t>(addr_bytes[1] & 0xFF)
        addr[2] = <uint8_t>(addr_bytes[2] & 0xFF)
        self._entries[self._count] = mosaic_encode(
            addr,
            <uint8_t>(data & 0xFF),
            <TrinaryState>state_val,
        )
        self._count += 1

    def entries(self):
        """Return list of dicts describing all encoded entries."""
        result = []
        for i in range(self._count):
            e = self._entries[i]
            result.append({
                'address':    [<int>e.address[0], <int>e.address[1], <int>e.address[2]],
                'color_code': <int>e.color_code,
                'data':       <int>e.data,
                'spin_state': <int>e.spin_state,
            })
        return result

    @property
    def count(self):
        return self._count


# =========================================================================
# Phase1UIBridge — Python-level Phase 1 BIOS firmware UI
# =========================================================================
cdef class Phase1UIBridge:
    """Python bridge to the Phase 1 BIOS Firmware UI.

    Combines SpinSystem + MosaicMemoryView, renders a text dashboard
    to stdout that mirrors the QEMU BIOS kernel display.

    Example::

        from mmuko_os.bios_firmware import Phase1UIBridge
        ui = Phase1UIBridge()
        for _ in range(20):
            ui.step()
        ui.render()
    """
    cdef SpinSystem      _spin
    cdef MosaicMemoryView _mosaic
    cdef int             _phase      # current NSIGII phase 1-6

    _PHASE_NAMES = [
        "", "NEED_STATE_INIT", "SAFETY_SCAN", "IDENTITY_CAL",
        "GOVERNANCE", "INTERNAL_PROBE", "INTEGRITY_VERIFY",
    ]
    _NSIGII = "NSIGII"

    def __cinit__(self):
        self._spin   = SpinSystem()
        self._mosaic = MosaicMemoryView()
        self._phase  = 1
        # Pre-populate 8 canonical mosaic entries
        _addrs = [
            [0xBC, 0x5F, 0xA9], [0x1E, 0x2F, 0x3C],
            [0xDE, 0xAD, 0xBE], [0xCA, 0xFE, 0x00],
            [0xFF, 0x00, 0x11], [0x7F, 0x80, 0x90],
            [0x42, 0x66, 0xAA], [0x0B, 0xA5, 0xE5],
        ]
        _states = [1, 1, 1, -1, -1, -1, 0, 0]  # YES×3, MAYBE×3, NO×2
        for i, (a, st) in enumerate(zip(_addrs, _states)):
            self._mosaic.encode(a, 0xA0 + i, st)

    def step(self):
        """Advance spin simulation one step."""
        self._spin.step()

    def calibrate(self):
        """Reset spin to initial state."""
        self._spin.reset()
        self._phase = 1

    def next_phase(self):
        """Advance NSIGII phase (1→2→…→6→1)."""
        self._phase = (self._phase % 6) + 1

    def render(self):
        """Print the Phase 1 UI dashboard to stdout."""
        dt_str = format_datetime()
        snap    = self._spin.snapshot()
        coher   = self._spin.is_coherent()
        outcome = "PASS" if (coher and self._phase > 1) else "HOLD"
        phase_name = self._PHASE_NAMES[self._phase] if self._phase <= 6 else "?"

        print("+====================================================+")
        print("|  OBINexus MMUKO-OS | Phase 1 BIOS Firmware UI      |")
        print(f"|  {dt_str:<48}|")
        print("+====================================================+\n")

        # NSIGII bar
        bar = " ".join(
            f"[{self._NSIGII[p-1]}:{p}{'*' if p == self._phase else ''}]"
            for p in range(1, 7)
        )
        print(f"NSIGII: {bar}")
        print(f"Phase : {self._phase}  ({phase_name})\n")

        # Spin state
        print("SPIN SIMULATION:")
        print(f"  Alpha (N/+1): x={snap['alpha_x']:<5}  orient={snap['alpha_orient']:>3}  "
              f"elev={snap['alpha_elev']:<6}  spin={snap['alpha_spin']}")
        print(f"  Bravo (S/-1): x={snap['bravo_x']:<5}  orient={snap['bravo_orient']:>3}  "
              f"elev={snap['bravo_elev']:<6}  spin={snap['bravo_spin']}")
        print(f"  Steps: {snap['steps']:<4}  Work: {snap['work']:<14.6f}  Coherent: {coher}\n")

        # Membrane outcome
        print(f"MEMBRANE OUTCOME: [{outcome}]\n")

        # Mosaic
        print("MOSAIC MEMORY:")
        _state_map = {1: "YES", 0: "NO", -1: "MAYBE", -2: "MAYBE_NOT"}
        for e in self._mosaic.entries():
            addr = e['address']
            st   = _state_map.get(e['spin_state'], "?")
            print(f"  #{addr[0]:02X}{addr[1]:02X}{addr[2]:02X}  "
                  f"[color={e['color_code']}]  state={st:<9}  data=0x{e['data']:02X}")

        print("\nCommands: step()  calibrate()  next_phase()  render()")

    def handle_input(self, key):
        """Handle a single keypress: 's'=step, 'c'=calibrate, 'n'=next_phase."""
        k = str(key).lower()
        if k == 's':
            self.step()
        elif k == 'c':
            self.calibrate()
        elif k == 'n':
            self.next_phase()

    @property
    def current_phase(self):
        return self._phase

    @property
    def is_coherent(self):
        return self._spin.is_coherent()


__all__ = [
    'get_datetime',
    'format_datetime',
    'SpinSystem',
    'MosaicMemoryView',
    'Phase1UIBridge',
]
