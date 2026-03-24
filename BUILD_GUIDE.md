# MMUKO-OS Build Guide

## Quick Start

```bash
# 1. Install dependencies (one-time setup on WSL/Ubuntu)
make install-deps

# 2. Build everything: firmware + boot chain + disk image
make all

# 3. Verify the build
make verify

# 4. Boot in QEMU
make run
```

## Makefile Targets

### Core Builds

| Target | Purpose | Output |
|--------|---------|--------|
| `make all` | Build firmware, boot chain, and disk image | `build/libnsigii_firmware.so`, `build/boot.bin`, `build/mmuko-os.bin`, `build/mmuko-os.img` |
| `make firmware` | Compile C firmware (shared lib + static archive) | `build/lib/libnsigii_firmware.so`, `build/lib/libnsigii_firmware.a` |
| `make firmware-cpp` | Build C++ wrapper for firmware | `build/lib/libnsigii_firmware_cpp.so` |
| `make boot` | Assemble boot chain (stage-1, stage-2, runtime) | `build/boot.bin`, `build/mmuko-os.bin`, `build/runtime.bin` |
| `make image` | Write disk image (1.44 MB FAT12) | `build/mmuko-os.img` |

### Python/Cython Package

```bash
# Build Python wheel + source distribution
make cython-build

# Install editable package (development)
make cython-develop

# Run Python UI compositor
make run-ui
```

**How to use the built package:**

```python
# After: make cython-develop

import mmuko_os
from mmuko_os import firmware, kernel

# Access firmware APIs
health = firmware.check_health()
print(f"Membrane: {health.outcome}")  # PASS/HOLD/ALERT

# Run kernel phases
phases = kernel.run_boot_sequence()
for phase in phases:
    print(f"{phase.name}: {phase.state}")  # YES/NO/MAYBE
```

### Verification & Diagnostics

| Target | Purpose |
|--------|---------|
| `make verify` | NSIGII boot verification (stage sizes, signatures) |
| `make tree` | Display filesystem driver tree (root/trunk/branch/leaves) |
| `python3 scripts/verify_boot.py` | Extended verification with enzyme diagnostics |
| `python3 scripts/enzyme_panic.py --check` | Kernel panic recovery state check |

### Cleanup

| Target | Purpose |
|--------|---------|
| `make clean` | Remove all build artifacts |
| `python3 scripts/cleanup.py --dry-run` | Preview cleanup operations |
| `python3 scripts/cleanup.py` | Actually clean stale artifacts |

## Boot Chain Architecture

```
Stage-1 (boot sector, 512B, 0xAA55 signature)
    ↓ loads sectors 1..16
Stage-2 (mmuko-os.bin, NSIGII handoff, 512B..8192B)
    ↓ initializes state block & membrane
Runtime (runtime.bin, firmware entry, loads at 0x8200)
    ↓ reads NSIGII contract & reports system state

Image: build/mmuko-os.img (1.44 MB FAT12 floppy)
```

## Python Automation Scripts

Located in `scripts/`:

### `assemble_boot.py` — Boot chain assembler
When NASM isn't available, generates x86 boot binaries in pure Python.

```bash
python3 scripts/assemble_boot.py --build-dir build
```

Outputs: `build/boot.bin`, `build/mmuko-os.bin`, `build/runtime.bin`

### `build_image.py` — Disk image builder
Assembles boot chain into 1.44 MB floppy image.

```bash
python3 scripts/build_image.py \
  --stage1 build/boot.bin \
  --stage2 build/mmuko-os.bin \
  --runtime build/runtime.bin \
  --output build/mmuko-os.img
```

### `verify_boot.py` — Boot verification
NSIGII verification with enzyme membrane diagnostics.

```bash
python3 scripts/verify_boot.py --build-dir build
```

Output: Enzyme state (YES/NO/MAYBE), membrane outcome (PASS/HOLD/ALERT)

### `enzyme_panic.py` — Kernel panic strategy
Evaluates boot component health and triggers recovery.

```bash
# Check health
python3 scripts/enzyme_panic.py --check --build-dir build

# Attempt recovery if components are broken
python3 scripts/enzyme_panic.py --recover --build-dir build
```

### `hotswap_sector.py` — Hot-swappable sector manager
Inject/extract/swap/verify boot sectors in the disk image.

```bash
# Inject a new kernel
python3 scripts/hotswap_sector.py inject \
  --image build/mmuko-os.img \
  --bin new_kernel.bin \
  --sector 1

# Extract sectors for inspection
python3 scripts/hotswap_sector.py extract \
  --image build/mmuko-os.img \
  --sector 1 --count 16 \
  --output extracted.bin

# Atomic swap with rollback on failure
python3 scripts/hotswap_sector.py swap \
  --image build/mmuko-os.img \
  --bin patched_kernel.bin \
  --sector 1

# Verify image integrity
python3 scripts/hotswap_sector.py verify --image build/mmuko-os.img
```

