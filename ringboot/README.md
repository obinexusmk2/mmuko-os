# MMUKO-OS Boot System

**OBINEXUS NSIGII-Verified Bootloader with Interdependency Tree Hierarchy**

A quantum-inspired operating system bootloader implementing stateless ring boot with non-deterministic finite automaton verification and interdependency resolution.

## Overview

MMUKO-OS (M for Mike, U for Uniform, K for Kilo, O for Oscar) is a multi-language boot system featuring:

- **NSIGII Protocol**: Trinary verification (YES/NO/MAYBE)
- **Interdependency Tree**: Hierarchical dependency resolution (A→B→C)
- **Quantum State Model**: 8 qubits with half-spin (π/4) allocation
- **Ring Boot State Machine**: SPARSE → REMEMBER → ACTIVE → VERIFY
- **RIFT Header**: 8-byte verification header with checksum
- **Multi-Language Support**: C, C++, C# implementations

## Project Structure

```
mmuko-os/
├── img/
│   └── mmuko-os.img          # 512-byte bootable image
├── include/
│   └── mmuko_types.h         # Core type definitions
├── src/
│   ├── boot_sector.asm       # x86 assembly boot sector
│   ├── mmuko_boot.c          # Main boot sequence
│   ├── interdependency.c     # Tree resolution system
│   └── obiboot.c/h           # Legacy boot support
├── cpp/
│   ├── riftbridge.hpp        # C++ interface
│   └── riftbridge.cpp        # C++ implementation
├── csharp/
│   └── RiftBridge.cs         # C# .NET implementation
├── build.sh                  # Main build script
├── ringboot.sh               # VirtualBox test script
└── README.md                 # This file
```

## Core Concepts

### NSIGII Verification States

```c
#define NSIGII_YES   0x55  // Boot verified (01010101)
#define NSIGII_NO    0xAA  // Boot failed (10101010)
#define NSIGII_MAYBE 0x00  // Pending verification
```

### Interdependency Tree Hierarchy

The boot sequence uses a tree structure for dependency resolution:

```
ROOT (0) - System initialization
  └── TRUNK (1) - Memory Manager
        ├── BRANCH (2) - Interrupt Handler
        │     └── LEAF (3) - Timer Service
        ├── BRANCH (4) - Device Manager
        │     └── LEAF (5) - Console Service
        └── BRANCH (6) - File System
              └── LEAF (7) - Boot Loader
```

Resolution order: Leaf → Branch → Trunk → Root (bottom-up)

### Quantum Spin Model

8 qubits representing compass directions with half-spin allocation:

| Qubit | Direction | Angle | State |
|-------|-----------|-------|-------|
| 0 | North | 0° | SPARSE |
| 1 | Northeast | π/4 | REMEMBER |
| 2 | East | π/2 | REMEMBER |
| 3 | Southeast | 3π/4 | ACTIVE |
| 4 | South | π | REMEMBER |
| 5 | Southwest | 5π/4 | REMEMBER |
| 6 | West | 3π/2 | REMEMBER |
| 7 | Northwest | 7π/4 | ACTIVE |

### Boot State Machine

```
SPARSE → REMEMBER → ACTIVE → VERIFY
   ↓        ↓          ↓         ↓
 Half    Memory    Full      NSIGII
 Spin    Alloc    Process   Check
```

## Building

### Prerequisites

- GCC compiler (C11 support)
- G++ compiler (C++17 support, optional)
- NASM assembler (optional, for assembly version)
- VirtualBox (for testing)
- Bash shell

### Build Process

```bash
# Make scripts executable
chmod +x build.sh ringboot.sh

# Build boot image
./build.sh
```

Expected output:
```
=== MMUKO-OS Build System ===
Interdependency Tree Hierarchy Boot

[1/6] Compiling C interdependency system...
✓ C compilation successful
[2/6] Linking boot sequence test...
✓ Linking successful
[3/6] Running NSIGII verification test...
✓ NSIGII verification PASSED (exit code 0)
[4/6] Assembling boot sector...
✓ Boot sector assembled with NASM
[5/6] Verifying boot image...
✓ Boot image is exactly 512 bytes
✓ RIFT header magic verified (NXOB)
✓ Boot signature (0x55AA) verified
[6/6] Building C++ RiftBridge...
✓ C++ RiftBridge compiled

=== Build Complete ===
Bootable image: img/mmuko-os.img
```

### C++ Build

```bash
cd cpp
g++ -std=c++17 -o riftbridge riftbridge.cpp
./riftbridge
```

### C# Build

```bash
cd csharp
dotnet build RiftBridge.cs
dotnet run -- --create-image ../img/mmuko-os-cs.img
```

## Testing in VirtualBox

### Automated Setup

```bash
./ringboot.sh
```

This script will:
1. Create a new VirtualBox VM
2. Attach the boot image as a floppy disk
3. Configure serial output logging
4. Start the VM
5. Monitor the boot sequence

### Manual VirtualBox Setup

1. Create new VM:
   - Name: MMUKO-OS-RingBoot
   - Type: Other
   - Version: Other/Unknown
   - RAM: 64MB

2. Add floppy controller:
   - Settings → Storage → Add Floppy Controller
   - Attach `img/mmuko-os.img`

3. Configure boot order:
   - Settings → System → Boot Order
   - Enable only Floppy

4. Start VM

### Expected Boot Sequence

The VM should display:

```
=== MMUKO-OS RINGBOOT ===
OBINEXUS NSIGII Verify
[Phase 1] SPARSE
[Phase 2] REMEMBER
[Phase 3] ACTIVE
[Phase 4] VERIFY

NSIGII_VERIFIED
BOOT_SUCCESS
```

Then halt with code `0x55` (NSIGII_YES).

## Technical Details

### RIFT Header Format

| Offset | Size | Field | Value | Description |
|--------|------|-------|-------|-------------|
| 0x00 | 4 | Magic | "NXOB" | OBINEXUS signature |
| 0x04 | 1 | Version | 0x01 | Protocol version |
| 0x05 | 1 | Reserved | 0x00 | Reserved |
| 0x06 | 1 | Checksum | 0xFE | XOR of header |
| 0x07 | 1 | Flags | 0x01 | Boot flags |

### Interdependency Resolution Algorithm

```c
// Topological sort with circular dependency detection
int interdep_resolve_tree(InterdepTree *tree) {
    // 1. Check for circular dependencies (DFS)
    if (has_circular_dep(tree->root)) return -1;
    
    // 2. Resolve dependencies bottom-up
    for each node in post-order:
        resolve_dependencies(node);
        execute_resolve_func(node);
        mark_resolved(node);
    
    return resolved_count;
}
```

### NSIGII Verification Logic

```c
if (verified_qubits >= 6) return NSIGII_YES;   // 0x55
if (verified_qubits < 3)  return NSIGII_NO;    // 0xAA
else                      return NSIGII_MAYBE; // 0x00
```

### Half-Spin Allocation

The system uses **half-spin** (π/4 rotations) to implement:

- **Double space, half time**: When in SPARSE state
- **Half space, double time**: When in ACTIVE state
- **Auxiliary star sequences**: No noise/noise, stop/start patterns

This corresponds to the polar coordinate model where:
- Each half-spin represents π/4 radians (45°)
- Full rotation is 8 half-spins (2π radians)
- State preservation uses conjugate pairs (N↔S, NE↔SW, etc.)

## Multi-Language API

### C Interface

```c
#include "mmuko_types.h"

// Initialize and boot
mmuko_boot_init();
mmuko_boot_sequence();

// Create interdependency tree
InterdepTree *tree = mmuko_create_boot_tree();
interdep_resolve_tree(tree);
```

### C++ Interface

```cpp
#include "riftbridge.hpp"
using namespace mmuko;

// Create bridge and boot
RiftBridge bridge;
NSIGIIState result = bridge.boot();

// Create boot image
bridge.createBootImage("mmuko-os.img");
```

### C# Interface

```csharp
using MMUKO;

// Create bridge and boot
var bridge = new RiftBridge();
NSIGIIState result = bridge.Boot();

// Create boot image
bridge.CreateBootImage("mmuko-os.img");
```

## RiftBridge Protocol

The RiftBridge protocol (from github.com/obinexus/riftbridge) provides:

1. **Cross-platform compatibility**: Windows, Linux, macOS
2. **Language interoperability**: C, C++, C#
3. **Consistent API**: Same boot sequence across languages
4. **Image generation**: 512-byte boot sector creation

### Protocol Features

- **Interdependency resolution**: Tree-based dependency management
- **Trinary logic**: YES/NO/MAYBE states
- **Quantum-inspired**: 8-qubit compass model
- **NSIGII verification**: Mathematical proof of boot integrity

## Troubleshooting

### Boot image not 512 bytes

```bash
# Check assembly output
cat build/obiboot_sector.asm

# Ensure padding is correct
times 510-($-$$) db 0
dw 0xAA55
```

### VM doesn't boot

1. Check boot order in VirtualBox settings
2. Verify floppy controller is attached
3. Ensure boot signature is present:
   ```bash
   xxd -s 510 -l 2 img/mmuko-os.img
   # Should show: 55aa
   ```

### NSIGII verification fails

1. Check qubit initialization in `mmuko_boot.c`
2. Verify half-spin allocation logic
3. Ensure transition count is correct
4. Check interdependency tree resolution

### Interdependency resolution fails

1. Check for circular dependencies
2. Verify tree structure is valid
3. Ensure all nodes have proper IDs (0-255)
4. Check dependency count matches actual dependencies

## Future Enhancements

- [ ] Lambda integration for energy measurement
- [ ] Tomographic onion layer encryption
- [ ] Multi-stage RIFT pipeline (TOKENISER → PARSER → AST)
- [ ] GossiLang coroutine bindings
- [ ] NLINK automaton state minimization
- [ ] Holographic boot interface (MMUKO-HoloLens)
- [ ] Noise-state verification (nosignal * nonoise)

## References

From the OBINEXUS documentation:
- **NSIGII Protocol**: Symbolic interpretation with pointer-based intent resolution
- **MUCO Boot**: Auxiliary star sequences with no-noise/noise patterns
- **Ring Boot**: Stateless active/sparse state machine with memory preservation
- **RIFT Ecosystem**: Single-pass compilation with policy-based thread safety
- **Interdependency**: github.com/obinexus/riftbridge tree hierarchy protocol

## License

Part of the OBINEXUS Computing Framework by Nnamdi Michael Okpala.

## Contact

For questions about NSIGII verification, MUCO boot sequences, or interdependency trees, refer to the source documents or the RIFT Ecosystem documentation.

---

**Built with care, compiled with intent, verified with NSIGII.**

*When systems fail, build your own.*
