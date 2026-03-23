# MMUKO-OS — NSIGII Heartfull Firmware

**OBINexus Computing | Nnamdi Michael Okpala**  
**Version:** 0.1-DRAFT | 20 March 2026  
**Status:** Active specification
**Canonical spec:** `MMUKO-OS.txt` is the authoritative input for generated artifacts.

1. `boot.asm` is a **minimal stage-1 BIOS loader only**.
2. `mmuko-boot/stage2.asm` is the **stage-2 loader**.
3. `mmuko-boot/runtime.asm` is a sample **native MMUKO runtime image** with an explicit header.
4. `mmuko-boot/mkimage.c` writes all three artifacts into `mmuko-os.img`.

## Boot architecture

MMUKO-OS is a constitutional computing environment whose boot boundary stays in native NASM/C/C++ artifacts, while the higher-level compositor now runs through a Python/Cython package. The NSIGII Heartfull Firmware still performs the six-phase calibration and keeps the native ABI stable for the boot/runtime boundary; Cython is the orchestration layer above that native surface.

### On-disk layout

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

```bash
make firmware
make firmware-cpp
```

This preserves the native ABI in:

- `build/lib/libnsigii_firmware.so`
- `build/lib/libnsigii_firmware.a`
- `build/lib/libnsigii_firmware_cpp.so`

### 2. Build the Python/Cython compositor

```bash
make compositor
```

That installs the package in editable mode and builds `python/mmuko_os/_firmware.pyx` against the existing native sources.

### 3. Run the compositor

```bash
make run-compositor
```

### 4. Build the Python/Cython firmware package

```bash
# Install the package in editable mode for local development
python3 -m pip install -e .

# Build source + wheel artifacts
python3 -m pip install build
python3 -m build
```

Additional runtime examples:

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

Validation rules used by stage-2:

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

The boot/runtime boundary continues to target native artifacts:

```text
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
libnsigii_firmware.so / .a
libnsigii_firmware_cpp.so
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

### Stage-1 (`boot.asm`)

This project uses the **LTF (Linkable Then Format)** pipeline. Files are linked before they are permitted to execute. The Python console compositor is now the primary user-facing workflow and only renders a Track B view after the native membrane reaches PASS. In production, the assembly writes `OUTCOME_PASS = 0xAA` to a shared memory location that user-space readers can check before loading higher-order interfaces.

### Stage-2 (`mmuko-boot/stage2.asm`)

## Build and run

### Build firmware + boot image

```bash
make all
```

Artifacts are generated under `build/`:

- `build/boot/stage1.bin`
- `build/boot/stage2-loader.bin`
- `build/boot/runtime.bin`
- `build/boot/stage2.bin`
- `build/mmuko-os.img`

### Verify boot artifacts

```bash
make verify
```

That means:

- `boot.asm` remains the pre-OS entry point.
- the C ABI in `heartfull_firmware.h`, `bzy_mpda.h`, and `tripartite_discriminant.h` remains the stable firmware surface.
- C++ helper exports from `nsigii_cpp_wrapper.cpp` remain available where the abstractions are still useful.
- Python/Cython orchestrates the firmware state machine and developer-facing workflows without replacing the native boundary itself.

---

## Main build targets

```bash
make all
make firmware
make firmware-cpp
make compositor
make run-compositor
make verify
make clean
```

---

## Legacy C# status

The historical C# compositor sources are retained in the repository as legacy reference material, but they are no longer part of the primary getting-started flow. The main documentation and Makefile path now target the Python/Cython compositor instead of `.csproj` / `dotnet` entry points.
