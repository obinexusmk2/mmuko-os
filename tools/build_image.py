#!/usr/bin/env python3
from __future__ import annotations

import math
import os
import struct
import subprocess
from dataclasses import dataclass
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = REPO_ROOT / "build"
BOOT_DIR = REPO_ROOT / "boot"
CONFIG_DIR = REPO_ROOT / "config"
ASSET_DIR = REPO_ROOT / "assets"

SECTOR_SIZE = 512
DISK_SECTORS = 32_768          # 16 MiB
PARTITION_START_LBA = 2_048    # 1 MiB aligned
PARTITION_SECTORS = 30_720     # 15 MiB FAT16 partition
SECTORS_PER_CLUSTER = 4
RESERVED_SECTORS = 16
FAT_COUNT = 2
ROOT_ENTRIES = 512
MEDIA_DESCRIPTOR = 0xF8
SECTORS_PER_TRACK = 63
HEADS = 16
PARTITION_TYPE = 0x06
VOLUME_LABEL = b"MMUKO BIOS "
FS_TYPE = b"FAT16   "
VOLUME_ID = 0x20260323

MBR_BIN = BUILD_DIR / "mbr.bin"
VBR_BIN = BUILD_DIR / "vbr.bin"
STAGE2_BIN = BUILD_DIR / "stage2.bin"
KERNEL_BIN = BUILD_DIR / "kernel.bin"
DISK_IMG = BUILD_DIR / "mmuko-os.img"


@dataclass
class FilePayload:
    host_path: Path
    image_name: bytes
    data: bytes
    start_cluster: int = 0


class BuildError(RuntimeError):
    pass


def run(*args: str) -> None:
    subprocess.run(args, cwd=REPO_ROOT, check=True)