### `tree_display.py` — Filesystem driver tree
Shows the root/trunk/branch/leaves hierarchy.

```bash
python3 scripts/tree_display.py --root .
```

Output:
```
root/
├── trunk/  (boot chain)
│   ├── branch/ stage1
│   ├── branch/ stage2
│   └── branch/ contract
├── trunk/  (kernel firmware)
├── trunk/  (drivers)
└── trunk/  (build output)
```

### `cleanup.py` — Codebase cleanup
Remove stale artifacts, Windows paths, empty .psc files.

```bash
# Dry-run (show what would be removed)
python3 scripts/cleanup.py --root . --dry-run

# Actually clean
python3 scripts/cleanup.py --root .
```

## Building the Python Wheel

The `pyproject.toml` at the root defines the Python package for MMUKO-OS.

```bash
# Build wheel + source distribution
python3 -m build

# Outputs:
#   dist/mmuko_os-0.1.0.tar.gz       (source distribution)
#   dist/mmuko_os-0.1.0-cp313-...    (wheel for Python 3.13)
```

**Install from wheel:**

```bash
pip install dist/mmuko_os-0.1.0-cp313-cp313-win_amd64.whl
```

**Or install in development mode (editable):**

```bash
make cython-develop
# or:
pip install -e .
```

## Enzyme Model: Recovery Strategy

The MMUKO-OS build uses the **enzyme model** for kernel panic recovery:

- **CREATE/DESTROY** — Phase initialization / teardown (YES/NO states)
- **BUILD/BREAK** — Sector assembly / disassembly (rebuild from source)
- **REPAIR/RENEW** — Panic recovery (restore from backup / recreate from stored patterns)

The membrane evaluates all boot components:

```
stage1: YES → CREATE/DESTROY
stage2: YES → CREATE/DESTROY
runtime: YES → CREATE/DESTROY
image: YES → CREATE/DESTROY

Membrane outcome: PASS (all YES) or HOLD (some MAYBE) or ALERT (any NO)
```

If any component fails (NO), the enzyme triggers:
1. **REPAIR** — Try to restore from backup sectors
2. **RENEW** — Recreate from stored patterns or full rebuild
3. **BREAK/BUILD** — Disassemble broken binary and reassemble from source

## Pseudocode & Specification

- **`pseudocode/mmuko.psc`** — Complete unified NSIGII spec
  - Trinary state algebra (YES/NO/MAYBE/MAYBE_NOT)
  - Enzyme model with CREATE/DESTROY, BUILD/BREAK, REPAIR/RENEW
  - 6-phase boot calibration
  - MMUKO boot handoff contract
  - Ringboot (hot-swappable boot sectors)

- **`pseudocode/mmuko-boot.psc`** — Boot-specific pseudocode
  - 6-phase boot sequence
  - Boot handoff contract
  - Kernel entry requirements

## Cross-Platform Notes

### WSL/Ubuntu
```bash
make install-deps  # Install nasm, gcc, build-essential
make all           # Full build
```

### Windows (with conda/Git Bash)
```bash
# Install NASM first (not in conda)
# Then use Git Bash:
make firmware      # GCC compiles via conda
make boot          # Run from WSL (nasm not in conda path)
make image         # Pure Python image builder
make run           # QEMU available via chocolatey or QEMU installer
```

### No NASM Available
The Makefile **automatically falls back** to the Python assembler:
```bash
[PYTHON] nasm not found — using Python fallback assembler
```

This works on any system with Python 3.

## Environment Variables

```bash
# Override toolchain
CC=clang make firmware        # Use Clang instead of GCC
NASM=/path/to/nasm make boot # Use custom NASM path

# Control build output
BUILD_DIR=mybuild make all    # Custom build directory
```

## Troubleshooting

**"nasm not found"**
→ Automatic fallback to Python assembler. If you want real NASM: `make install-deps`

**"build/obj: Permission denied"**
→ Run `make clean` and rebuild. The sandbox may have file permission issues.

**"Clock skew detected"**
→ Harmless warning. Build completed successfully. Ignore or rebuild.

**Python wheel won't install**
→ Ensure you're using the right Python version (3.10+):
```bash
python3 --version
python3 -m pip install dist/mmuko_os-*.whl
```

## Summary

| What You Want | Command |
|---------------|---------|
| Full build | `make all` |
| Just boot chain | `make boot` |
| Just firmware | `make firmware` |
| Just disk image | `make image` |
| Verify everything | `make verify` |
| Python package | `make cython-build` && `pip install dist/...whl` |
| Development Python | `make cython-develop` |
| Display tree | `make tree` |
| Clean up | `make clean` |
| Show all targets | `make help` |

---

**OBINexus Computing | MMUKO-OS Kernel | Enzyme Model Boot Strategy**
