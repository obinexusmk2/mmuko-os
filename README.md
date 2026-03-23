# MMUKO-OS — NSIGII Heartfull Firmware Compositor

**OBINexus Computing | Nnamdi Michael Okpala**  
**Version:** 0.1-DRAFT | 20 March 2026  
**Status:** Active specification

---

## What this is

MMUKO-OS is a constitutional computing environment whose boot boundary stays in native NASM/C/C++ artifacts, while the higher-level compositor now runs through a Python/Cython package. The NSIGII Heartfull Firmware still performs the six-phase calibration and keeps the native ABI stable for the boot/runtime boundary; Cython is the orchestration layer above that native surface.

---

## Quick start

### 1. Build the native firmware boundary

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

Equivalent direct invocation:

```bash
PYTHONPATH=python python3 -m mmuko_os --simulate-pass --tier1 yes --tier2 yes --w-actor yes
```

Additional runtime examples:

```bash
PYTHONPATH=python python3 -m mmuko_os --boot-passed true --tier1 yes --tier2 yes --w-actor yes
PYTHONPATH=python python3 -m mmuko_os --boot-passed true --tier1 maybe --tier2 maybe --w-actor maybe
PYTHONPATH=python python3 -m mmuko_os --boot-passed true --tier1 no --tier2 maybe --w-actor maybe
```

### 4. Optional boot image flow

```bash
make boot
make image
make run
```

The boot path remains native. The Python/Cython layer starts only after the boot/runtime boundary has been satisfied.

---

## Python package layout

| File | Role |
|------|------|
| `python/mmuko_os/_firmware.pyx` | Cython bindings over the C firmware, MPDA, tripartite discriminant, and selected C++ exports. |
| `python/mmuko_os/heartfull_firmware.pxd` | Cython declarations for `heartfull_firmware.h`. |
| `python/mmuko_os/bzy_mpda.pxd` | Cython declarations for `bzy_mpda.h`. |
| `python/mmuko_os/tripartite_discriminant.pxd` | Cython declarations for `tripartite_discriminant.h`. |
| `python/mmuko_os/api.py` | Python-facing API that replaces the public compositor behavior previously surfaced through the C# layer. |
| `python/mmuko_os/__main__.py` | CLI entry point used by `make run-compositor`. |
| `setup.py` / `pyproject.toml` | Build configuration for the Python/Cython package. |

---

## Native/runtime boundary

The boot/runtime boundary continues to target native artifacts:

```text
Hardware / BIOS
      |
boot.asm
      |
libnsigii_firmware.so / .a
libnsigii_firmware_cpp.so
      |
Python/Cython compositor (mmuko_os)
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
