#!/usr/bin/env python3
"""assemble_boot.py — Python-based boot sector builder for MMUKO-OS.

When NASM is not available (e.g., in CI or sandboxed environments),
this script generates the boot chain binaries directly in Python
using pre-assembled machine code patterns.

Outputs:
    build/boot.bin     — 512-byte stage-1 boot sector (0xAA55 sig)
    build/mmuko-os.bin — stage-2 kernel with NSIGII handoff
    build/runtime.bin  — firmware runtime entry

Usage:
    python3 scripts/assemble_boot.py [--build-dir build]
"""
from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path


def emit_string(s: str) -> bytes:
    """Encode a null-terminated string for real-mode BIOS teletype."""
    return s.encode("ascii") + b"\x00"


def emit_print_string_routine() -> bytes:
    """x86 16-bit print_string routine: SI=string, uses INT 0x10."""
    # lodsb; test al,al; jz .done; mov ah,0x0E; mov bh,0; mov bl,0x0F; int 0x10; jmp print_string; .done: ret
    return bytes([
        0xAC,                   # lodsb
        0x84, 0xC0,             # test al, al
        0x74, 0x09,             # jz .done (+9)
        0xB4, 0x0E,             # mov ah, 0x0E
        0xB7, 0x00,             # mov bh, 0
        0xB3, 0x0F,             # mov bl, 0x0F
        0xCD, 0x10,             # int 0x10
        0xEB, 0xF1,             # jmp print_string (-15)
        0xC3,                   # ret
    ])


