#!/usr/bin/env python3
"""enzyme_panic.py — Enzyme model kernel panic strategy for MMUKO-OS.

Implements the trinary enzyme lifecycle for kernel panic recovery:
    MAYBE → YES / NO
    CREATE / DESTROY   (phase initialization / teardown)
    BUILD  / BREAK     (sector assembly / disassembly)
    REPAIR / RENEW     (panic recovery / hot-reload of boot sectors)

This module provides reusable functions for:
  - Evaluating enzyme state from boot chain health
  - Triggering recovery actions (REPAIR/RENEW) on panic
  - Hot-swapping boot sectors via ringboot strategy

Usage:
    python3 scripts/enzyme_panic.py [--check] [--recover] [--build-dir build]
"""
from __future__ import annotations

import argparse
import shutil
import struct
import sys
import time
from dataclasses import dataclass, field
from enum import IntEnum
from pathlib import Path
from typing import Callable


class Trinary(IntEnum):
    """NSIGII trinary state values."""
    MAYBE_NOT = -2
    MAYBE = -1
    NO = 0
    YES = 1


class EnzymeAction(IntEnum):
    """Enzyme lifecycle actions."""
    CREATE = 1
    DESTROY = 2
    BUILD = 3
    BREAK = 4
    REPAIR = 5
    RENEW = 6


@dataclass
class EnzymeState:
    """Enzyme state for a single boot component."""
    name: str
    state: Trinary = Trinary.MAYBE
    action: EnzymeAction = EnzymeAction.CREATE
    timestamp: float = field(default_factory=time.time)
    error: str = ""

    @property
    def pair(self) -> tuple[EnzymeAction, EnzymeAction]:
        """Return the enzyme action pair for current state."""
        if self.state == Trinary.YES:
            return (EnzymeAction.CREATE, EnzymeAction.DESTROY)
        elif self.state == Trinary.NO:
            return (EnzymeAction.BUILD, EnzymeAction.BREAK)
        else:
            return (EnzymeAction.REPAIR, EnzymeAction.RENEW)


def trinary_compose(a: Trinary, b: Trinary) -> Trinary:
    """NSIGII trinary composition (RIFT table)."""
    if a == Trinary.MAYBE_NOT or b == Trinary.MAYBE_NOT:
        return Trinary.MAYBE_NOT
    if a == Trinary.NO or b == Trinary.NO:
        return Trinary.NO
    if a == Trinary.YES and b == Trinary.YES:
        return Trinary.YES
    if a == Trinary.MAYBE and b == Trinary.MAYBE:
        return Trinary.YES  # double negation resolves
    return Trinary.MAYBE


def check_component(name: str, path: Path, validator: Callable[[bytes], bool] | None = None) -> EnzymeState:
    """Check a boot component and return its enzyme state."""
    es = EnzymeState(name=name)
    if not path.exists():
        es.state = Trinary.NO
        es.action = EnzymeAction.BUILD
        es.error = f"{path} not found"
        return es

    data = path.read_bytes()
    if len(data) == 0:
        es.state = Trinary.NO
        es.action = EnzymeAction.BUILD
        es.error = f"{path} is empty"
        return es

    if validator and not validator(data):
        es.state = Trinary.MAYBE
        es.action = EnzymeAction.REPAIR
        es.error = f"{path} failed validation"
        return es

    es.state = Trinary.YES
    es.action = EnzymeAction.CREATE
    return es


def validate_boot_sector(data: bytes) -> bool:
    """Validate 512-byte boot sector with 0xAA55 signature."""
    if len(data) != 512:
        return False
    sig = struct.unpack_from("<H", data, 510)[0]
    return sig == 0xAA55


def validate_image(data: bytes) -> bool:
    """Validate 1.44 MB disk image with boot signature."""
    if len(data) != 512 * 2880:
        return False
    sig = struct.unpack_from("<H", data, 510)[0]
    return sig == 0xAA55


