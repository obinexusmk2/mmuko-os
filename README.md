# MMUKO-OS — NSIGII Heartfull Firmware Compositor

**OBINexus Computing | Nnamdi Michael Okpala**
**Version:** 0.1-DRAFT | 20 March 2026
**Status:** Active specification
**Canonical spec:** `MMUKO-OS.txt` is the authoritative input for generated artifacts.

---

## What this is

MMUKO-OS is a **constitutional computing environment** built on the principle that a computer should not boot until the operator's fundamental needs are verified. The NSIGII Heartfull Firmware is the pre-boot calibration layer that enforces this — it runs before any OS process, application, or UI loads.

The firmware implements a **Maslow needs-gate**: Tier 1 (physiological) and Tier 2 (safety) must be satisfied or actively pending before computation proceeds. This is not a metaphor. It is a boot protocol.

---

## Canonical boot spec and regeneration

`MMUKO-OS.txt` is the canonical MMUKO-OS boot specification and the source of truth for the boot phases, filesystem target, kernel handoff contract, NSIGII firmware requirements, and artifact names. Implementers should update the spec first, then regenerate derived files instead of hand-editing generated outputs.

### Regenerate derived boot files

```bash
python3 mmuko-boot/generate_from_spec.py
```

This generator emits:

- `mmuko-boot/generated/boot.asm` — canonical assembly skeleton for the boot sector.
- `mmuko-boot/generated/mmuko-boot.c` — canonical C implementation skeleton aligned to the same phase order.
- `mmuko-boot/include/mmuko_boot_spec.h` — native boot handoff definitions and constants.
- `mmuko-boot/include/mmuko_runtime_interface.h` — native runtime entry contract helpers.
- `mmuko-boot/generated/spec-validation.json` — machine-readable validation output derived from the canonical spec.

You can also run:

```bash
make -C mmuko-boot generate-spec
```

## Quick start

### 1. Assemble the boot sector

```powershell
# Generate the sources first, then assemble the generated stage-1 source
make codegen
nasm -f bin boot/mmuko_stage1_boot.asm -o boot.bin

# Verify: should be exactly 512 bytes with 0xAA55 at offset 510
```

### 2. Write the disk image

```powershell
# PowerShell — write boot sector to image
$boot = [System.IO.File]::ReadAllBytes("boot.bin")
$img  = [System.IO.File]::ReadAllBytes("mmuko-os.img")
[System.Array]::Copy($boot, 0, $img, 0, 512)
[System.IO.File]::WriteAllBytes("mmuko-os.img", $img)
```

### 3. Boot in QEMU

```powershell
qemu-system-x86_64.exe -drive format=raw,file=mmuko-os.img
```

You should see:

```
MMUKO-OS NSIGII v0.1
Y=1 N=0 M=-1
[N]Need
[S]Safe
[I]Ident
[G]Gov
[I]Probe
[I]Integ

NSIGII_VERIFIED
HR:PASS
BOOT_OK
```

### 4. Build the Python/Cython firmware package

```bash
# Install the package in editable mode for local development
python3 -m pip install -e .

# Build source + wheel artifacts
python3 -m pip install build
python3 -m build
```

Or use the Makefile (Linux/WSL only):

```bash
make codegen        # regenerate boot/, kernel/, include/, python/ from the canonical spec + pseudocode
make all            # run codegen, then build everything
make boot           # assemble generated boot sector only
make firmware       # build C shared library + archive (includes generated stage-2 loader)
make firmware-cpp   # build C++ wrapper plus generated bridge
make run            # boot with QEMU
make verify         # run NSIGII verification checks
make clean          # remove build artifacts
```

### 5. Run the Python console compositor

```bash
# Editable install recommended so the compiled extension is importable
make cython-develop

# Default console compositor run
make run-ui

# HOLD scenario (T1 pending)
PYTHONPATH=python python3 -m mmuko_os --tier1 maybe --tier2 maybe --w-actor maybe

# ALERT scenario (T1 violated)
PYTHONPATH=python python3 -m mmuko_os --tier1 no --tier2 maybe --w-actor maybe
```

---

## Canonical generator pipeline