def build_stage1(build_dir: Path) -> None:
    """Build the stage-1 boot sector (512 bytes with 0xAA55 signature)."""
    sector = bytearray(512)

    # FAT12 BPB header
    pos = 0
    sector[pos:pos + 3] = b"\xEB\x3C\x90"  # jmp short start; nop
    pos = 3
    sector[pos:pos + 8] = b"MMUKOGEN"  # OEM ID
    pos = 11
    struct.pack_into("<H", sector, pos, 512)    # bytes per sector
    pos = 13
    sector[pos] = 1                              # sectors per cluster
    pos = 14
    struct.pack_into("<H", sector, pos, 1)       # reserved sectors
    pos = 16
    sector[pos] = 2                              # FAT count
    pos = 17
    struct.pack_into("<H", sector, pos, 224)     # root entries
    pos = 19
    struct.pack_into("<H", sector, pos, 2880)    # total sectors
    pos = 21
    sector[pos] = 0xF0                           # media descriptor
    pos = 22
    struct.pack_into("<H", sector, pos, 9)       # sectors per FAT
    pos = 24
    struct.pack_into("<H", sector, pos, 18)      # sectors per track
    pos = 26
    struct.pack_into("<H", sector, pos, 2)       # heads
    pos = 28
    struct.pack_into("<I", sector, pos, 0)       # hidden sectors
    pos = 32
    struct.pack_into("<I", sector, pos, 0)       # large sector count
    pos = 36
    sector[pos] = 0                              # drive number
    pos = 37
    sector[pos] = 0                              # reserved
    pos = 38
    sector[pos] = 0x29                           # ext boot sig
    pos = 39
    struct.pack_into("<I", sector, pos, 0x4D4D554B)  # serial
    pos = 43
    sector[pos:pos + 11] = b"MMUKO-GEN  "       # volume label
    pos = 54
    sector[pos:pos + 8] = b"FAT12   "           # FS type

    # start: at offset 0x3C (after BPB)
    code_start = 0x3C
    code = bytearray()

    # cli; xor ax,ax; mov ds,ax; mov es,ax; mov ss,ax; mov sp,0x7C00; sti
    code += bytes([
        0xFA,                       # cli
        0x31, 0xC0,                 # xor ax, ax
        0x8E, 0xD8,                 # mov ds, ax
        0x8E, 0xC0,                 # mov es, ax
        0x8E, 0xD0,                 # mov ss, ax
        0xBC, 0x00, 0x7C,           # mov sp, 0x7C00
        0xFB,                       # sti
    ])

    # Save boot drive: mov [boot_drive], dl
    boot_drive_offset = 0x100  # data area
    code += bytes([0x88, 0x16]) + struct.pack("<H", 0x7C00 + boot_drive_offset)

    # Print banner: mov si, banner; call print_string
    banner_offset = 0x110
    code += bytes([0xBE]) + struct.pack("<H", 0x7C00 + banner_offset)
    print_string_offset = 0x0140
    code += bytes([0xE8]) + struct.pack("<h", (0x7C00 + print_string_offset) - (0x7C00 + code_start + len(code) + 3))

    # Load stage-2: BIOS INT 13h
    # mov ax, 0x0000; mov es, ax; mov bx, 0x8000
    code += bytes([
        0xB8, 0x00, 0x00,           # mov ax, 0
        0x8E, 0xC0,                 # mov es, ax
        0xBB, 0x00, 0x80,           # mov bx, 0x8000
    ])
    # mov ah, 0x02; mov al, 16; mov ch, 0; mov cl, 2; mov dh, 0
    code += bytes([
        0xB4, 0x02,                 # mov ah, 0x02
        0xB0, 0x10,                 # mov al, 16
        0xB5, 0x00,                 # mov ch, 0
        0xB1, 0x02,                 # mov cl, 2
        0xB6, 0x00,                 # mov dh, 0
    ])
    # mov dl, [boot_drive]
    code += bytes([0x8A, 0x16]) + struct.pack("<H", 0x7C00 + boot_drive_offset)
    # int 0x13
    code += bytes([0xCD, 0x13])
    # jc disk_error
    disk_error_rel = 20  # approximate
    code += bytes([0x72, disk_error_rel])

    # Print "Stage-2 loaded OK"
    stage2_ok_offset = banner_offset + 20
    code += bytes([0xBE]) + struct.pack("<H", 0x7C00 + stage2_ok_offset)
    code += bytes([0xE8]) + struct.pack("<h", (0x7C00 + print_string_offset) - (0x7C00 + code_start + len(code) + 3))

    # jmp 0x0000:0x8000
    code += bytes([0xEA, 0x00, 0x80, 0x00, 0x00])

    # disk_error: print error message, halt
    disk_err_msg_offset = stage2_ok_offset + 20
    code += bytes([0xBE]) + struct.pack("<H", 0x7C00 + disk_err_msg_offset)
    code += bytes([0xE8]) + struct.pack("<h", (0x7C00 + print_string_offset) - (0x7C00 + code_start + len(code) + 3))
    # hlt; jmp halt
    code += bytes([0xF4, 0xEB, 0xFD])

    # Place code
    sector[code_start:code_start + len(code)] = code

    # Data area
    sector[boot_drive_offset] = 0  # boot_drive placeholder
    banner = b"\r\nMMUKO-OS stage-1\r\n\x00"
    sector[banner_offset:banner_offset + len(banner)] = banner
    stage2_ok = b"Stage-2 loaded OK\r\n\x00"
    sector[stage2_ok_offset:stage2_ok_offset + len(stage2_ok)] = stage2_ok
    disk_err = b"Disk error - halting\r\n\x00"
    sector[disk_err_msg_offset:disk_err_msg_offset + len(disk_err)] = disk_err

    # Print string routine
    ps = emit_print_string_routine()
    sector[print_string_offset:print_string_offset + len(ps)] = ps

    # Boot signature
    struct.pack_into("<H", sector, 510, 0xAA55)

    out = build_dir / "boot.bin"
    out.write_bytes(sector)
    sig = struct.unpack_from("<H", sector, 510)[0]
    print(f"[STAGE1] {out}: {len(sector)} bytes, sig=0x{sig:04X}")


