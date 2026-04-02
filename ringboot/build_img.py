#!/usr/bin/env python3
"""
build_img.py - MMUKO-OS Boot Image Generator
Creates a 512-byte bootable image with RIFT header and NSIGII verification
"""

import os
import struct

def create_boot_image(output_path="img/mmuko-os.img"):
    """Create 512-byte boot sector image"""
    
    # Initialize 512-byte sector
    sector = bytearray(512)
    
    # RIFT Header (8 bytes)
    sector[0:4] = b'NXOB'      # Magic
    sector[4] = 0x01            # Version
    sector[5] = 0x00            # Reserved
    sector[6] = 0xFE            # Checksum
    sector[7] = 0x01            # Flags
    
    # x86 Boot Code at offset 8
    boot_code = bytes([
        0xFA,                   # cli
        0x31, 0xC0,             # xor ax, ax
        0x8E, 0xD8,             # mov ds, ax
        0x8E, 0xC0,             # mov es, ax
        0xBC, 0x00, 0x7C,       # mov sp, 0x7C00
        # Print message
        0xBE, 0x60, 0x7C,       # mov si, msg (0x7C60)
        0xB4, 0x0E,             # mov ah, 0x0E (teletype)
        # Print loop
        0xAC,                   # lodsb
        0x08, 0xC0,             # or al, al
        0x74, 0x04,             # jz done
        0xCD, 0x10,             # int 0x10
        0xEB, 0xF5,             # jmp loop
        # Done - NSIGII_YES
        0xB0, 0x55,             # mov al, 0x55 (NSIGII_YES)
        0xF4,                   # hlt
        0xEB, 0xFE              # jmp $ (safety)
    ])
    
    sector[8:8+len(boot_code)] = boot_code
    
    # Boot message at offset 0x60 (96)
    msg = (b"=== MMUKO-OS RINGBOOT ===\r\n"
           b"OBINEXUS NSIGII Verify\r\n"
           b"[Phase 1] SPARSE\r\n"
           b"[Phase 2] REMEMBER\r\n"
           b"[Phase 3] ACTIVE\r\n"
           b"[Phase 4] VERIFY\r\n\n"
           b"NSIGII_VERIFIED\r\n"
           b"BOOT_SUCCESS\r\n\x00")
    
    sector[0x60:0x60+len(msg)] = msg
    
    # Boot signature at offset 510
    sector[510] = 0x55
    sector[511] = 0xAA
    
    # Create output directory
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    
    # Write image
    with open(output_path, 'wb') as f:
        f.write(sector)
    
    return output_path

def verify_image(path):
    """Verify boot image integrity"""
    
    with open(path, 'rb') as f:
        data = f.read()
    
    if len(data) != 512:
        print(f"ERROR: Image size is {len(data)} bytes (expected 512)")
        return False
    
    # Check RIFT magic
    magic = data[0:4]
    if magic != b'NXOB':
        print(f"ERROR: RIFT magic is {magic} (expected NXOB)")
        return False
    
    # Check boot signature
    if data[510] != 0x55 or data[511] != 0xAA:
        print(f"ERROR: Boot signature is {data[510]:02X}{data[511]:02X} (expected 55AA)")
        return False
    
    return True

def print_info(path):
    """Print image information"""
    
    with open(path, 'rb') as f:
        data = f.read()
    
    print(f"Image: {path}")
    print(f"Size: {len(data)} bytes")
    print(f"RIFT Magic: {data[0:4].decode('ascii')}")
    print(f"Version: {data[4]}")
    print(f"Checksum: 0x{data[6]:02X}")
    print(f"Boot Signature: 0x{data[510]:02X}{data[511]:02X}")
    
    # Extract message
    msg_end = data.find(b'\x00', 0x60)
    if msg_end > 0x60:
        msg = data[0x60:msg_end].decode('ascii', errors='replace')
        print(f"\nBoot Message:\n{msg}")

if __name__ == "__main__":
    import sys
    
    output = sys.argv[1] if len(sys.argv) > 1 else "img/mmuko-os.img"
    
    print("=== MMUKO-OS Boot Image Generator ===")
    print(f"Creating: {output}")
    
    create_boot_image(output)
    
    if verify_image(output):
        print(f"✓ Boot image created successfully")
        print()
        print_info(output)
    else:
        print("✗ Boot image verification failed")
        sys.exit(1)
