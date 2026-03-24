#!/usr/bin/env python3
"""Fail if NASM is not available."""
from __future__ import annotations

import shutil
import sys

if shutil.which("nasm") is None:
    print("[ERROR] nasm not found.")
    print("  WSL/Ubuntu: make install-deps")
    print("  Windows: install nasm or use pre-built boot artifacts")
    raise SystemExit(1)

sys.exit(0)
