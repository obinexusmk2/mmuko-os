"""MMUKO-OS NSIGII Kernel -- persistent REPL shell.

The kernel boots through the NSIGII 6-phase sequence, then provides an
interactive command interface backed by in-memory C firmware structs.
"""

from __future__ import annotations

import cmd
import sys
from enum import IntEnum

from . import (
    PerspectiveMembraneHandle,
    MPDAHandle,
    TRINARY_VALUES,
    MEMBRANE_LABELS,
    BZY_STATE_LABELS,
    DISCRIMINANT_LABELS,
    CONSENSUS_LABELS,
    drift_radial,
    tripartite_summary,
    trinary_compose,
    trinary_resolve,
    enzyme_apply,
)

# ---------------------------------------------------------------------------
# Constants
# ---------------------------------------------------------------------------

PHASE_NAMES = {
    1: "NEED_STATE_INIT",
    2: "SAFETY_SCAN",
    3: "IDENTITY_CALIBRATION",
    4: "GOVERNANCE_CHECK",
    5: "INTERNAL_PROBE",
    6: "INTEGRITY_VERIFICATION",
}

ENZYME_NAMES = {0: "CREATE", 1: "DESTROY", 2: "BUILD", 3: "BREAK", 4: "RENEW", 5: "REPAIR"}

# Priority hierarchy from HERE AND NOW C&C pseudocode
PRIORITY_LABELS = {0: "BREATHING", 1: "LIVING", 2: "WORKING"}

# ---------------------------------------------------------------------------
# Trinary input helper
# ---------------------------------------------------------------------------

def _parse_trinary(value: str) -> int:
    key = value.strip().lower().replace("-", "_")
    if key in TRINARY_VALUES:
        return TRINARY_VALUES[key]
    try:
        v = int(value)
        if v in (1, 0, -1, -2):
            return v
    except ValueError:
        pass
    raise ValueError(f"Unknown trinary value: {value!r}  (use yes/no/maybe/maybe_not)")


# ---------------------------------------------------------------------------
# Kernel state -- persists in memory for the session
# ---------------------------------------------------------------------------

class KernelState:
    def __init__(self) -> None:
        self.membrane = PerspectiveMembraneHandle()
        self.mpda = MPDAHandle()
        self.boot_outcome: str = "hold"
        self.completed_phases: int = 0
        self.validation_flags: int = 0
        # Breathing pointer (HERE AND NOW C&C)
        self.breathing: str = "FLOWING"
        self.living: str = "ALIVE"
        self.working: str = "SUSPENDED"
        # RWX state machine (ON-THE-FLY C&C)
        self.read_count: int = 0
        self.write_count: int = 0
        self.execute_count: int = 0

    @property
    def booted(self) -> bool:
        return self.boot_outcome == "pass" and self.completed_phases == 6


# ---------------------------------------------------------------------------
# REPL kernel
# ---------------------------------------------------------------------------

