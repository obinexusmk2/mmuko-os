# MMUKO Boot Loader — Quick Start Guide

## ✅ What You Have

Your project now includes:
- ✅ **mmuko-boot.c** — Full bootloader with CAB loader integrated
- ✅ **profile_loader.h/c** — Standalone CAB parser
- ✅ **profile.cab** — Example calibration profile
- ✅ **Makefile** — Build configuration (corrected for mmuko-boot.c)
- ✅ **README.md** — Full documentation

## Windows/WSL Setup

### Step 1: Install WSL (if you haven't already)

```powershell
# In PowerShell as Administrator
wsl --install -d Ubuntu
```

### Step 2: Enter WSL Ubuntu

```powershell
# From PowerShell
wsl -d Ubuntu
```

### Step 3: Navigate to Your Project

```bash
cd /mnt/c/Users/OBINexus/Downloads/mmuko-boot-with-cab
ls -la
```

You should see:
```
mmuko-boot.c
profile_loader.c
profile_loader.h
profile.cab
Makefile
README.md
```

### Step 4: Build

```bash
make clean
make all
```

Expected output:
```
rm -f mmuko-boot *.o *.a
[CLEAN] Build artifacts removed
gcc -Wall -Wextra -std=c11 -O2 -lm -o mmuko-boot mmuko-boot.c
[BUILD] Complete: mmuko-boot
```

The warnings about `strncpy` are harmless (happens on all systems).

### Step 5: Run

```bash
./mmuko-boot
```

Expected output (first 30 lines):
```
MMUKO OS Boot Loader
OBINexus R&D — "Don't just boot systems. Boot truthful ones."
Version: 0.2-cab-integrated

Initialized MMUKO system with 16 bytes

Attempting to load CAB: profile.cab
[CAB] Loaded: cab-DEFAULT-001A
[CAB] Applied calibration and cubit values
  Gravity (vacuum): 9.8000
  Username: obinexus

=== MMUKO BOOT SEQUENCE v0.2-cab-integrated ===

[PHASE 0] Vacuum medium initialized: G=9.8000
[PHASE 1] Initializing cubit rings...
[PHASE 1] Initialized 16 cubit rings
[PHASE 2] Compass alignment...
[PHASE 2] All cubits aligned to compass directions
[PHASE 3] Entangling superposition pairs...
[PHASE 3] Resolved interference at byte 0, pair (0, 7)
...
[PHASE 7] MMUKO BOOT COMPLETE — All cubits aligned, no lock detected.

=== SYSTEM READY ===
CAB Profile: cab-DEFAULT-001A
Frame of reference: SOUTHWEST
Gravity medium: G=9.8000
```

## Makefile Commands

```bash
# Build (default)
make

# Clean build artifacts
make clean

# Run with profile.cab
make run

# Run with research.cab
make run-research

# Debug build (with -g -O0)
make debug

# Static analysis
make analyze

# Format code
make format

# Memory check (requires valgrind)
make memcheck

# Show help
make help
```

## Troubleshooting

### Issue: `make` command not found in Windows CMD

**Solution:** You need to use WSL. Run:
```powershell
wsl -d Ubuntu
```

Then cd to your project and run `make`.

### Issue: `rm -f` fails on Windows PowerShell

**Solution:** Use WSL (not PowerShell). The Makefile uses Unix commands.

### Issue: `gcc` command not found

**Solution:** Install build tools in WSL:
```bash
sudo apt update
sudo apt install build-essential
```

### Issue: Binary not found in Windows Explorer

**Explanation:** The binary `mmuko-boot` is a Linux ELF executable. It only runs in WSL/Linux. To run on Windows, you'd need to:
1. Compile with MinGW or MSVC
2. Or use WSL (recommended)

### Issue: File permission denied when running

**Solution:**
```bash
chmod +x mmuko-boot
./mmuko-boot
```

## Creating Custom CAB Profiles

Copy `profile.cab` to `research.cab` and edit:

```ini
[CAB_HEADER]
profile_id = cab-RESEARCH-002A
username = research-user

[CALIBRATION]
gravity_vacuum = 9.8
gravity_lepton = 0.98
# ... modify as needed

[CUBIT_RING]
byte0 = {raw:100}
byte1 = {raw:110}
# ... change initial values
```

Then run:
```bash
./mmuko-boot research.cab
```

## What Each File Does

| File | Purpose |
|------|---------|
| `mmuko-boot.c` | Main bootloader (all-in-one, CAB loader embedded) |
| `profile_loader.h` | CAB parser header (reference only, code is in mmuko-boot.c) |
| `profile_loader.c` | CAB parser standalone (reference only) |
| `profile.cab` | Example calibration profile |
| `Makefile` | Build configuration (Unix/Linux/WSL) |
| `README.md` | Full technical documentation |

## System Requirements

- **WSL Ubuntu** (or native Linux)
- **GCC** (build-essential package)
- **Make** (usually included with build-essential)
- **~1MB disk space** for binary

## Next Steps

1. ✅ Build and run `./mmuko-boot`
2. Create `research.cab` with custom settings
3. Run `./mmuko-boot research.cab`
4. Study the 7-phase boot output
5. Modify MMUKO_System or CAB profile structures
6. Document findings

## Reference

**Project:** MMUKO OS Boot Loader (OBINexus R&D)  
**Version:** 0.2-cab-integrated  
**Status:** Working ✅  
**Platform:** Linux/WSL/Unix  

---

**Questions?** Read README.md for architectural details.
