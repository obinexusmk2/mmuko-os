# MMUKO-OS boot image specification

## Selected boot target

MMUKO-OS now targets a **BIOS hard-disk image with an MBR, a FAT16 partition, and a VBR + stage-2 loader**.

This replaces the previous floppy-style image stub that only copied a single 512-byte sector into a raw image without writing a filesystem.

## Disk layout

- Raw disk size: 16 MiB.
- LBA 0: MBR with one active FAT16 partition.
- Partition start: LBA 2048.
- Partition size: 30,720 sectors (15 MiB).
- Partition type: `0x06` (FAT16, up to 32 MiB).

## FAT16 filesystem layout

The FAT16 partition uses these on-disk structures:

- Bytes per sector: 512.
- Sectors per cluster: 4.
- Reserved sectors: 16.
  - Sector 0 in the partition: FAT16 VBR.
  - Sectors 1..N: stage-2 loader payload stored in the reserved area.
- FAT copies: 2.
- Root directory entries: 512.
- Filesystem label: `MMUKO BIOS`.

The image builder computes `sectors_per_fat` from the chosen geometry and the final file payload sizes.

## Boot flow

1. The BIOS loads the MBR at `0000:7C00`.
2. The MBR locates the active partition and chainloads its VBR.
3. The VBR reads the stage-2 loader from the partition reserved sectors using INT 13h extensions.
4. The stage-2 loader parses FAT16 structures, loads:
   - `KERNEL.BIN`
   - `BOOT.CFG`
   - `NSIGII.TXT`
5. Control transfers to `KERNEL.BIN`.

## FAT support policy

This repository now treats FAT as a real on-disk format, not a label written into BPB fields.

- The image builder writes the FAT16 BPB, FAT tables, root directory entries, and cluster chains.
- The stage-2 loader includes its own FAT16 parser under `boot/`.
- No project-specific `FAT64` format is used.
