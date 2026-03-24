#!/usr/bin/env python3
"""cleanup.py — Lightweight codebase cleanup and dependency pruning.

Scans the MMUKO-OS project tree and removes:
  - Stale build artifacts
  - Duplicate/orphan object files
  - Empty pseudocode files
  - Windows path references in generated files
  - __pycache__ and .egg-info directories

Usage:
    python3 scripts/cleanup.py [--root .] [--dry-run]
"""
from __future__ import annotations

import argparse
import shutil
import sys
from pathlib import Path


def find_stale_artifacts(root: Path) -> list[Path]:
    """Find stale build artifacts that can be safely removed."""
    stale: list[Path] = []
    patterns = [
        "**/__pycache__",
        "**/*.egg-info",
        "**/*.pyc",
        "**/obj/**/*.o",
        "legacy/csharp-compositor/bin",
        "legacy/csharp-compositor/obj",
    ]
    for pat in patterns:
        stale.extend(root.glob(pat))
    return stale


def find_empty_psc(root: Path) -> list[Path]:
    """Find empty .psc pseudocode files."""
    psc_dir = root / "pseudocode"
    if not psc_dir.exists():
        return []
    return [p for p in psc_dir.glob("*.psc") if p.stat().st_size == 0]


def find_duplicate_binaries(root: Path) -> list[Path]:
    """Find duplicate .exe and binary files that shouldn't be in source."""
    dupes: list[Path] = []
    for pattern in ["**/*.exe", "**/*.dll"]:
        dupes.extend(root.glob(pattern))
    return dupes


def fix_windows_paths(root: Path, dry_run: bool) -> int:
    """Replace absolute Windows paths with relative paths in generated files."""
    count = 0
    for ext in ["*.c", "*.h", "*.cpp", "*.asm", "*.pxd", "*.pyx"]:
        for fpath in root.rglob(ext):
            try:
                text = fpath.read_text(encoding="utf-8", errors="replace")
            except Exception:
                continue
            if "C:/Users/" in text or "C:\\Users\\" in text:
                if dry_run:
                    print(f"  [DRY] Would fix Windows paths in {fpath}")
                else:
                    import re
                    text = re.sub(
                        r"C:[/\\]Users[/\\]\w+[/\\]Projects[/\\]mmuko-os[/\\]?",
                        "",
                        text,
                    )
                    fpath.write_text(text, encoding="utf-8")
                    print(f"  [FIX] Cleaned Windows paths in {fpath}")
                count += 1
    return count


def report_sizes(root: Path) -> None:
    """Report file sizes for the main source files."""
    print("\n--- Source file sizes ---")
    for ext in ["*.c", "*.h", "*.cpp", "*.asm", "*.py", "*.psc"]:
        files = list(root.rglob(ext))
        if not files:
            continue
        total = sum(f.stat().st_size for f in files)
        print(f"  {ext:>8}: {len(files):>3} files, {total:>10,} bytes")


def main() -> int:
    parser = argparse.ArgumentParser(description="MMUKO-OS cleanup")
    parser.add_argument("--root", default=".", help="project root")
    parser.add_argument("--dry-run", action="store_true", help="show what would be done")
    args = parser.parse_args()
    root = Path(args.root)

    print("MMUKO-OS Codebase Cleanup")
    print("=" * 40)

    # 1. Stale artifacts
    stale = find_stale_artifacts(root)
    print(f"\n[1] Stale artifacts: {len(stale)}")
    for s in stale:
        if args.dry_run:
            print(f"  [DRY] Would remove {s}")
        else:
            if s.is_dir():
                shutil.rmtree(s, ignore_errors=True)
            else:
                s.unlink(missing_ok=True)
            print(f"  [DEL] {s}")

    # 2. Empty .psc files
    empty = find_empty_psc(root)
    print(f"\n[2] Empty .psc files: {len(empty)}")
    for e in empty:
        if args.dry_run:
            print(f"  [DRY] Would flag {e}")
        else:
            print(f"  [WARN] {e.name} is empty — consider populating or removing")

    # 3. Duplicate binaries
    dupes = find_duplicate_binaries(root)
    print(f"\n[3] Binary files in source: {len(dupes)}")
    for d in dupes:
        print(f"  [WARN] {d} ({d.stat().st_size:,} bytes)")

    # 4. Windows path cleanup
    print(f"\n[4] Windows path references")
    count = fix_windows_paths(root, args.dry_run)
    print(f"  {count} file(s) {'would be' if args.dry_run else ''} cleaned")

    # 5. Size report
    report_sizes(root)

    print("\n[DONE]")
    return 0


if __name__ == "__main__":
    sys.exit(main())
