#!/usr/bin/env python3
"""Create directories if missing."""
from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("dirs", nargs="+")
    args = parser.parse_args()

    for entry in args.dirs:
        Path(entry).mkdir(parents=True, exist_ok=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
