#!/usr/bin/env python3
"""tree_display.py — Display MMUKO-OS filesystem driver tree.

Shows the root/trunk/branch/leaves hierarchy for the MMUKO-OS distro,
mapping boot chain components and drivers to a visual tree structure.

Usage:
    python3 scripts/tree_display.py [--root .]
"""
from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path

# ANSI colors
CYAN = "\033[96m"
GREEN = "\033[92m"
YELLOW = "\033[93m"
RED = "\033[91m"
BOLD = "\033[1m"
DIM = "\033[2m"
RESET = "\033[0m"

# Tree characters
PIPE = "│"
BRANCH = "├──"
LAST = "└──"
INDENT = "    "


def _tree_line(prefix: str, connector: str, label: str, color: str = "") -> str:
    return f"{prefix}{connector} {color}{label}{RESET}"


def display_driver_tree(root: Path) -> None:
    """Display the MMUKO-OS filesystem tree in root/trunk/branch/leaves format."""
    now = time.strftime("%Y-%m-%d %H:%M:%S")
    print(f"{BOLD}{CYAN}MMUKO-OS Filesystem Driver Tree{RESET}")
    print(f"{DIM}Generated: {now}{RESET}")
    print()

    # Root level
    print(f"{BOLD}{GREEN}root/{RESET}  {DIM}(mmuko-os distro){RESET}")

    # Trunk: boot chain
    print(f"{PIPE}")
    print(f"{BRANCH} {YELLOW}trunk/{RESET}  {DIM}(boot chain){RESET}")
    boot_dir = root / "boot"
    build_dir = root / "build"

    # Boot chain branches
    boot_files = [
        ("stage1", "mmuko_stage1_boot.asm", "BIOS boot sector (512B, 0xAA55)"),
        ("stage2", "stage2.asm", "mmuko-os kernel loader (NSIGII handoff)"),
        ("contract", "contract.inc", "boot contract constants"),
    ]
    for i, (name, fname, desc) in enumerate(boot_files):
        is_last = i == len(boot_files) - 1
        conn = LAST if is_last else BRANCH
        exists = (boot_dir / fname).exists()
        status = f"{GREEN}OK{RESET}" if exists else f"{RED}MISSING{RESET}"
        prefix = f"{PIPE}   "
        print(f"{prefix}{conn} {CYAN}branch/{RESET} {name}: {fname}  [{status}]")
        if not is_last:
            print(f"{prefix}{PIPE}   {LAST} {DIM}leaves/{RESET} {desc}")
        else:
            print(f"{prefix}    {LAST} {DIM}leaves/{RESET} {desc}")

    # Trunk: kernel / firmware
    print(f"{PIPE}")
    print(f"{BRANCH} {YELLOW}trunk/{RESET}  {DIM}(kernel firmware){RESET}")
    kernel_dir = root / "kernel"
    kernel_files = [
        ("runtime", "runtime.asm", "firmware entry point (16-bit real mode)"),
        ("stage2_loader", "mmuko_stage2_loader.c", "C stage-2 phase descriptor"),
        ("stage2_bridge", "mmuko_stage2_bridge.cpp", "C++ bridge wrapper"),
    ]
    for i, (name, fname, desc) in enumerate(kernel_files):
        is_last = i == len(kernel_files) - 1
        conn = LAST if is_last else BRANCH
        exists = (kernel_dir / fname).exists()
        status = f"{GREEN}OK{RESET}" if exists else f"{RED}MISSING{RESET}"
        prefix = f"{PIPE}   "
        print(f"{prefix}{conn} {CYAN}branch/{RESET} {name}: {fname}  [{status}]")
        leaf_prefix = f"{prefix}{'    ' if is_last else PIPE + '   '}"
        print(f"{leaf_prefix}{LAST} {DIM}leaves/{RESET} {desc}")

    # Trunk: drivers (firmware C code)
    print(f"{PIPE}")
    print(f"{BRANCH} {YELLOW}trunk/{RESET}  {DIM}(drivers){RESET}")
    drivers = [
        ("heartfull_membrane", "heartfull_membrane.c", "NSIGII 6-phase membrane calibrator"),
        ("bzy_mpda", "bzy_mpda.c", "BZY multi-perspective drift analyzer"),
        ("tripartite_disc", "tripartite_discriminant.c", "tripartite discriminant evaluator"),
        ("nsigii_cpp", "nsigii_cpp_wrapper.cpp", "C++ RAII wrapper for NSIGII ABI"),
    ]
    for i, (name, fname, desc) in enumerate(drivers):
        is_last = i == len(drivers) - 1
        conn = LAST if is_last else BRANCH
        exists = (root / fname).exists()
        status = f"{GREEN}OK{RESET}" if exists else f"{RED}MISSING{RESET}"
        prefix = f"{PIPE}   "
        print(f"{prefix}{conn} {CYAN}branch/{RESET} {name}: {fname}  [{status}]")
        leaf_prefix = f"{prefix}{'    ' if is_last else PIPE + '   '}"
        print(f"{leaf_prefix}{LAST} {DIM}leaves/{RESET} {desc}")

    # Trunk: pseudocode (NSIGII protocol)
    print(f"{PIPE}")
    print(f"{BRANCH} {YELLOW}trunk/{RESET}  {DIM}(NSIGII pseudocode){RESET}")
    psc_dir = root / "pseudocode"
    psc_files = sorted(psc_dir.glob("*.psc")) if psc_dir.exists() else []
    for i, psc in enumerate(psc_files):
        is_last = i == len(psc_files) - 1
        conn = LAST if is_last else BRANCH
        prefix = f"{PIPE}   "
        size = psc.stat().st_size
        print(f"{prefix}{conn} {DIM}leaves/{RESET} {psc.name}  ({size:,} bytes)")

    # Trunk: build artifacts
    print(f"{PIPE}")
    print(f"{LAST} {YELLOW}trunk/{RESET}  {DIM}(build output){RESET}")
    artifacts = [
        ("boot.bin", "stage-1 binary (512B)"),
        ("mmuko-os.bin", "stage-2 kernel binary"),
        ("runtime.bin", "firmware runtime binary"),
        ("mmuko-os.img", "bootable disk image (1.44 MB)"),
    ]
    for i, (fname, desc) in enumerate(artifacts):
        is_last = i == len(artifacts) - 1
        conn = LAST if is_last else BRANCH
        exists = (build_dir / fname).exists()
        if exists:
            size = (build_dir / fname).stat().st_size
            status = f"{GREEN}{size:,}B{RESET}"
        else:
            status = f"{RED}not built{RESET}"
        prefix = "    "
        print(f"{prefix}{conn} {CYAN}branch/{RESET} {fname}: {desc}  [{status}]")

    print()
    print(f"{DIM}Enzyme lifecycle: CREATE/DESTROY | BUILD/BREAK | REPAIR/RENEW{RESET}")


def main() -> int:
    parser = argparse.ArgumentParser(description="MMUKO-OS filesystem tree")
    parser.add_argument("--root", default=".", help="project root directory")
    args = parser.parse_args()
    display_driver_tree(Path(args.root))
    return 0


if __name__ == "__main__":
    sys.exit(main())