def assemble(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    run("nasm", "-f", "bin", str(src), "-o", str(dst))


def compute_sectors_per_fat(total_sectors: int) -> int:
    root_dir_sectors = math.ceil(ROOT_ENTRIES * 32 / SECTOR_SIZE)
    sectors_per_fat = 1
    while True:
        data_sectors = total_sectors - RESERVED_SECTORS - (FAT_COUNT * sectors_per_fat) - root_dir_sectors
        cluster_count = data_sectors // SECTORS_PER_CLUSTER
        needed = math.ceil(((cluster_count + 2) * 2) / SECTOR_SIZE)
        if needed == sectors_per_fat:
            return sectors_per_fat
        sectors_per_fat = needed


def to_chs(lba: int) -> bytes:
    sectors_per_cylinder = HEADS * SECTORS_PER_TRACK
    c = lba // sectors_per_cylinder
    temp = lba % sectors_per_cylinder
    h = temp // SECTORS_PER_TRACK
    s = (temp % SECTORS_PER_TRACK) + 1
    if c > 1023:
        return b"\xFE\xFF\xFF"
    ch = c & 0xFF
    cl = s | ((c >> 2) & 0xC0)
    dh = h & 0xFF
    return bytes((dh, cl, ch))


def patch_mbr(binary: bytearray) -> None:
    if len(binary) != SECTOR_SIZE:
        raise BuildError(f"MBR must be 512 bytes, got {len(binary)}")

    entry_offset = 446
    start_chs = to_chs(PARTITION_START_LBA)
    end_chs = to_chs(PARTITION_START_LBA + PARTITION_SECTORS - 1)
    entry = bytearray(16)
    entry[0] = 0x80
    entry[1:4] = start_chs
    entry[4] = PARTITION_TYPE
    entry[5:8] = end_chs
    struct.pack_into("<I", entry, 8, PARTITION_START_LBA)
    struct.pack_into("<I", entry, 12, PARTITION_SECTORS)
    binary[entry_offset:entry_offset + 16] = entry


def patch_vbr(binary: bytearray, sectors_per_fat: int, stage2_sectors: int) -> None:
    if len(binary) != SECTOR_SIZE:
        raise BuildError(f"VBR must be 512 bytes, got {len(binary)}")

    struct.pack_into("<H", binary, 11, SECTOR_SIZE)
    struct.pack_into("<B", binary, 13, SECTORS_PER_CLUSTER)
    struct.pack_into("<H", binary, 14, RESERVED_SECTORS)
    struct.pack_into("<B", binary, 16, FAT_COUNT)
    struct.pack_into("<H", binary, 17, ROOT_ENTRIES)
    struct.pack_into("<H", binary, 19, PARTITION_SECTORS)
    struct.pack_into("<B", binary, 21, MEDIA_DESCRIPTOR)
    struct.pack_into("<H", binary, 22, sectors_per_fat)
    struct.pack_into("<H", binary, 24, SECTORS_PER_TRACK)
    struct.pack_into("<H", binary, 26, HEADS)
    struct.pack_into("<I", binary, 28, PARTITION_START_LBA)
    struct.pack_into("<I", binary, 32, 0)
    struct.pack_into("<B", binary, 36, 0x80)
    struct.pack_into("<B", binary, 37, 0)
    struct.pack_into("<B", binary, 38, 0x29)
    struct.pack_into("<I", binary, 39, VOLUME_ID)
    binary[43:54] = VOLUME_LABEL.ljust(11, b" ")[:11]
    binary[54:62] = FS_TYPE
    struct.pack_into("<H", binary, 62, stage2_sectors)
    struct.pack_into("<H", binary, 64, 0x0800)


def make_root_entry(name: bytes, start_cluster: int, size_bytes: int) -> bytes:
    if len(name) != 11:
        raise BuildError(f"8.3 name must be 11 bytes, got {name!r}")
    entry = bytearray(32)
    entry[0:11] = name
    entry[11] = 0x20
    struct.pack_into("<H", entry, 26, start_cluster)
    struct.pack_into("<I", entry, 28, size_bytes)
    return bytes(entry)


def build_fat_image(vbr: bytes, stage2: bytes, kernel: bytes) -> None:
    root_dir_sectors = math.ceil(ROOT_ENTRIES * 32 / SECTOR_SIZE)
    sectors_per_fat = compute_sectors_per_fat(PARTITION_SECTORS)
    stage2_sectors = math.ceil(len(stage2) / SECTOR_SIZE)
    if stage2_sectors > RESERVED_SECTORS - 1:
        raise BuildError("Stage-2 loader does not fit in reserved sectors")

    vbr_bytes = bytearray(vbr)
    patch_vbr(vbr_bytes, sectors_per_fat, stage2_sectors)

    files = [
        FilePayload(KERNEL_BIN, b"KERNEL  BIN", kernel),
        FilePayload(CONFIG_DIR / "boot.cfg", b"BOOT    CFG", (CONFIG_DIR / "boot.cfg").read_bytes()),
        FilePayload(ASSET_DIR / "nsigii.txt", b"NSIGII  TXT", (ASSET_DIR / "nsigii.txt").read_bytes()),
    ]

    data_region_sectors = PARTITION_SECTORS - RESERVED_SECTORS - (FAT_COUNT * sectors_per_fat) - root_dir_sectors
    cluster_count = data_region_sectors // SECTORS_PER_CLUSTER
    next_cluster = 2
    fat_entries = [0x0000] * (cluster_count + 2)
    fat_entries[0] = 0xFFF8
    fat_entries[1] = 0xFFFF

    for payload in files:
        cluster_span = max(1, math.ceil(len(payload.data) / (SECTOR_SIZE * SECTORS_PER_CLUSTER)))
        payload.start_cluster = next_cluster
        for idx in range(cluster_span):
            cluster = next_cluster + idx
            fat_entries[cluster] = 0xFFFF if idx == cluster_span - 1 else cluster + 1
        next_cluster += cluster_span

    if next_cluster > len(fat_entries):
        raise BuildError("Payload files do not fit in FAT16 data area")

    fat_bytes = bytearray(sectors_per_fat * SECTOR_SIZE)
    for index, value in enumerate(fat_entries):
        struct.pack_into("<H", fat_bytes, index * 2, value)

    root_dir = bytearray(root_dir_sectors * SECTOR_SIZE)
    offset = 0
    for payload in files:
        entry = make_root_entry(payload.image_name, payload.start_cluster, len(payload.data))
        root_dir[offset:offset + 32] = entry
        offset += 32

    partition = bytearray(PARTITION_SECTORS * SECTOR_SIZE)
    partition[0:SECTOR_SIZE] = vbr_bytes
    partition[SECTOR_SIZE:SECTOR_SIZE + len(stage2)] = stage2

    fat_offset = RESERVED_SECTORS * SECTOR_SIZE
    for fat_index in range(FAT_COUNT):
        start = fat_offset + fat_index * len(fat_bytes)
        partition[start:start + len(fat_bytes)] = fat_bytes

    root_offset = (RESERVED_SECTORS + FAT_COUNT * sectors_per_fat) * SECTOR_SIZE
    partition[root_offset:root_offset + len(root_dir)] = root_dir

    data_offset = (RESERVED_SECTORS + FAT_COUNT * sectors_per_fat + root_dir_sectors) * SECTOR_SIZE
    cluster_size = SECTOR_SIZE * SECTORS_PER_CLUSTER
    for payload in files:
        cluster_index = payload.start_cluster - 2
        start = data_offset + cluster_index * cluster_size
        partition[start:start + len(payload.data)] = payload.data

    mbr_bytes = bytearray(MBR_BIN.read_bytes())
    patch_mbr(mbr_bytes)

    disk = bytearray(DISK_SECTORS * SECTOR_SIZE)
    disk[0:SECTOR_SIZE] = mbr_bytes
    part_start = PARTITION_START_LBA * SECTOR_SIZE
    disk[part_start:part_start + len(partition)] = partition
    DISK_IMG.write_bytes(disk)

    print(f"[IMAGE] Wrote {DISK_IMG.relative_to(REPO_ROOT)}")
    print(f"[IMAGE] Disk size: {len(disk)} bytes")
    print(f"[IMAGE] FAT16 sectors/FAT: {sectors_per_fat}")
    print(f"[IMAGE] Stage2 sectors: {stage2_sectors}")
    for payload in files:
        print(f"[IMAGE] {payload.image_name.decode('ascii').strip()} -> cluster {payload.start_cluster} ({len(payload.data)} bytes)")


def verify_image() -> None:
    blob = DISK_IMG.read_bytes()
    assert len(blob) == DISK_SECTORS * SECTOR_SIZE
    assert blob[510:512] == b"\x55\xAA"

    mbr_entry = blob[446:462]
    assert mbr_entry[0] == 0x80
    assert mbr_entry[4] == PARTITION_TYPE
    start_lba = struct.unpack_from("<I", mbr_entry, 8)[0]
    part_sectors = struct.unpack_from("<I", mbr_entry, 12)[0]
    assert start_lba == PARTITION_START_LBA
    assert part_sectors == PARTITION_SECTORS

    vbr = blob[start_lba * SECTOR_SIZE:(start_lba + 1) * SECTOR_SIZE]
    assert vbr[510:512] == b"\x55\xAA"
    assert struct.unpack_from("<H", vbr, 11)[0] == SECTOR_SIZE
    assert vbr[54:62] == FS_TYPE

    sectors_per_fat = struct.unpack_from("<H", vbr, 22)[0]
    root_entries = struct.unpack_from("<H", vbr, 17)[0]
    root_dir_sectors = math.ceil(root_entries * 32 / SECTOR_SIZE)
    root_lba = start_lba + RESERVED_SECTORS + FAT_COUNT * sectors_per_fat
    root = blob[root_lba * SECTOR_SIZE:(root_lba + root_dir_sectors) * SECTOR_SIZE]
    names = {root[i:i + 11] for i in range(0, len(root), 32) if root[i] not in (0x00, 0xE5)}
    assert {b"KERNEL  BIN", b"BOOT    CFG", b"NSIGII  TXT"}.issubset(names)
    print("[VERIFY] MBR, FAT16 VBR, and root directory entries look valid.")


def main() -> None:
    BUILD_DIR.mkdir(exist_ok=True)
    assemble(BOOT_DIR / "mbr.asm", MBR_BIN)
    assemble(BOOT_DIR / "vbr.asm", VBR_BIN)
    assemble(BOOT_DIR / "stage2.asm", STAGE2_BIN)
    assemble(BOOT_DIR / "kernel.asm", KERNEL_BIN)

    build_fat_image(VBR_BIN.read_bytes(), STAGE2_BIN.read_bytes(), KERNEL_BIN.read_bytes())
    verify_image()


if __name__ == "__main__":
    main()
