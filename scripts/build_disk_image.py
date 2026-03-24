#!/usr/bin/env python3
"""Create mmuko-os raw image with stage-1 and stage-2 payloads."""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description="Build raw floppy image.")
    parser.add_argument("--stage1", required=True)
    parser.add_argument("--stage2", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--sectors", type=int, default=2880)
    args = parser.parse_args()

    stage1 = Path(args.stage1).read_bytes()
    stage2 = Path(args.stage2).read_bytes()

    image = bytearray(b"\0" * (512 * args.sectors))
    image[:512] = stage1
    image[512 : 512 + len(stage2)] = stage2
    Path(args.output).write_bytes(image)

    print(
        f"[IMAGE] wrote {len(image)} bytes with stage2 payload {len(stage2)} bytes -> {args.output}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