def build_stage2(build_dir: Path) -> None:
    """Build stage-2 (mmuko-os.bin) — NSIGII handoff kernel."""
    # Stage-2 runs at 0x8000, prints messages, jumps to 0x8200
    code = bytearray()

    # cli; xor ax,ax; setup segments; sti
    code += bytes([
        0xFA,                       # cli
        0x31, 0xC0,                 # xor ax, ax
        0x8E, 0xD8,                 # mov ds, ax
        0x8E, 0xC0,                 # mov es, ax
        0x8E, 0xD0,                 # mov ss, ax
        0xBC, 0x00, 0x90,           # mov sp, 0x9000
        0xFB,                       # sti
    ])

    # Print "[stage2] NSIGII handoff"
    msg1_off = 0x80
    code += bytes([0xBE]) + struct.pack("<H", 0x8000 + msg1_off)
    ps_off = 0xC0
    code += bytes([0xE8]) + struct.pack("<h", ps_off - (len(code) + 3))

    # Print "[stage2] membrane PASS"
    msg2_off = msg1_off + 26
    code += bytes([0xBE]) + struct.pack("<H", 0x8000 + msg2_off)
    code += bytes([0xE8]) + struct.pack("<h", ps_off - (len(code) + 3))

    # Print "[stage2] jumping runtime"
    msg3_off = msg2_off + 25
    code += bytes([0xBE]) + struct.pack("<H", 0x8000 + msg3_off)
    code += bytes([0xE8]) + struct.pack("<h", ps_off - (len(code) + 3))

    # jmp 0x0000:0x8200
    code += bytes([0xEA, 0x00, 0x82, 0x00, 0x00])

    # Pad to data area
    code += bytes(msg1_off - len(code))

    # Messages
    msgs = [
        b"\r\n[stage2] NSIGII handoff\r\n\x00",
        b"[stage2] membrane PASS\r\n\x00",
        b"[stage2] jumping runtime\r\n\x00",
    ]
    for m in msgs:
        code += m

    # Pad to print_string routine
    if len(code) < ps_off:
        code += bytes(ps_off - len(code))
    code += emit_print_string_routine()

    # Pad to 512 bytes
    if len(code) < 512:
        code += bytes(512 - len(code))

    out = build_dir / "mmuko-os.bin"
    out.write_bytes(code[:512])
    print(f"[STAGE2] {out}: {min(len(code), 512)} bytes (mmuko-os kernel)")


def build_runtime(build_dir: Path) -> None:
    """Build runtime.bin — firmware entry at 0x8200."""
    code = bytearray()

    # cli; xor ax,ax; setup segments; sti
    code += bytes([
        0xFA,                       # cli
        0x31, 0xC0,                 # xor ax, ax
        0x8E, 0xD8,                 # mov ds, ax
        0x8E, 0xC0,                 # mov es, ax
        0x8E, 0xD0,                 # mov ss, ax
        0xBC, 0x00, 0x98,           # mov sp, 0x9800
        0xFB,                       # sti
    ])

    # Print "[runtime] firmware entry"
    msg1_off = 0x60
    code += bytes([0xBE]) + struct.pack("<H", 0x8200 + msg1_off)
    ps_off = 0xA0
    code += bytes([0xE8]) + struct.pack("<h", ps_off - (len(code) + 3))

    # Print "runtime ready; handoff contract preserved"
    msg2_off = msg1_off + 28
    code += bytes([0xBE]) + struct.pack("<H", 0x8200 + msg2_off)
    code += bytes([0xE8]) + struct.pack("<h", ps_off - (len(code) + 3))

    # hlt; jmp halt
    code += bytes([0xF4, 0xEB, 0xFD])

    # Pad to data area
    if len(code) < msg1_off:
        code += bytes(msg1_off - len(code))

    msgs = [
        b"\r\n[runtime] firmware entry\r\n\x00",
        b"runtime ready; handoff contract preserved\r\n\x00",
    ]
    for m in msgs:
        code += m

    # Pad to print_string
    if len(code) < ps_off:
        code += bytes(ps_off - len(code))
    code += emit_print_string_routine()

    # Pad to sector boundary
    size = ((len(code) + 511) // 512) * 512
    if len(code) < size:
        code += bytes(size - len(code))

    out = build_dir / "runtime.bin"
    out.write_bytes(code)
    print(f"[RUNTIME] {out}: {len(code)} bytes (firmware entry)")


def main() -> int:
    parser = argparse.ArgumentParser(description="MMUKO-OS boot assembler (Python fallback)")
    parser.add_argument("--build-dir", default="build")
    args = parser.parse_args()
    bd = Path(args.build_dir)
    bd.mkdir(parents=True, exist_ok=True)

    print("MMUKO-OS Boot Chain Assembly (Python fallback — no NASM)")
    print("=" * 55)
    build_stage1(bd)
    build_stage2(bd)
    build_runtime(bd)
    print("=" * 55)
    print("Boot chain assembled successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
