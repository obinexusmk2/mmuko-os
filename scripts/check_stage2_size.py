#!/usr/bin/env python3
"""Validate stage-2 payload size."""
from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--path", required=True)
    parser.add_argument("--expected", type=int, required=True)
    args = parser.parse_args()

    size = len(Path(args.path).read_bytes())
    if size != args.expected:
        raise SystemExit(f"stage-2 payload size mismatch: {size} (expected {args.expected})")

    print(f"[STAGE2] {size} bytes OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
