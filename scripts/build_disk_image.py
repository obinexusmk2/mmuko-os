#!/usr/bin/env python3
"""Build a 1.44MB disk image with stage-1 and stage-2 payloads."""
from __future__ import annotations

import argparse
from pathlib import Path

SECTOR_SIZE = 512
FLOPPY_SECTORS = 2880


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--stage1", required=True)
    parser.add_argument("--stage2", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    stage1 = Path(args.stage1).read_bytes()
    stage2 = Path(args.stage2).read_bytes()

    image = bytearray(b"\0" * (SECTOR_SIZE * FLOPPY_SECTORS))
    image[:SECTOR_SIZE] = stage1
    image[SECTOR_SIZE : SECTOR_SIZE + len(stage2)] = stage2
    Path(args.output).write_bytes(image)

    print(
        f"[IMAGE] wrote {len(image)} bytes with stage2 payload {len(stage2)} bytes"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