def recover_component(es: EnzymeState, build_dir: Path) -> EnzymeState:
    """REPAIR/RENEW: attempt recovery for a failed component.

    Strategy:
    - If component is BROKEN (NO): try to REBUILD from source
    - If component is MAYBE: try to REPAIR from backup
    - If component is YES: no action needed (CREATE state)
    """
    if es.state == Trinary.YES:
        return es

    backup_dir = build_dir / "backup"
    backup_dir.mkdir(parents=True, exist_ok=True)

    target = build_dir / {
        "stage1": "boot.bin",
        "stage2": "mmuko-os.bin",
        "runtime": "runtime.bin",
        "image": "mmuko-os.img",
    }.get(es.name, es.name)

    backup = backup_dir / target.name
    if backup.exists():
        print(f"  [RENEW] Restoring {es.name} from backup...")
        shutil.copy2(backup, target)
        es.state = Trinary.MAYBE
        es.action = EnzymeAction.RENEW
    else:
        print(f"  [REPAIR] No backup for {es.name} — needs rebuild (make boot)")
        es.action = EnzymeAction.REPAIR

    return es


def backup_component(name: str, build_dir: Path) -> None:
    """Create a backup of a healthy component for future RENEW."""
    target = build_dir / {
        "stage1": "boot.bin",
        "stage2": "mmuko-os.bin",
        "runtime": "runtime.bin",
        "image": "mmuko-os.img",
    }.get(name, name)

    if target.exists():
        backup_dir = build_dir / "backup"
        backup_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(target, backup_dir / target.name)


def run_check(build_dir: Path) -> list[EnzymeState]:
    """Check all boot components and return enzyme states."""
    components = [
        ("stage1", build_dir / "boot.bin", validate_boot_sector),
        ("stage2", build_dir / "mmuko-os.bin", None),
        ("runtime", build_dir / "runtime.bin", None),
        ("image", build_dir / "mmuko-os.img", validate_image),
    ]

    states = []
    for name, path, validator in components:
        es = check_component(name, path, validator)
        states.append(es)

    return states


def print_enzyme_report(states: list[EnzymeState]) -> Trinary:
    """Print enzyme state report and return membrane outcome."""
    print("=" * 55)
    print(" MMUKO-OS Enzyme Kernel Panic Strategy")
    print(" MAYBE → YES/NO | CREATE/DESTROY | BUILD/BREAK | REPAIR/RENEW")
    print("=" * 55)
    print()

    membrane = Trinary.YES
    for es in states:
        pair = es.pair
        status = "PASS" if es.state == Trinary.YES else "HOLD" if es.state == Trinary.MAYBE else "FAIL"
        print(f"  {es.name:>10}: {es.state.name:>10}  {status:>4}  → {pair[0].name}/{pair[1].name}")
        if es.error:
            print(f"             {es.error}")
        membrane = trinary_compose(membrane, es.state)

        # Backup healthy components
        if es.state == Trinary.YES:
            backup_component(es.name, es.timestamp and Path("build") or Path("build"))

    print()
    outcome = "PASS" if membrane == Trinary.YES else "HOLD" if membrane == Trinary.MAYBE else "PANIC"
    print(f"  Membrane: {outcome} ({membrane.name})")
    if membrane <= Trinary.NO:
        print(f"  Action:   REPAIR/RENEW cycle required")
    print()
    return membrane


def main() -> int:
    parser = argparse.ArgumentParser(description="MMUKO-OS enzyme panic strategy")
    parser.add_argument("--check", action="store_true", help="check boot chain health")
    parser.add_argument("--recover", action="store_true", help="attempt recovery")
    parser.add_argument("--build-dir", default="build")
    args = parser.parse_args()

    build_dir = Path(args.build_dir)

    if not args.check and not args.recover:
        args.check = True

    states = run_check(build_dir)
    membrane = print_enzyme_report(states)

    if args.recover and membrane < Trinary.YES:
        print("--- Recovery ---")
        for es in states:
            if es.state < Trinary.YES:
                recover_component(es, build_dir)
        print()
        # Re-check
        states = run_check(build_dir)
        membrane = print_enzyme_report(states)

    return 0 if membrane >= Trinary.MAYBE else 1


if __name__ == "__main__":
    sys.exit(main())
