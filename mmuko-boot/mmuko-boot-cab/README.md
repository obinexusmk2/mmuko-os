# MMUKO OS Boot Loader with CAB Profile Integration

## Project Status

✅ **CAB Loader Integration Complete**

The MMUKO bootloader now supports dynamic calibration profiles via `.cab` (Calibration Archive Bundle) files. The system reads calibration constants, cubit values, and user profiles from the CAB file at startup.

---

## What's New (v0.2)

### CAB Profile Loader
- **profile_loader.h/c** — Standalone CAB parser
- **mmuko-boot.c** — Integrated CAB loader in main executable
- **profile.cab** — Example calibration profile

### CAB File Format

A CAB file contains seven sections:

```
[CAB_HEADER]       → Profile metadata
[CALIBRATION]      → Gravity constants (vacuum, lepton, muon, deep)
[USER_PROFILE]     → Behavioral fingerprint (username, entropy, latency)
[AST_SIGNATURES]   → Program structural signatures
[CUBIT_RING]       → Initial cubit byte values
[BOOT_VALIDATION]  → Test results
[CAB_SIGNATURE]    → Security hash
```

---

## Build & Run

### Quick Build

```bash
make clean && make all
```

### Run with CAB

```bash
# Default profile.cab
./mmuko-boot

# Specify a CAB file
./mmuko-boot research.cab

# Using Makefile target
make run
make run-research
```

### Debug Build

```bash
make debug
```

### Memory Check (Valgrind)

```bash
make memcheck
```

---

## Boot Flow (with CAB)

```
1. Create system (16 bytes default)
2. Load default CAB profile
3. Attempt to read profile.cab
4. Parse [CALIBRATION] → apply gravity constants
5. Parse [CUBIT_RING] → load cubit raw values
6. Execute 7-phase boot sequence:
   - PHASE 0: Vacuum medium init (from CAB)
   - PHASE 1: Cubit ring initialization
   - PHASE 2: Compass alignment
   - PHASE 3: Superposition entanglement
   - PHASE 4: Frame of reference centering
   - PHASE 5: Nonlinear index resolution
   - PHASE 6: Rotation freedom verification
   - PHASE 7: Boot complete
7. Launch kernel scheduler
```

---

## CAB Profile Example

```ini
[CAB_HEADER]
profile_id = cab-DEFAULT-001A
author = OBINexus
created = 2026-03-13

[CALIBRATION]
gravity_vacuum = 9.8
gravity_lepton = 0.98
gravity_muon = 0.098
gravity_deep = 0.0098
frame_reference = SOUTHWEST

[USER_PROFILE]
username = obinexus
profile_hash = 7f3a22d90aa12bfe
mouse_entropy = 0.442
typing_latency = 128
command_pattern = nonlinear

[CUBIT_RING]
byte0 = {raw:42}
  cubit0 = {value:0, dir:N, spin:0.7854}
  ...
byte1 = {raw:59}
...
```

---

## Portable OS Personalities

One of the powerful features: different CAB files can give the OS different "personalities":

```bash
./mmuko-boot default.cab      # Standard configuration
./mmuko-boot research.cab     # Research setup
./mmuko-boot secure.cab       # Security-hardened
./mmuko-boot minimal.cab      # Minimal footprint
```

Each CAB contains:
- Different gravity calibrations
- Different cubit initialization values
- Different user profiles
- Different AST signature sets

So the same bootloader behaves differently based on which CAB is loaded.

---

## System Output Example

When you run `./mmuko-boot` with `profile.cab`:

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
Gravity medium: G=9.8000 (lepton=0.9800, muon=0.0980, deep=0.0098)

Sample cubit states:
Byte[0].Cubit[0]: val=0, dir=NORTH, state=DOWN, spin=0.7854, super=YES, ent=7
Byte[0].Cubit[2]: val=0, dir=EAST, state=DOWN, spin=1.5708, super=YES, ent=5
Byte[5].Cubit[5]: val=0, dir=SOUTHWEST, state=UP, spin=1.5708, super=YES, ent=2

Launching kernel scheduler...
```

---

## File Structure

```
mmuko-boot/
├── mmuko-boot.c          (main bootloader with CAB integration)
├── profile_loader.h      (CAB loader header)
├── profile_loader.c      (CAB loader implementation)
├── profile.cab           (example default profile)
├── Makefile              (build configuration)
└── README.md             (this file)
```

---

## Architecture

### MMUKO_System Structure (Enhanced)

```c
typedef struct {
    MMUKO_Byte* memory_map;         // 16 bytes of cubits
    size_t memory_size;             // 16
    VacuumMedium medium;            // Gravity constants
    Direction frame_of_reference;   // SOUTHWEST (default)
    bool boot_complete;             // Status flag
    char cab_loaded[256];           // Track loaded profile
} MMUKO_System;
```

### CAB_Profile Structure

```c
typedef struct {
    char profile_id[64];
    double gravity_vacuum;
    double gravity_lepton;
    char username[64];
    uint8_t cubit_values[16];
    size_t cubit_count;
    // ... 20+ fields total
} CAB_Profile;
```

---

## Next Steps

### Short-term
- [ ] Create `research.cab` profile
- [ ] Create `secure.cab` profile  
- [ ] Add CAB signature verification (SHA-256)
- [ ] Implement CAB encryption

### Medium-term
- [ ] Formal cubit ring algebra definition
- [ ] Boot convergence proof
- [ ] AST fingerprint verification
- [ ] Behavioral telemetry integration

### Long-term
- [ ] Publish research paper (MMUKO cybernetic OS model)
- [ ] Build kernel scheduler layer
- [ ] Implement process/memory management
- [ ] Full OS prototype

---

## Compilation Warnings

Minor strncpy truncation warnings on Mac/Clang are benign — they're flagged because destination buffers are fixed-size, but actual data fits.

To suppress:
```bash
gcc -Wall -Wextra -std=c11 -O2 -Wno-stringop-truncation -lm -o mmuko-boot mmuko-boot.c
```

---

## Philosophy

> "Don't just boot systems. Boot truthful ones."

MMUKO represents a new OS paradigm:
- **Topological memory** (cubits arranged as compass rings)
- **Cybernetic identity** (behavioral fingerprints in boot)
- **Calibration-driven** (system aligns to user + environment)
- **No lock memory** (free rotation, no stuck states)

The CAB profile is the **system identity capsule** — it embeds who the system is and how it should behave.

---

## License

OBINexus R&D — Internal research project

---

## Contact

Questions about MMUKO? The code is self-documenting. Run it. Observe the phases. Understand the geometry.

**Version:** 0.2-cab-integrated  
**Last Updated:** 2026-03-13  
**Status:** Active Development
