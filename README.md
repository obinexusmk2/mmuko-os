# MMUKO-OS — NSIGII Heartfull Firmware

**OBINexus Computing | Nnamdi Michael Okpala**  
**Version:** 0.3-DRAFT | 23 March 2026

---

## Boot architecture

The BIOS boot path is split so the first sector stays minimal and auditable:

1. **`boot/stage1.asm`** — 512-byte BIOS/FAT12 entry sector.
2. **`boot/stage2.asm`** — stage-2 loader at `0000:8000`.
3. **`kernel/runtime.asm`** — firmware runtime entry at `0000:8200`.

Stage-1 only performs BPB setup, real-mode setup, handoff-contract initialization, BIOS disk reset/read, and stage-2 transfer. Any future filesystem parsing belongs in stage-2, not in the BIOS entry sector.

---

## Handoff contract

Stage-1 publishes a fixed handoff contract at **`0000:0600`** before transferring control.

### Load addresses

- **Stage-1 entry:** `0000:7C00`
- **Stage-2 loader:** `0000:8000`
- **Runtime entry:** `0000:8200`
- **NSIGII state block:** `0000:0680`
- **Memory map table:** `0000:06C0`

### Contract contents

The contract preserves:

- BIOS boot drive from `DL`
- stage-2 segment:offset
- runtime segment:offset
- stage-2 sector count
- NSIGII membrane outcome
- tier-1 / tier-2 trinary state
- BIOS conventional memory size
- last BIOS disk error
- pointers to the state block and memory map table

### Low-memory map

| Region | Address | Size | Purpose |
|---|---:|---:|---|
| Boot contract | `0000:0600` | `0x40` | Stage-1 → stage-2/runtime handoff |
| NSIGII state block | `0000:0680` | `0x40` | Membrane + tier state |
| Memory map table | `0000:06C0` | `4 x 8` bytes | Published stage-owned regions |
| Stage-1 sector | `0000:7C00` | `0x200` | BIOS entry sector |
| Stage-2 loader | `0000:8000` | `0x200` | Loader / ontological init |
| Runtime | `0000:8200` | variable | Firmware runtime |

---

## Repository layout

| Path | Role |
|---|---|
| `boot/stage1.asm` | Minimal 512-byte BIOS entry sector. |
| `boot/stage2.asm` | Stage-2 loader and NSIGII interface initialization. |
| `boot/contract.inc` | Shared constants for the boot handoff contract. |
| `kernel/runtime.asm` | First real firmware runtime entry. |
| `legacy/csharp-compositor/` | Archived C# compositor project kept outside the active boot path. |
| `Makefile` | Source-first build orchestration for firmware, boot stages, and image generation. |

---

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

### Boot in QEMU

```bash
make run
```

### Build the legacy C# compositor

```bash
make compositor
```

The compositor project now lives at `legacy/csharp-compositor/mmuko-compositor.csproj`.

---

## Cleaning generated files

```bash
make clean
```

`make clean` removes generated boot/image artifacts under `build/`, legacy .NET `bin/` / `obj/` folders, and old root-level boot/image leftovers from earlier revisions.

---

## Notes

- The boot image currently uses **raw contiguous sector loading** for stage-2.
- If file-based loading is added later, FAT parsing should live in **stage-2**, not stage-1.
- `boot.asm` remains as a compatibility include that simply pulls in `boot/stage1.asm`.
