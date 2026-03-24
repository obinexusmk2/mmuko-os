#!/usr/bin/env python3
"""Reusable helper to ensure required build tools are available."""

from __future__ import annotations

import argparse
import shutil
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description="Check if a tool exists on PATH.")
    parser.add_argument("tool", help="Tool binary name, e.g. nasm")
    parser.add_argument(
        "--hint",
        default="",
        help="Optional hint shown when the tool is missing.",
    )
    args = parser.parse_args()

    tool_path = shutil.which(args.tool)
    if tool_path:
        print(f"[CHECK] {args.tool}: {tool_path}")
        return 0

    print(f"[ERROR] Required tool not found: {args.tool}")
    if args.hint:
        print(args.hint)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