The repository now treats `MMUKO-OS.txt` as the **authoritative textual input** for generated artifacts. The generator under `tools/mmuko_codegen/` reads that canonical spec first, then consumes `mmuko-boot/pseudocode/mmuko-boot.psc` as the primary executable pseudocode, and finally scans the remaining `.psc` files in `mmuko-boot/pseudocode/` as supporting context. Generated artifacts are written to `boot/`, `kernel/`, `include/`, and `python/` before native build targets run.

### Pseudocode-to-module mapping

| Pseudocode section | Generated module | Language | Purpose |
|---|---|---|---|
| `PHASE 0`–`PHASE 6` in `mmuko-boot.psc` | `boot/mmuko_stage1_boot.asm` | NASM | Emits the stage-1 bootloader skeleton and boot-sector handoff comments. |
| `PHASE 0`–`PHASE 6` in `mmuko-boot.psc` | `kernel/mmuko_stage2_loader.c` | C | Exposes the ordered phase table, spec summary, and pseudocode provenance for stage-2/native consumers. |
| Parsed exported loader metadata from `mmuko-boot.psc` | `kernel/mmuko_stage2_bridge.cpp` | C++ | Wraps the generated C loader metadata for higher-level native integrations. |
| Parsed interface symbols from the canonical spec and pseudocode | `include/mmuko_codegen.h` | C header | Defines the shared ABI used by generated native and Python bindings. |
| Generated ABI bindings for the shared header | `python/mmuko_codegen.pxd`, `python/mmuko_generated.pyx` | Cython | Binds the generated loader metadata into Python-facing extension code. |
| Supporting `.psc` documents in `mmuko-boot/pseudocode/` | `tools/mmuko_codegen/manifest.txt` | Text manifest | Records provenance for the pseudocode set consumed by generation. |

## File reference

| File | Language | Role |
|------|----------|------|
| `MMUKO-OS.txt` | Text | Canonical spec and authoritative generator input. |
| `tools/mmuko_codegen/generate.py` | Python | Generator that reads the canonical spec plus `.psc` inputs and emits boot/kernel/include/python artifacts. |
| `boot/mmuko_stage1_boot.asm` | NASM x86-16 | Generated stage-1 bootloader source derived from the canonical spec and `mmuko-boot.psc`. |
| `kernel/mmuko_stage2_loader.c` | C | Generated stage-2 loader metadata source. |
| `kernel/mmuko_stage2_bridge.cpp` | C++17 | Generated native bridge around the stage-2 loader metadata. |
| `include/mmuko_codegen.h` | C | Generated shared native interface for stage-2 consumers and bindings. |
| `python/mmuko_codegen.pxd` / `python/mmuko_generated.pyx` | Cython | Generated Python bindings for the stage-2 metadata ABI. |
| `boot.asm` | NASM x86-16 | Legacy hand-authored boot sector retained for reference. |
| `heartfull_firmware.h` | C | All types: `TrinaryState`, `MembraneOutcome`, `PerspectiveMembrane`, `QubitCompass`, `MaslowNeedsState`, `EnzymeOp`, `KanbanTrack`. |
| `heartfull_membrane.c` | C | Six-phase NSIGII calibrator. Trinary composition, enzyme degradation, compass rotation, drift theorem, membrane gate. |
| `bzy_mpda.h` / `bzy_mpda.c` | C | Byzantine Maybe PDA — formal 5-tuple `M=(Q,Σ,Γ,δ,q₀,F)`, magnetic transition table, pushdown stack, LTCodec reverse-read. |
| `tripartite_discriminant.h` / `.c` | C | `G={U,V,W}` discriminant `Δ=b²−4ac`, Byzantine fault detection, quadratic roots (BUILD/BREAK paths). |
| `nsigii_cpp_wrapper.cpp` | C++17 | Optional RAII wrappers: `Trinary`, `MembraneCalibrator`, `ByzantineChecker`, `MPDARunner`, `DriftMonitor`. |
| `python/mmuko_os/firmware.pxd` | Cython | Declares exported enums, structs, and functions from the native firmware headers. |
| `python/mmuko_os/firmware.pyx` | Cython | Wraps the native firmware, MPDA, and tripartite discriminant APIs for Python. |
| `python/mmuko_os/ui.py` | Python | Console compositor for the Cython workflow; renders a lightweight Kanban-style status view. |
| `pyproject.toml` / `setup.py` | Packaging | Setuptools/Cython build definition for `mmuko_os`. |
| `Makefile` | GNU Make | Build orchestration for firmware, boot media, and the Python/Cython workflow. |
| `boot.bin` | Binary | Pre-assembled boot sector (512 bytes). |
| `mmuko-os.img` | Binary | Pre-imaged 1.44 MB FAT12 disk image. |

