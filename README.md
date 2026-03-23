# MMUKO-OS — NSIGII Heartfull Firmware Compositor

**OBINexus Computing | Nnamdi Michael Okpala**  
**Version:** 0.2-DRAFT | 23 March 2026  
**Status:** Active specification

---

## Selected boot target

MMUKO-OS now targets a **BIOS hard-disk image with an MBR, a FAT16 partition, a FAT16 VBR, and a stage-2 loader**.

This replaces the old image flow that copied a single boot sector into a root-level `mmuko-os.img` without creating a real partition table or FAT filesystem.

The current boot path is:

```text
BIOS
  -> MBR (LBA 0)
  -> active FAT16 partition
  -> FAT16 VBR
  -> stage2 loader in reserved sectors
  -> FAT16 parser loads KERNEL.BIN, BOOT.CFG, NSIGII.TXT
  -> kernel payload
```

See `docs/boot-image-spec.md` for the on-disk layout and loader contract.

---

## Quick start

### Build the native firmware and the BIOS disk image

```bash
make firmware
make image
```

### Build everything

```bash
make all
```

### Boot in QEMU

```bash
make run
```

The generated BIOS image is written to `build/mmuko-os.img`.

### Verify the image layout

```bash
make verify
```

---

## What the image builder does

The image pipeline lives in `tools/build_image.py`. It:

1. assembles the MBR, VBR, stage-2 loader, and kernel payload with NASM,
2. creates a real 16 MiB disk image,
3. writes a valid MBR partition table,
4. formats the selected partition as FAT16,
5. installs the VBR and reserved-sector stage-2 loader,
6. writes FAT tables and root directory entries,
7. copies these payloads into the FAT16 volume:
   - `KERNEL.BIN`
   - `BOOT.CFG`
   - `NSIGII.TXT`

This repo no longer labels an image as FAT unless the FAT structures are actually written.

---

## Repository file reference

| File | Role |
|------|------|
| `boot/mbr.asm` | BIOS MBR that chainloads the active partition VBR. |
| `boot/vbr.asm` | FAT16 volume boot record that loads the reserved-sector stage-2 loader. |
| `boot/stage2.asm` | FAT16 stage-2 loader and root-directory parser. |
| `boot/kernel.asm` | Minimal kernel payload loaded from `KERNEL.BIN`. |
| `tools/build_image.py` | Real BIOS disk image builder and verifier. |
| `docs/boot-image-spec.md` | Selected boot target and on-disk image specification. |
| `config/boot.cfg` | Boot configuration copied into the FAT16 image. |
| `assets/nsigii.txt` | NSIGII asset payload copied into the FAT16 image. |
| `Makefile` | Build orchestration for firmware, boot artifacts, and image generation. |
| `heartfull_membrane.c` | NSIGII calibration engine. |
| `bzy_mpda.c` | Byzantine Maybe PDA support. |
| `tripartite_discriminant.c` | Tripartite discriminant implementation. |
| `NSIGII_HeartfullFirmware.cs` | C# compositor firmware layer. |
| `NSIGII_HeartFeltFirmware.cs` | C# UI layer. |
| `Main.cs` | .NET entry point. |

---

## FAT support policy

MMUKO-OS uses **standard FAT16** for the BIOS image.

- There is no project-specific `FAT64` format in the current boot path.
- FAT metadata in the BPB must match the image that is actually written.
- The stage-2 loader carries its own FAT16 parser instead of treating FAT labels as placeholders.

---

## Native firmware build

```bash
make firmware
make firmware-cpp
```

This builds:

- `build/lib/libnsigii_firmware.so`
- `build/lib/libnsigii_firmware.a`
- `build/lib/libnsigii_firmware_cpp.so`

---

## C# compositor

```bash
make compositor
make run-compositor
```

Development examples:

```bash
dotnet run --project mmuko-compositor.csproj -- --simulate-pass
dotnet run --project mmuko-compositor.csproj -- --boot-passed true --tier1 yes --tier2 yes
dotnet run --project mmuko-compositor.csproj -- --boot-passed true --tier1 maybe --tier2 maybe
dotnet run --project mmuko-compositor.csproj -- --boot-passed true --tier1 no --tier2 maybe
```
