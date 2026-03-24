#!/usr/bin/env python3
"""build_image.py — Assemble MMUKO-OS disk image from boot chain binaries.

Combines stage-1 (boot sector), stage-2 (mmuko-os kernel), and runtime
(firmware entry) into a 1.44 MB FAT12 floppy image.

Usage:
    python3 scripts/build_image.py [--stage1 build/boot.bin]
                                   [--stage2 build/mmuko-os.bin]
                                   [--runtime build/runtime.bin]
                                   [--output build/mmuko-os.img]

Enzyme model: BUILD phase — assembles sectors into bootable image.
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

FLOPPY_SIZE = 512 * 2880  # 1.44 MB
BOOT_SIG = 0xAA55
SECTOR_SIZE = 512


def validate_stage1(data: bytes) -> bool:
    """Verify boot sector: 512 bytes with 0xAA55 signature."""
    if len(data) != SECTOR_SIZE:
        print(f"[ERROR] stage1 size={len(data)}, expected {SECTOR_SIZE}")
        return False
    sig = struct.unpack_from("<H", data, 510)[0]
    if sig != BOOT_SIG:
        print(f"[ERROR] stage1 sig=0x{sig:04X}, expected 0xAA55")
        return False
    return True


def build_image(stage1: Path, stage2: Path, runtime: Path, output: Path) -> int:
    """BUILD: assemble boot chain into disk image."""
    s1 = stage1.read_bytes()
    s2 = stage2.read_bytes()
    rt = runtime.read_bytes()

    if not validate_stage1(s1):
        return 1

    img = bytearray(b"\x00" * FLOPPY_SIZE)

    # Sector 0: boot sector (stage-1)
    img[:SECTOR_SIZE] = s1

    # Sectors 1..N: stage-2 kernel (mmuko-os.bin)
    offset = SECTOR_SIZE
    img[offset : offset + len(s2)] = s2
    s2_end = offset + len(s2)

    # After stage-2: runtime firmware entry
    img[s2_end : s2_end + len(rt)] = rt

    output.write_bytes(img)
    print(f"[IMAGE] wrote {len(img)} bytes")
    print(f"  stage1:  {len(s1):>6} bytes (sector 0)")
    print(f"  stage2:  {len(s2):>6} bytes (mmuko-os kernel)")
    print(f"  runtime: {len(rt):>6} bytes (firmware entry)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="MMUKO-OS image builder")
    parser.add_argument("--stage1", default="build/boot.bin")
    parser.add_argument("--stage2", default="build/mmuko-os.bin")
    parser.add_argument("--runtime", default="build/runtime.bin")
    parser.add_argument("--output", default="build/mmuko-os.img")
    args = parser.parse_args()

    for name, path in [("stage1", args.stage1), ("stage2", args.stage2), ("runtime", args.runtime)]:
        if not Path(path).exists():
            print(f"[ERROR] {name} not found: {path}")
            return 1

    return build_image(Path(args.stage1), Path(args.stage2), Path(args.runtime), Path(args.output))


if __name__ == "__main__":
    sys.exit(main())