---

## Architecture

```
Hardware / BIOS
      |
boot/mmuko_stage1_boot.asm  (NASM, 16-bit, generated)
  Phase N — Need-state init    (theta=0,   THERE_AND_THEN)
  Phase S — Safety scan        (theta=0,   T2 check)
  Phase I — Identity calib     (theta=120, HERE_AND_NOW)
  Phase G — Governance check   (theta=240, WHEN_AND_WHERE)
  Phase I — Internal probe P_I (compose alpha x beta x gamma)
  Phase I — Integrity / delta  (discriminant >= 0 ?)
      |
  MEMBRANE_PASS = 0xAA
      |
C firmware library (libnsigii_firmware.so / .a)
  heartfull_membrane.c    -- calibration engine
  bzy_mpda.c              -- Byzantine Maybe pushdown automaton
  tripartite_discriminant.c -- G={U,V,W} fault detection
      |
C++ wrapper (libnsigii_firmware_cpp.so)
  nsigii_cpp_wrapper.cpp  -- optional RAII layer
      |
Python/Cython package (mmuko_os)
  python/mmuko_os/firmware.pxd -- C declarations for native firmware
  python/mmuko_os/firmware.pyx -- compiled bindings for membrane/MPDA/discriminant
  python/mmuko_os/ui.py        -- console compositor / demo UI
```

### LTF — Linkable Then Executable

This project uses the **LTF (Linkable Then Format)** pipeline. Files are linked before they are permitted to execute. The Python console compositor is now the primary user-facing workflow and only renders a Track B view after the native membrane reaches PASS. In production, the assembly writes `OUTCOME_PASS = 0xAA` to a shared memory location that user-space readers can check before loading higher-order interfaces.

---

## Trinary alphabet

The firmware operates on a **four-value trinary alphabet**, not the classical binary:

| Symbol | Value | Meaning | Action |
|--------|-------|---------|--------|
| `YES` | `+1` | Needs met, contract honoured | Proceed |
| `NO` | `0` | Needs violated, contract breached | ALERT |
| `MAYBE` | `-1` | Needs uncertain, response delayed | Enzyme pathway |
| `MAYBE_NOT` | `-2` | Deferred — do NOT handle for operator | System absorbs |

### RIFT trinary composition rules

```
YES   * YES       = YES        (both confirmed)
NO    * anything  = NO         (NO absorbs)
MAYBE * MAYBE     = YES        (double negation resolves)
MAYBE * YES       = MAYBE      (uncertainty persists)
MAYBE_NOT * any   = MAYBE_NOT  (defer wins)
```

### Enzyme degradation — MAYBE states

When `MAYBE` is encountered on a Kanban thread, an enzyme operation is applied:

```
ENZYME_CREATE  : MAYBE -> YES    (creates a resolved state)
ENZYME_DESTROY : MAYBE -> NO     (destroys ambiguous state)
ENZYME_BUILD   : MAYBE -> YES    (builds toward resolution)
ENZYME_BREAK   : YES   -> MAYBE  (breaks apart certainty)
ENZYME_RENEW   : *     -> MAYBE  (refreshes to pending)
ENZYME_REPAIR  : NO    -> MAYBE  (patches violation back to pending)
```

---

## Byzantine discriminant — G = {U, V, W}

The tripartite discriminant detects adversarial interference in the constitutional relationship between operator (U), institution (V), and third-party/attacker (W):

