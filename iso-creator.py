#!/usr/bin/env python3
"""
ISO Creator for MMUKO-OS Bootable Image
Creates a bootable x86-64 ISO with El Torito support from mmuko-os.img
"""

import os
import struct
import sys
import argparse

def create_bootable_iso(boot_img_path, output_iso_path, verbose=False):
    """
    Create a bootable ISO 9660 image with El Torito support
    from a 512-byte boot sector image.
    
    Args:
        boot_img_path: Path to the 512-byte boot sector image
        output_iso_path: Path to write the output ISO
        verbose: Enable verbose output
    """
    
    # Read boot image
    if not os.path.exists(boot_img_path):
        print(f"Error: Boot image not found: {boot_img_path}")
        return False
    
    with open(boot_img_path, 'rb') as f:
        boot_sector = f.read()
    
    if len(boot_sector) != 512:
        print(f"Error: Boot image must be exactly 512 bytes, got {len(boot_sector)}")
        return False
    
    # Verify boot signature
    if boot_sector[-2:] != b'\x55\xAA':
        print("Warning: Boot sector signature (0x55AA) not found")
    
    if verbose:
        print(f"✓ Boot image loaded: {boot_img_path} ({len(boot_sector)} bytes)")
        print(f"✓ Boot signature: {boot_sector[-2:].hex()}")
    
    # Create El Torito boot catalog (one sector = 2048 bytes)
    boot_catalog = bytearray(2048)
    
    # Validation Entry (8 bytes at offset 0)
    boot_catalog[0] = 0x01  # Validation Entry Type
    boot_catalog[1] = 0x00  # Platform (0x00 = 80x86/BIOS)
    boot_catalog[2:4] = struct.pack('<H', 0x0000)  # Reserved
    
    # Calculate checksum for validation entry
    checksum = 0
    for i in range(0, 4, 2):
        checksum += struct.unpack('<H', boot_catalog[i:i+2])[0]
    checksum = (0x10000 - (checksum & 0xFFFF)) & 0xFFFF
    boot_catalog[4:6] = struct.pack('<H', checksum)
    boot_catalog[6:8] = b'\xAA\x55'  # Signature
    
    # Boot Entry (32 bytes at offset 32)
    boot_catalog[32] = 0x88  # Boot entry, not emulation, bootable
    boot_catalog[33] = 0x00  # Medium type (0 = 1.44MB floppy)
    boot_catalog[34:36] = struct.pack('<H', 0x0001)  # Load segment
    boot_catalog[36] = 0x00  # System type
    boot_catalog[37] = 0x00  # Reserved
    boot_catalog[38:40] = struct.pack('<H', 1)  # Load count (1 sector)
    boot_catalog[40:44] = struct.pack('<I', 19)  # Boot image LBA
    boot_catalog[44:2048] = b'\x00' * (2048 - 44)  # Padding
    
    if verbose:
        print("✓ El Torito boot catalog created")
    
    # Create Primary Volume Descriptor (PVD)
    pvd = bytearray(2048)
    pvd[0] = 0x01  # Type code for PVD
    pvd[1:6] = b'CD001'  # Standard identifier
    pvd[6] = 0x01  # Version
    
    # System identifier
    sys_id = b'MMUKO-OS BOOT'
    pvd[8:8+len(sys_id)] = sys_id
    pvd[8+len(sys_id):40] = b' ' * (32 - len(sys_id))
    
    # Volume identifier
    vol_id = b'MMUKO-OS'
    pvd[40:40+len(vol_id)] = vol_id
    pvd[40+len(vol_id):72] = b' ' * (32 - len(vol_id))
    
    # Logical block descriptors (both little-endian and big-endian)
    num_sectors = 25  # Minimal ISO size (25 sectors = 51.2 KB)
    pvd[80:84] = struct.pack('<I', num_sectors)  # Little-endian
    pvd[84:88] = struct.pack('>I', num_sectors)  # Big-endian
    
    # Escape sequences
    pvd[88:121] = b' ' * 33
    
    # Volume set size
    pvd[121:123] = struct.pack('<H', 1)
    pvd[123:125] = struct.pack('>H', 1)
    
    # Volume sequence number
    pvd[125:127] = struct.pack('<H', 1)
    pvd[127:129] = struct.pack('>H', 1)
    
    # Logical block size
    pvd[129:131] = struct.pack('<H', 2048)
    pvd[131:133] = struct.pack('>H', 2048)
    
    # Path table size (both endianness)
    pvd[133:137] = struct.pack('<I', 0)
    pvd[141:145] = struct.pack('>I', 0)
    
    # Root directory extent location
    pvd[156:160] = struct.pack('<I', 0)
    pvd[164:168] = struct.pack('>I', 0)
    
    # Volume set identifier, publisher, etc. - leave empty
    
    if verbose:
        print("✓ Primary Volume Descriptor created")
    
    # Create Terminator Descriptor
    term_desc = bytearray(2048)
    term_desc[0] = 0xFF  # Terminator type
    term_desc[1:6] = b'CD001'
    term_desc[6] = 0x01
    
    if verbose:
        print("✓ Terminator descriptor created")
    
    # Assemble ISO: System area (0-15) + PVD (16) + Boot catalog (17) + 
    # Boot image (18) + Terminator (19) + padding
    iso_data = bytearray(2048 * num_sectors)
    
    # PVD at sector 16
    iso_data[16*2048:(16*2048)+2048] = pvd
    
    # Boot catalog at sector 17
    iso_data[17*2048:(17*2048)+2048] = boot_catalog
    
    # Boot image at sector 19 (El Torito points here)
    iso_data[19*2048:(19*2048)+512] = boot_sector
    
    # Terminator at sector 18
    iso_data[18*2048:(18*2048)+2048] = term_desc
    
    if verbose:
        print(f"✓ ISO structure assembled ({len(iso_data)} bytes)")
    
    # Write ISO file
    try:
        with open(output_iso_path, 'wb') as f:
            f.write(iso_data)
        
        iso_size_kb = len(iso_data) / 1024
        print(f"\n{'='*60}")
        print(f"{'='*60}")
        print(f"✓ Bootable ISO created successfully!")
        print(f"✓ Location: {output_iso_path}")
        print(f"✓ Size: {len(iso_data)} bytes ({iso_size_kb:.1f} KB, {num_sectors} sectors)")
        print(f"{'='*60}")
        print(f"{'='*60}\n")
        
        if verbose:
            print("ISO Structure:")
            print(f"  Sector  0-15: System area (reserved)")
            print(f"  Sector 16: Primary Volume Descriptor (PVD)")
            print(f"  Sector 17: El Torito boot catalog")
            print(f"  Sector 18: Volume Descriptor Set Terminator")
            print(f"  Sector 19: Boot image (mmuko-os.img)")
            print(f"  Sector 20-24: Padding")
            print()
        
        return True
        
    except IOError as e:
        print(f"Error: Failed to write ISO: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(
        description='Create a bootable x86-64 ISO from MMUKO-OS boot sector'
    )
    parser.add_argument(
        '--boot-img',
        default='img/mmuko-os.img',
        help='Path to boot sector image (default: img/mmuko-os.img)'
    )
    parser.add_argument(
        '--output',
        default='img/mmuko-os-bootable.iso',
        help='Output ISO path (default: img/mmuko-os-bootable.iso)'
    )
    parser.add_argument(
        '-v', '--verbose',
        action='store_true',
        help='Enable verbose output'
    )
    
    args = parser.parse_args()
    
    success = create_bootable_iso(args.boot_img, args.output, args.verbose)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()

