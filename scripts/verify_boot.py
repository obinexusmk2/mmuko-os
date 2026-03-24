#!/usr/bin/env python3
"""verify_boot.py — NSIGII boot chain verification with enzyme model diagnostics.

Checks all boot artifacts for correctness and reports enzyme state
(CREATE/DESTROY, BUILD/BREAK, REPAIR/RENEW) for each component.

Usage:
    python3 scripts/verify_boot.py [--build-dir build]
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

# Enzyme trinary states
YES = 1
NO = 0
MAYBE = -1
MAYBE_NOT = -2

# Enzyme lifecycle pairs
ENZYME_PAIRS = [
    ("CREATE", "DESTROY"),
    ("BUILD", "BREAK"),
    ("REPAIR", "RENEW"),
]


def enzyme_label(state: int) -> str:
    """Map trinary state to human-readable label."""
    return {YES: "YES", NO: "NO", MAYBE: "MAYBE", MAYBE_NOT: "MAYBE_NOT"}.get(state, "UNKNOWN")


def check_stage1(path: Path) -> int:
    """Verify stage-1 boot sector."""
    if not path.exists():
        print(f"  [FAIL] {path} not found")
        return NO
    data = path.read_bytes()
    if len(data) != 512:
        print(f"  [FAIL] stage1 size={len(data)}, expected 512")
        return NO
    sig = struct.unpack_from("<H", data, 510)[0]
    if sig != 0xAA55:
        print(f"  [FAIL] stage1 sig=0x{sig:04X}, expected 0xAA55")
        return NO
    print(f"  [PASS] stage1: 512 bytes, sig=0xAA55")
    return YES


def check_stage2(path: Path) -> int:
    """Verify stage-2 (mmuko-os kernel)."""
    if not path.exists():
        print(f"  [FAIL] {path} not found")
        return NO
    data = path.read_bytes()
    if len(data) == 0:
        print(f"  [FAIL] stage2 is empty")
        return NO
    print(f"  [PASS] stage2 (mmuko-os): {len(data)} bytes")
    return YES


def check_runtime(path: Path) -> int:
    """Verify runtime firmware entry."""
    if not path.exists():
        print(f"  [MAYBE] runtime not found (optional)")
        return MAYBE
    data = path.read_bytes()
    print(f"  [PASS] runtime: {len(data)} bytes")
    return YES


def check_image(path: Path) -> int:
    """Verify disk image."""
    if not path.exists():
        print(f"  [FAIL] {path} not found")
        return NO
    data = path.read_bytes()
    expected = 512 * 2880
    if len(data) != expected:
        print(f"  [FAIL] image size={len(data)}, expected {expected}")
        return NO
    sig = struct.unpack_from("<H", data, 510)[0]
    if sig != 0xAA55:
        print(f"  [FAIL] image boot sig=0x{sig:04X}")
        return NO
    print(f"  [PASS] image: {len(data)} bytes (1.44 MB), boot sig OK")
    return YES


def trinary_compose(a: int, b: int) -> int:
    """NSIGII trinary composition."""
    if a == MAYBE_NOT or b == MAYBE_NOT:
        return MAYBE_NOT
    if a == NO or b == NO:
        return NO
    if a == YES and b == YES:
        return YES
    if a == MAYBE and b == MAYBE:
        return YES  # double negation resolves
    return MAYBE


def main() -> int:
    parser = argparse.ArgumentParser(description="NSIGII boot verification")
    parser.add_argument("--build-dir", default="build")
    args = parser.parse_args()
    bd = Path(args.build_dir)

    print("=" * 50)
    print(" NSIGII Boot Chain Verification")
    print(" Enzyme Model: CREATE/DESTROY | BUILD/BREAK | REPAIR/RENEW")
    print("=" * 50)
    print()

    results = {}
    print("[1] Stage-1 boot sector")
    results["stage1"] = check_stage1(bd / "boot.bin")

    print("[2] Stage-2 kernel (mmuko-os.bin)")
    results["stage2"] = check_stage2(bd / "mmuko-os.bin")

    print("[3] Runtime firmware entry")
    results["runtime"] = check_runtime(bd / "runtime.bin")

    print("[4] Disk image")
    results["image"] = check_image(bd / "mmuko-os.img")

    # Enzyme membrane evaluation
    print()
    print("--- Enzyme Membrane ---")
    membrane = YES
    for name, state in results.items():
        membrane = trinary_compose(membrane, state)
        action_pair = ENZYME_PAIRS[0] if state == YES else ENZYME_PAIRS[2] if state == MAYBE else ENZYME_PAIRS[1]
        print(f"  {name:>10}: {enzyme_label(state):>10}  → {action_pair[0]}/{action_pair[1]}")

    print()
    outcome = "PASS" if membrane == YES else "HOLD" if membrane == MAYBE else "ALERT"
    print(f"  Membrane outcome: {outcome} ({enzyme_label(membrane)})")

    print()
    print("[5] Trinary alphabet")
    for name, val in [("YES", 1), ("NO", 0), ("MAYBE", -1), ("MAYBE_NOT", -2)]:
        print(f"    {name} = {val}")
    print()
    print("=== NSIGII_VERIFIED ===" if membrane >= MAYBE else "=== NSIGII_ALERT ===")
    return 0 if membrane >= MAYBE else 1


if __name__ == "__main__":
    sys.exit(main())
