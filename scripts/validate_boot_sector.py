#!/usr/bin/env python3
"""Validate boot sector size and signature."""
from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", required=True)
    args = parser.parse_args()

    data = Path(args.path).read_bytes()
    if len(data) != 512:
        raise SystemExit(f"boot sector size mismatch: {len(data)} (expected 512)")

    sig = data[510] | (data[511] << 8)
    if sig != 0xAA55:
        raise SystemExit(f"boot signature mismatch: {hex(sig)} (expected 0xAA55)")

    print(f"[BOOT] {len(data)} bytes sig=0x{sig:04X} OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