```
Delta = b^2 - 4ac
  where b = U + V + W  (sum of trinary signals)
        a = 1, c = 1   (unit constitutional constants)

Delta > 0  -> STABLE   : two real roots, W is benign or absent
Delta = 0  -> CRITICAL : W is present and creating pressure
Delta < 0  -> FAULT    : W is actively disrupting U-V relationship
```

**Examples:**

| U | V | W | b | Delta | Region |
|---|---|---|---|-------|--------|
| +1 | +1 | +1 | +3 | +5 | STABLE |
| -1 | -1 | -1 | -3 | +5 | STABLE (all MAYBE is not a fault) |
| 0 | 0 | 0 | 0 | -4 | FAULT |
| +1 | -1 | 0 | 0 | -4 | FAULT |

Note: all-MAYBE (`b=-3`, `Delta=+5`) is **STABLE**, not a fault. MAYBE is the normal real-world state of an institution that has not yet responded. The system must handle it, not collapse it to failure.

---

## Maslow-Kanban three-track interface

The firmware exposes its state to the user via three Kanban tracks:

| Track | Maslow tiers | Condition |
|-------|-------------|-----------|
| **Track A** — Foundation | T1 (physiological) + T2 (safety) | Always active — PASS or HOLD |
| **Track B** — Aspiration | T3–T5 (belonging, esteem, actualisation) | **Locked** until membrane issues PASS |
| **Track W** — Adversarial | W-actor G={U,V,W} monitor | Always active |

Track B is unconditionally locked until T1 and T2 are both satisfied. You cannot self-actualise (build OBINexus, pursue PhD, deploy CORN) while the firmware is returning HOLD or ALERT on food, water, or shelter.

---

## Three-ring qubit compass

The firmware's internal scanning mechanism uses three temporal frames:

```
Ring 1 (outer)  — THERE_AND_THEN  : theta = 0    (historical needs record)
Ring 2 (middle) — HERE_AND_NOW    : theta = 120  (current scan)
Ring 3 (inner)  — WHEN_AND_WHERE  : theta = 240  (predicted state)
```

Coherence is maximum when all three rings align (`delta_theta = 0`). Drift is the measure of how far apart they are.

---

## Drift theorem

Radial and angular drift between two tripolar scan vectors:

```
V(t) = (alpha, beta, gamma)         -- tripolar observation vector
D(t) = dV(t)/dt                     -- drift vector

Dr = ||V_t|| - ||V_{t-1}||          -- radial drift
  Dr > 0 : needs diverging (resolving)
  Dr < 0 : needs converging (approaching satisfaction)

omega = d(theta)/dt                  -- angular drift
  Weighted observation: W(t) = (2/3)*P(t) + (1/3)*P(t-1)
```

---

## NSIGII six-phase identifier

| Position | Letter | NATO | Firmware role |
|----------|--------|------|---------------|
| 1 | N | November | Need-state initialisation |
| 2 | S | Sierra | Safety scan |
| 3 | I | India | Identity calibration (Uche/Obi/Eze tripolar) |
| 4 | G | Golf/Gold | Governance layer (OHA/IWU/IJI) |
| 5 | I | India | Internal probe (P_I activation) |
| 6 | I | India | Integrity verification (discriminant check) |

---

## Requirements

| Component | Requirement |
|-----------|-------------|
| `boot.asm` | NASM 2.14+ |
| `*.c` / `*.h` | GCC 9+ or Clang 10+ with C11 |
| `nsigii_cpp_wrapper.cpp` | GCC 9+ or Clang 10+ with C++17 |
| `*.cs` / `*.csproj` | .NET 8 SDK |
| QEMU | qemu-system-x86_64 |
| Makefile | GNU Make (Linux/WSL) |

---

## Igbo ontological framework

This system is grounded in the Igbo tripartite model of personhood and governance:

- **Uche / Obi / Eze** — mind / heart / will (identity calibration, Phase I)
- **OHA / IWU / IJI** — community / law / execution (governance layer, Phase G)

These are not decorative labels. They are the structural basis for the tripolar pointer algebra (`alpha=WANT`, `beta=NEED`, `gamma=SHOULD`) and the three-ring qubit compass.

---

*OBINexus Computing — Neurodivergent-First Constitutional Infrastructure*
