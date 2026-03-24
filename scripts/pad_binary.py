#!/usr/bin/env python3
"""Pad a binary file to a fixed size with 0x00 bytes."""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser(description="Pad binary file.")
    parser.add_argument("path")
    parser.add_argument("--size", type=int, required=True)
    args = parser.parse_args()

    data = Path(args.path).read_bytes()
    if len(data) > args.size:
        raise SystemExit(f"[ERROR] {args.path} is {len(data)} bytes, exceeds {args.size}")

    padded = data + (b"\0" * (args.size - len(data)))
    Path(args.path).write_bytes(padded)
    print(f"[PAD] {args.path}: {len(data)} -> {len(padded)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
