#!/usr/bin/env python3
"""hotswap_sector.py — Hot-swappable boot sector ring management.

Implements the MMUKO-OS ringboot strategy where boot sectors can be
hot-swapped at runtime for interdependency management.

Boot sector ring layout:
    Sector 0:     stage-1 boot (512B, immutable during boot)
    Sectors 1-16: stage-2 kernel payload (mmuko-os.bin, hot-swappable)
    Sectors 17+:  runtime firmware (hot-swappable)

Each sector slot supports:
    - INJECT:  write a new binary payload into the image at a sector offset
    - EXTRACT: pull a sector range out of the image into a standalone .bin
    - SWAP:    atomically replace a sector range (backup → write → verify)
    - VERIFY:  check sector integrity

Usage:
    python3 scripts/hotswap_sector.py inject  --image build/mmuko-os.img --bin build/mmuko-os.bin --sector 1
    python3 scripts/hotswap_sector.py extract --image build/mmuko-os.img --sector 1 --count 16 --output extracted.bin
    python3 scripts/hotswap_sector.py swap    --image build/mmuko-os.img --bin new_kernel.bin --sector 1
    python3 scripts/hotswap_sector.py verify  --image build/mmuko-os.img
"""
from __future__ import annotations

import argparse
import shutil
import struct
import sys
from pathlib import Path

SECTOR_SIZE = 512
FLOPPY_SECTORS = 2880


def inject(image: Path, binary: Path, sector: int) -> int:
    """INJECT a binary payload into the image at a sector offset."""
    img = bytearray(image.read_bytes())
    payload = binary.read_bytes()
    offset = sector * SECTOR_SIZE
    end = offset + len(payload)

    if end > len(img):
        print(f"[ERROR] payload ({len(payload)}B) exceeds image at sector {sector}")
        return 1

    img[offset:end] = payload
    image.write_bytes(img)
    sectors_used = (len(payload) + SECTOR_SIZE - 1) // SECTOR_SIZE
    print(f"[INJECT] {binary.name} → sectors {sector}..{sector + sectors_used - 1} ({len(payload)} bytes)")
    return 0


def extract(image: Path, sector: int, count: int, output: Path) -> int:
    """EXTRACT a sector range from the image into a standalone binary."""
    img = image.read_bytes()
    offset = sector * SECTOR_SIZE
    end = offset + count * SECTOR_SIZE

    if end > len(img):
        print(f"[ERROR] sector range {sector}..{sector + count} exceeds image")
        return 1

    output.write_bytes(img[offset:end])
    print(f"[EXTRACT] sectors {sector}..{sector + count - 1} → {output} ({count * SECTOR_SIZE} bytes)")
    return 0


def swap(image: Path, binary: Path, sector: int) -> int:
    """SWAP: atomically replace a sector range (backup → write → verify)."""
    backup = image.with_suffix(".img.bak")
    shutil.copy2(image, backup)
    print(f"[BACKUP] {image} → {backup}")

    result = inject(image, binary, sector)
    if result != 0:
        shutil.copy2(backup, image)
        print(f"[ROLLBACK] restored from backup")
        return result

    # Verify boot signature is still intact
    img = image.read_bytes()
    sig = struct.unpack_from("<H", img, 510)[0]
    if sig != 0xAA55:
        shutil.copy2(backup, image)
        print(f"[ROLLBACK] boot signature damaged during swap")
        return 1

    print(f"[SWAP] complete, boot signature preserved")
    return 0


def verify(image: Path) -> int:
    """VERIFY: check sector integrity of the disk image."""
    if not image.exists():
        print(f"[ERROR] {image} not found")
        return 1

    img = image.read_bytes()
    total_sectors = len(img) // SECTOR_SIZE

    print(f"[VERIFY] {image}: {len(img)} bytes, {total_sectors} sectors")

    # Check boot signature
    sig = struct.unpack_from("<H", img, 510)[0]
    sig_ok = sig == 0xAA55
    print(f"  Sector 0 (boot):  sig=0x{sig:04X}  {'OK' if sig_ok else 'FAIL'}")

    # Check stage-2 (sector 1): should have non-zero content
    s2_start = SECTOR_SIZE
    s2_data = img[s2_start:s2_start + SECTOR_SIZE]
    s2_nonzero = any(b != 0 for b in s2_data)
    print(f"  Sector 1 (stage2): {'populated' if s2_nonzero else 'empty'}")

    # Count populated sectors
    populated = sum(1 for i in range(total_sectors) if any(b != 0 for b in img[i * SECTOR_SIZE:(i + 1) * SECTOR_SIZE]))
    print(f"  Populated sectors: {populated}/{total_sectors}")

    return 0 if sig_ok else 1


def main() -> int:
    parser = argparse.ArgumentParser(description="MMUKO-OS hot-swappable sector manager")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_inject = sub.add_parser("inject", help="inject binary into image")
    p_inject.add_argument("--image", required=True)
    p_inject.add_argument("--bin", required=True)
    p_inject.add_argument("--sector", type=int, required=True)

    p_extract = sub.add_parser("extract", help="extract sectors from image")
    p_extract.add_argument("--image", required=True)
    p_extract.add_argument("--sector", type=int, required=True)
    p_extract.add_argument("--count", type=int, required=True)
    p_extract.add_argument("--output", required=True)

    p_swap = sub.add_parser("swap", help="atomically swap sectors")
    p_swap.add_argument("--image", required=True)
    p_swap.add_argument("--bin", required=True)
    p_swap.add_argument("--sector", type=int, required=True)

    p_verify = sub.add_parser("verify", help="verify image integrity")
    p_verify.add_argument("--image", required=True)

    args = parser.parse_args()

    if args.cmd == "inject":
        return inject(Path(args.image), Path(args.bin), args.sector)
    elif args.cmd == "extract":
        return extract(Path(args.image), args.sector, args.count, Path(args.output))
    elif args.cmd == "swap":
        return swap(Path(args.image), Path(args.bin), args.sector)
    elif args.cmd == "verify":
        return verify(Path(args.image))
    return 1


if __name__ == "__main__":
    sys.exit(main())
