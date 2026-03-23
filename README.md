# MMUKO-OS

MMUKO-OS now uses a **two-stage BIOS boot flow**:

1. `boot.asm` is a **minimal stage-1 BIOS loader only**.
2. `mmuko-boot/stage2.asm` is the **stage-2 loader**.
3. `mmuko-boot/runtime.asm` is a sample **native MMUKO runtime image** with an explicit header.
4. `mmuko-boot/mkimage.c` writes all three artifacts into `mmuko-os.img`.

The image format is a **custom raw reserved-sector layout**, not FAT12 and not a pseudo-"FAT64" reuse of FAT12 metadata. This keeps the stage-1 loader small and avoids pretending that a custom layout is FAT-compatible.

## Boot layout decision

Compatibility with standard FAT tooling is **not** the current priority for this image. Instead, the disk image uses a fixed raw layout that is straightforward for BIOS code to load.

### On-disk layout

All offsets below are in 512-byte sectors:

| Region | Start LBA | Sector count | Purpose |
|---|---:|---:|---|
| Stage-1 boot sector | 0 | 1 | BIOS entrypoint, loads stage-2 into memory. |
| Stage-2 loader | 1 | 16 | Real-mode loader that reads the runtime image and switches to protected mode. |
| Native MMUKO runtime image | 17 | 32 | Runtime header plus protected-mode code. |
| Remaining disk | 49 | 2831 | Reserved for future filesystem or data use. |

### Memory layout during boot

| Component | Load address | Notes |
|---|---:|---|
| BIOS stage-1 (`boot.asm`) | `0x0000:0x7C00` | Standard BIOS boot address. |
| Stage-2 loader | `0x0800:0x0000` (`0x00008000`) | Loaded by stage-1 from fixed sectors. |
| Runtime image | `0x1000:0x0000` (`0x00010000`) | Loaded by stage-2 before validation. |

## Runtime image header

The runtime image starts with an explicit header. If someone later wants to call the format “FAT64”, they must define a real filesystem; until then this image is **raw MMUKO boot media**.

```c
struct mmuko_runtime_header {
    uint32_t magic;        // 'MMKR'
    uint32_t signature;    // 'NSIG'
    uint16_t version;      // 1
    uint16_t header_size;  // sizeof(header)
    uint32_t image_size;   // full runtime payload size in bytes
    uint32_t entry_offset; // protected-mode entry offset from image base
    uint32_t load_address; // expected physical load address (0x00010000)
    uint32_t reserved;     // 0 for now
};
```

Validation rules used by stage-2:

- `magic == 'MMKR'`
- `signature == 'NSIG'`
- `version == 1`
- `image_size` must fit in the runtime reserved-sector allocation
- `entry_offset` must point inside the image
- `load_address == 0x00010000`

## What each boot stage does

### Stage-1 (`boot.asm`)

- Initializes 16-bit real-mode segments and stack.
- Reads the stage-2 loader from fixed LBAs 1-16 using BIOS INT 13h sector reads.
- Jumps directly to stage-2.

### Stage-2 (`mmuko-boot/stage2.asm`)

- Reads the runtime image from fixed LBAs 17-48.
- Validates the runtime header magic/signature and basic size/address constraints.
- Builds a small GDT.
- Enables protected mode.
- Jumps to the runtime entrypoint described by the validated header.

### Runtime (`mmuko-boot/runtime.asm`)

- Exposes the native runtime header consumed by stage-2.
- Contains a simple 32-bit entrypoint proving the protected-mode transfer works.
- Writes a status message to VGA text memory and halts.

## Build and image creation

### Build everything

```bash
make all
```

### Build only the boot path

```bash
make boot
make image
make verify
```

`make image` now writes **stage-1, stage-2, and the runtime image** into `mmuko-os.img`; it no longer copies only the first 512 bytes.

## File reference

| File | Role |
|---|---|
| `boot.asm` | Minimal 512-byte BIOS stage-1 loader. |
| `mmuko-boot/stage2.asm` | Stage-2 real-mode loader with runtime validation and protected-mode handoff. |
| `mmuko-boot/runtime.asm` | Native MMUKO runtime image with header + 32-bit entrypoint. |
| `mmuko-boot/image_layout.h` | Shared constants and the explicit on-disk/runtime header definition. |
| `mmuko-boot/mkimage.c` | Image creation tool that places all boot artifacts into `mmuko-os.img`. |
| `Makefile` | Root build orchestration for firmware libraries and boot image generation. |

## Future filesystem note

If filesystem compatibility becomes important later, prefer one of these approaches:

1. Move to **FAT32** and implement a real FAT32-aware loader, or
2. Keep this **custom raw layout** and define a dedicated MMUKO filesystem from scratch.

Do **not** label a non-FAT image as FAT12/FAT32/FAT64 unless its on-disk metadata and loader behavior actually implement that filesystem.