class NSIGIIKernel(cmd.Cmd):
    intro = (
        "\n"
        "  MMUKO-OS v0.1.0 | NSIGII Heartfull Firmware\n"
        "  OBINexus Computing | Nnamdi Michael Okpala\n"
        "  Type 'help' for commands, 'quit' to exit.\n"
    )
    prompt = "mmuko> "

    def __init__(self, tier1: str = "yes", tier2: str = "yes") -> None:
        super().__init__()
        self.state = KernelState()
        self._init_tier1 = tier1
        self._init_tier2 = tier2

    # ---- boot sequence ----

    def preloop(self) -> None:
        self._run_boot(self._init_tier1, self._init_tier2)

    def _run_boot(self, tier1_str: str = "yes", tier2_str: str = "yes") -> None:
        tier1 = _parse_trinary(tier1_str)
        tier2 = _parse_trinary(tier2_str)

        print("=" * 60)
        print("  NSIGII 6-PHASE BOOT SEQUENCE")
        print("=" * 60)

        # Phase 1: Need-State Init
        outcome = self.state.membrane.calibrate({1: tier1, 2: tier2})
        snap = self.state.membrane.snapshot()
        if not snap["needs"]["tier1_satisfied"]:
            print(f"  [PHASE 1] {PHASE_NAMES[1]}: FAILED -- tier1 not satisfied")
            self.state.boot_outcome = "alert"
            self._boot_footer()
            return
        self.state.membrane.run_phase(1)
        self.state.completed_phases = 1
        self.state.validation_flags |= 0x01
        print(f"  [PHASE 1] {PHASE_NAMES[1]}: OK  (outcome={outcome})")

        # Phase 2: Safety Scan
        self.state.membrane.run_phase(2)
        self.state.completed_phases = 2
        self.state.validation_flags |= 0x02
        print(f"  [PHASE 2] {PHASE_NAMES[2]}: OK")

        # Phase 3: Identity Calibration
        self.state.membrane.run_phase(3)
        self.state.completed_phases = 3
        self.state.validation_flags |= 0x04
        print(f"  [PHASE 3] {PHASE_NAMES[3]}: OK")

        # Phase 4: Governance Check
        self.state.membrane.run_phase(4)
        self.state.completed_phases = 4
        self.state.validation_flags |= 0x08
        print(f"  [PHASE 4] {PHASE_NAMES[4]}: OK")

        # Phase 5: Internal Probe
        self.state.membrane.run_phase(5)
        self.state.completed_phases = 5
        self.state.validation_flags |= 0x10
        print(f"  [PHASE 5] {PHASE_NAMES[5]}: OK")

        # Phase 6: Integrity Verification
        snap = self.state.membrane.snapshot()
        if snap["discriminant"] < 0:
            print(f"  [PHASE 6] {PHASE_NAMES[6]}: FAILED -- discriminant < 0")
            self.state.boot_outcome = "alert"
            self._boot_footer()
            return
        self.state.membrane.run_phase(6)
        self.state.completed_phases = 6
        self.state.validation_flags |= 0x20
        print(f"  [PHASE 6] {PHASE_NAMES[6]}: OK")

        self.state.boot_outcome = "pass"
        self.state.working = "ACTIVE"
        self._boot_footer()

    def _boot_footer(self) -> None:
        outcome_upper = self.state.boot_outcome.upper()
        print("=" * 60)
        print(f"  BOOT: phases={self.state.completed_phases}/6  "
              f"flags=0x{self.state.validation_flags:04X}  "
              f"outcome={outcome_upper}")
        print("=" * 60)
        print()

    # ---- REPL commands ----

    def do_status(self, arg: str) -> None:
        """Show current kernel and membrane state."""
        snap = self.state.membrane.snapshot()
        mpda_snap = self.state.mpda.snapshot()
        print(f"  Boot outcome   : {self.state.boot_outcome.upper()}")
        print(f"  Phases complete : {self.state.completed_phases}/6")
        print(f"  Flags          : 0x{self.state.validation_flags:04X}")
        print(f"  Membrane       : {snap['outcome_name'].upper()}")
        print(f"  Discriminant   : {snap['discriminant']:.4f}")
        print(f"  Track B        : {'OPEN' if snap['track_b_unlocked'] else 'LOCKED'}")
        print(f"  Scan (a,b,g)   : ({snap['scan']['alpha']}, {snap['scan']['beta']}, {snap['scan']['gamma']})")
        print(f"  MPDA state     : {mpda_snap['state']}")
        print(f"  MPDA theta     : {mpda_snap['theta']:.1f}")
        print(f"  MPDA stack     : depth={mpda_snap['stack_depth']}")

    def do_calibrate(self, arg: str) -> None:
        """Recalibrate membrane: calibrate <tier1> <tier2>

        Example: calibrate yes yes
                 calibrate maybe no"""
        parts = arg.split()
        if len(parts) < 2:
            print("  Usage: calibrate <tier1> <tier2>")
            print("  Values: yes, no, maybe, maybe_not")
            return
        try:
            t1 = _parse_trinary(parts[0])
            t2 = _parse_trinary(parts[1])
        except ValueError as exc:
            print(f"  Error: {exc}")
            return
        outcome = self.state.membrane.calibrate({1: t1, 2: t2})
        snap = self.state.membrane.snapshot()
        print(f"  Outcome      : {outcome.upper()}")
        print(f"  Discriminant : {snap['discriminant']:.4f}")
        print(f"  Track B      : {'OPEN' if snap['track_b_unlocked'] else 'LOCKED'}")

    def do_tripartite(self, arg: str) -> None:
        """Run tripartite discriminant: tripartite <u> <v> <w>

        U=user, V=institution, W=adversary.
        Example: tripartite yes yes maybe"""
        parts = arg.split()
        if len(parts) < 3:
            print("  Usage: tripartite <u> <v> <w>")
            return
        try:
            u, v, w = [_parse_trinary(p) for p in parts[:3]]
        except ValueError as exc:
            print(f"  Error: {exc}")
            return
        tri = tripartite_summary(u, v, w)
        print(f"  U={u}  V={v}  W={w}")
        print(f"  Delta          : {tri['delta']:.4f}")
        print(f"  Region         : {tri['region_name']}")
        print(f"  Consensus      : {tri['consensus_name']}")
        print(f"  Fault detected : {tri['fault_detected']}")
        if tri["has_real_roots"]:
            r1, r2 = tri["roots"]
            print(f"  Roots          : ({r1:.4f}, {r2:.4f})")

    def do_compose(self, arg: str) -> None:
        """Compose two trinary states: compose <a> <b>

        Example: compose yes maybe"""
        parts = arg.split()
        if len(parts) < 2:
            print("  Usage: compose <a> <b>")
            return
        try:
            a, b = _parse_trinary(parts[0]), _parse_trinary(parts[1])
        except ValueError as exc:
            print(f"  Error: {exc}")
            return
        result = trinary_compose(a, b)
        labels = {v: k for k, v in TRINARY_VALUES.items()}
        print(f"  compose({labels.get(a, a)}, {labels.get(b, b)}) = {labels.get(result, result)} ({result})")

    def do_resolve(self, arg: str) -> None:
        """Resolve trinary triad: resolve <want> <need> <should>

        Example: resolve yes maybe no"""
        parts = arg.split()
        if len(parts) < 3:
            print("  Usage: resolve <want> <need> <should>")
            return
        try:
            want, need, should = [_parse_trinary(p) for p in parts[:3]]
        except ValueError as exc:
            print(f"  Error: {exc}")
            return
        result = trinary_resolve(want, need, should)
        labels = {v: k for k, v in TRINARY_VALUES.items()}
        print(f"  resolve(want={labels.get(want, want)}, need={labels.get(need, need)}, "
              f"should={labels.get(should, should)}) = {labels.get(result, result)} ({result})")

    def do_enzyme(self, arg: str) -> None:
        """Apply enzyme operation: enzyme <op> <state>

        Ops: create(0), destroy(1), build(2), break(3), renew(4), repair(5)
        Example: enzyme renew maybe"""
        parts = arg.split()
        if len(parts) < 2:
            print("  Usage: enzyme <op> <state>")
            print("  Ops: create, destroy, build, break, renew, repair (or 0-5)")
            return
        op_str = parts[0].strip().lower()
        op_map = {name.lower(): idx for idx, name in ENZYME_NAMES.items()}
        try:
            op = op_map.get(op_str)
            if op is None:
                op = int(op_str)
        except ValueError:
            print(f"  Unknown enzyme op: {op_str}")
            return
        try:
            state = _parse_trinary(parts[1])
        except ValueError as exc:
            print(f"  Error: {exc}")
            return
        result = enzyme_apply(op, state)
        labels = {v: k for k, v in TRINARY_VALUES.items()}
        print(f"  enzyme({ENZYME_NAMES.get(op, op)}, {labels.get(state, state)}) = "
              f"{labels.get(result, result)} ({result})")

    def do_drift(self, arg: str) -> None:
        """Compute radial drift between two scans: drift <a1,b1,g1> <a2,b2,g2>

        Example: drift yes,yes,maybe  yes,no,maybe"""
        parts = arg.split()
        if len(parts) < 2:
            print("  Usage: drift <a1,b1,g1> <a2,b2,g2>")
            return
        try:
            prev = tuple(_parse_trinary(v) for v in parts[0].split(","))
            curr = tuple(_parse_trinary(v) for v in parts[1].split(","))
            if len(prev) != 3 or len(curr) != 3:
                raise ValueError("Each scan needs exactly 3 values (alpha,beta,gamma)")
        except ValueError as exc:
            print(f"  Error: {exc}")
            return
        d = drift_radial(prev, curr)
        print(f"  Radial drift: {d:.6f}")

    def do_mpda(self, arg: str) -> None:
        """Run MPDA automaton with input values: mpda <v1> <v2> ...

        Example: mpda yes no maybe yes"""
        parts = arg.split()
        if not parts:
            snap = self.state.mpda.snapshot()
            print(f"  State : {snap['state']}")
            print(f"  Theta : {snap['theta']:.1f}")
            print(f"  Stack : depth={snap['stack_depth']}")
            return
        try:
            values = [_parse_trinary(p) for p in parts]
        except ValueError as exc:
            print(f"  Error: {exc}")
            return
        self.state.mpda.reset()
        final_state = self.state.mpda.run(values)
        snap = self.state.mpda.snapshot()
        disc = self.state.membrane.discriminant
        accepted = self.state.mpda.accepts(disc)
        print(f"  Input    : {values}")
        print(f"  State    : {final_state}")
        print(f"  Theta    : {snap['theta']:.1f}")
        print(f"  Stack    : depth={snap['stack_depth']}")
        print(f"  Accepted : {accepted} (discriminant={disc:.4f})")

    def do_board(self, arg: str) -> None:
        """Render the Kanban board from current membrane state."""
        from .ui import ConsoleBoard, KanbanTask, KanbanTrack, KanbanColumn
        snap = self.state.membrane.snapshot()
        board = ConsoleBoard(self.state.membrane)
        board.add_task(KanbanTask("Physiological needs verified", KanbanTrack.FOUNDATION,
                                  KanbanColumn.DONE, snap["needs"]["tiers"][0]))
        board.add_task(KanbanTask("Safety scan completed", KanbanTrack.FOUNDATION,
                                  KanbanColumn.DONE, snap["needs"]["tiers"][1]))
        if self.state.membrane.track_b_unlocked:
            board.add_task(KanbanTask("Build OBINexus platform", KanbanTrack.ASPIRATION,
                                      KanbanColumn.BACKLOG, TRINARY_VALUES["maybe"]))
        board.add_task(KanbanTask("W-actor discriminant monitor", KanbanTrack.ADVERSARIAL,
                                  KanbanColumn.IN_PROGRESS, TRINARY_VALUES["maybe"]))
        print(board.render())

    def do_breathing(self, arg: str) -> None:
        """Show or set the breathing pointer priority hierarchy.

        Priority: BREATHING(0) > LIVING(1) > WORKING(2)
        Usage: breathing             -- show state
               breathing disrupted   -- set breathing state"""
        if not arg:
            print(f"  [{PRIORITY_LABELS[0]}]  {self.state.breathing}  (priority 0 -- mandatory)")
            print(f"  [{PRIORITY_LABELS[1]}]  {self.state.living}  (priority 1 -- mandatory)")
            print(f"  [{PRIORITY_LABELS[2]}]  {self.state.working}  (priority 2 -- optional)")
            return
        new_state = arg.strip().upper()
        if new_state == "DISRUPTED":
            self.state.breathing = "DISRUPTED"
            self.state.working = "HALTED"
            print("  BREATHING DISRUPTED -- working halted. System in safe mode.")
        elif new_state in ("FLOWING", "RESTORE"):
            self.state.breathing = "FLOWING"
            if self.state.booted:
                self.state.working = "ACTIVE"
            print(f"  Breathing restored. Working={self.state.working}")
        else:
            print(f"  Unknown state: {new_state}. Use 'disrupted' or 'flowing'.")

    def do_rwx(self, arg: str) -> None:
        """RWX state machine (2R=1W, 2W=1X, 4R=1X).

        Usage: rwx           -- show counters
               rwx read      -- increment read counter
               rwx write     -- increment write counter
               rwx reset     -- reset counters"""
        action = arg.strip().lower()
        if not action:
            print(f"  Reads    : {self.state.read_count}")
            print(f"  Writes   : {self.state.write_count}")
            print(f"  Executes : {self.state.execute_count}")
            print(f"  Law: 2R=1W, 2W=1X, 4R=1X")
            return
        if action == "read":
            self.state.read_count += 1
            promoted = ""
            if self.state.read_count >= 2:
                self.state.write_count += 1
                self.state.read_count -= 2
                promoted = " -> promoted to WRITE"
                if self.state.write_count >= 2:
                    self.state.execute_count += 1
                    self.state.write_count -= 2
                    promoted = " -> promoted to EXECUTE"
            print(f"  READ registered (R={self.state.read_count} W={self.state.write_count} "
                  f"X={self.state.execute_count}){promoted}")
        elif action == "write":
            self.state.write_count += 1
            promoted = ""
            if self.state.write_count >= 2:
                self.state.execute_count += 1
                self.state.write_count -= 2
                promoted = " -> promoted to EXECUTE"
            print(f"  WRITE registered (R={self.state.read_count} W={self.state.write_count} "
                  f"X={self.state.execute_count}){promoted}")
        elif action == "reset":
            self.state.read_count = 0
            self.state.write_count = 0
            self.state.execute_count = 0
            print("  RWX counters reset.")
        else:
            print("  Usage: rwx [read|write|reset]")

    def do_boot(self, arg: str) -> None:
        """Re-run the NSIGII 6-phase boot sequence.

        Usage: boot [tier1] [tier2]
        Example: boot yes yes"""
        parts = arg.split()
        t1 = parts[0] if len(parts) > 0 else "yes"
        t2 = parts[1] if len(parts) > 1 else "yes"
        self.state = KernelState()
        self._run_boot(t1, t2)

    def do_quit(self, arg: str) -> bool:
        """Exit the MMUKO-OS kernel."""
        print("  MMUKO-OS kernel shutting down.")
        return True

    do_exit = do_quit

    def do_EOF(self, arg: str) -> bool:
        """Handle Ctrl+D."""
        print()
        return self.do_quit(arg)

    def emptyline(self) -> None:
        pass

    def default(self, line: str) -> None:
        print(f"  Unknown command: {line}")
        print("  Type 'help' for available commands.")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main(argv: list[str] | None = None) -> int:
    import argparse
    parser = argparse.ArgumentParser(description="MMUKO-OS NSIGII Kernel REPL")
    parser.add_argument("--tier1", default="yes", help="Initial tier1 state (yes/no/maybe)")
    parser.add_argument("--tier2", default="yes", help="Initial tier2 state (yes/no/maybe)")
    args = parser.parse_args(argv)
    try:
        kernel = NSIGIIKernel(tier1=args.tier1, tier2=args.tier2)
        kernel.cmdloop()
    except KeyboardInterrupt:
        print("\n  Interrupted. MMUKO-OS kernel shutting down.")
    return 0
