# MMUKO-OS Bootable ISO Guide

## Overview

This guide explains how to create and boot the **MMUKO-OS bootable ISO** for x86-64 VirtualBox environments. The ISO contains the NSIGII-verified MMUKO-OS bootloader with full interdependency tree resolution support.

## Quick Start

### Option 1: Using Pre-built ISO (Recommended)

If an ISO already exists at `img/mmuko-os-bootable.iso`:

```bash
# Make the boot script executable
chmod +x iso-boot.sh

# Start the VM
./iso-boot.sh
```

### Option 2: Create ISO from Boot Sector

If the ISO doesn't exist, create it from the boot sector:

```bash
# Using Python script (no external tools required)
python3 iso-creator.py

# Or with custom paths
python3 iso-creator.py --boot-img img/mmuko-os.img --output img/mmuko-os-bootable.iso
```

Then boot:
```bash
./iso-boot.sh
```

## File Structure

```
mmuko-os/
├── img/
│   ├── mmuko-os.img              # 512-byte boot sector
│   └── mmuko-os-bootable.iso     # Generated bootable ISO
├── iso-creator.py                # Standalone ISO creator
├── iso-boot.sh                   # VirtualBox ISO boot script
└── ringboot.sh                   # Legacy floppy boot script
```

## Creating the ISO

### Method 1: Using iso-creator.py (Recommended)

```bash
python3 iso-creator.py --verbose
```

**Output:**
- Creates `img/mmuko-os-bootable.iso`
- x86-64 BIOS bootable with El Torito support
- ~50 KB minimal ISO size

### Method 2: Using Available System Tools

If you have `xorriso` or `mkisofs` installed:

```bash
# Using xorriso
xorriso -as mkisofs \
  -boot-load-size 1 \
  -boot-info-table \
  -b img/mmuko-os.img \
  -o img/mmuko-os-bootable.iso \
  -isohybrid-mbr /dev/null \
  -follow-links \
  img/

# Using mkisofs/genisoimage
mkisofs -b img/mmuko-os.img \
  -c boot.cat \
  -o img/mmuko-os-bootable.iso \
  -isohybrid-mbr /usr/lib/syslinux/isohdpfx.bin \
  img/
```

## Booting in VirtualBox

### Automated Setup (Recommended)

```bash
./iso-boot.sh
```

This will:
1. Check for VirtualBox installation
2. Create a new VM with optimal settings
3. Attach the ISO
4. Configure serial port logging
5. Start the VM

### Manual Setup

If you prefer to configure VirtualBox manually:

1. **Create VM:**
   - Name: `MMUKO-OS-ISO-Boot`
   - Type: Linux
   - Version: Linux (64-bit)
   - RAM: 128 MB
   - VRAM: 8 MB
   - Storage: 10 GB (dynamic)

2. **Configure Storage:**
   - Create IDE controller (PIIX4)
   - Attach `img/mmuko-os-bootable.iso` as CD/DVD drive
   - Boot order: CD/DVD first

3. **Configure Boot:**
   - System → Firmware: BIOS
   - System → Boot Order: Enable DVD/CD
   - System → Secure Boot: Disabled

4. **Optional - Serial Output:**
   - Settings → Serial Ports
   - Port 1: Host Device `/dev/ttyS0` (Linux) or `COM1` (Windows)
   - Or file: `/tmp/mmuko-os-iso-serial.log`

5. **Start VM:**
   - Double-click VM or press `Start`

## Expected Boot Sequence

When the VM boots, you should see:

```
=== MMUKO-OS RINGBOOT ===
OBINEXUS NSIGII Verify

[Phase 1] SPARSE
[Phase 2] REMEMBER
[Phase 3] ACTIVE
[Phase 4] VERIFY

RIFT Header: NXOB ✓
Boot Signature: 55AA ✓
Interdependency Tree: RESOLVED ✓
NSIGII Verification: PASSED ✓

BOOT_SUCCESS
```

Then the system halts with code `0x55` (NSIGII_YES).

## Troubleshooting

### "No bootable medium found"

**Issue:** VM can't find a bootable device.

**Solutions:**
1. Verify ISO path is correct in VM settings
2. Check boot order in VirtualBox settings
3. Ensure IDE controller is properly configured
4. Verify boot signature: `xxd -s 510 -l 2 img/mmuko-os.img`
   - Should show: `55aa`

### ISO doesn't boot but floppy image does

**Issue:** ISO structure might be incorrect.

**Solutions:**
1. Recreate ISO: `python3 iso-creator.py --verbose`
2. Verify ISO structure:
   ```bash
   file img/mmuko-os-bootable.iso
   # Should show: "ISO 9660 CD-ROM filesystem data"
   ```
3. Check boot image: `ls -lah img/mmuko-os.img`
   - Must be exactly 512 bytes

### VM starts but doesn't boot

**Issue:** BIOS might not recognize ISO as bootable.

**Solutions:**
1. Check BIOS firmware in VM settings
   - Set to: BIOS (not UEFI)
2. Verify boot device in VM settings
   - Boot priority: CD/DVD first
3. Check serial output for errors:
   ```bash
   tail -f /tmp/mmuko-os-iso-serial.log
   ```

### "NSIGII verification failed"

**Issue:** Bootloader verification didn't pass.

**Likely causes:**
1. Boot image corrupted or modified
2. RIFT header checksum incorrect
3. Memory allocation issue in VM

**Debug steps:**
1. Verify original boot image:
   ```bash
   xxd img/mmuko-os.img | head -1
   ```
2. Rebuild everything:
   ```bash
   ./build.sh
   python3 iso-creator.py --verbose
   ```
3. Check VM memory allocation (increase to 256MB if needed)

## ISO Technical Details

### ISO 9660 Structure

```
Sector  0-15: System Area (reserved)
Sector 16: Primary Volume Descriptor (PVD)
Sector 17: El Torito Boot Catalog
Sector 18: Volume Descriptor Set Terminator
Sector 19: Boot Image (mmuko-os.img, 512 bytes)
Sector 20-24: Padding
```

### El Torito Boot Configuration

- **Boot Media Type:** 1.44 MB Floppy
- **Load Segment:** 0x0001
- **Boot Entry Signature:** 0x88 (bootable, no emulation)
- **Load Count:** 1 sector (512 bytes)
- **Boot Image LBA:** Sector 19

### Boot Signature Verification

The mmuko-os.img boot sector includes:

```
Offset 0x00: RIFT Header (8 bytes)
    Magic: "NXOB" (OBINexus signature)
    Version: 0x01
    Checksum: 0xFE
    Flags: 0x01

Offset 0x510: Boot Signature
    Must be: 0x55AA (x86 boot magic)
```

## Integration with Build System

### Update Makefile

To automatically create the ISO during build:

```makefile
# Add to Makefile
iso: img/mmuko-os-bootable.iso

img/mmuko-os-bootable.iso: img/mmuko-os.img
	python3 iso-creator.py

# Update 'all' target
all: build iso

test-iso: iso
	./iso-boot.sh
```

### Update build.sh

```bash
# Add to build.sh at end
echo "Creating bootable ISO..."
python3 iso-creator.py --verbose || {
    echo -e "${RED}Failed to create ISO${NC}"
    exit 1
}
```

## Advanced Usage

### Automated Testing

```bash
#!/bin/bash
# automated-iso-test.sh

# Clean previous builds
rm -f img/mmuko-os-bootable.iso

# Build system
./build.sh || exit 1

# Create ISO
python3 iso-creator.py || exit 1

# Boot test
./iso-boot.sh

# Check serial output
if grep -q "BOOT_SUCCESS" /tmp/mmuko-os-iso-serial.log; then
    echo "✓ ISO boot test PASSED"
    exit 0
else
    echo "✗ ISO boot test FAILED"
    tail /tmp/mmuko-os-iso-serial.log
    exit 1
fi
```

### Custom ISO Sizes

The default ISO is 50 KB (minimal). To create larger ISO:

Edit `iso-creator.py`, line with `num_sectors = 25`:

```python
num_sectors = 50   # For ~100 KB ISO
num_sectors = 100  # For ~200 KB ISO
```

## Performance Notes

### VirtualBox VM Settings for Optimal Boot

```bash
VBoxManage modifyvm "MMUKO-OS-ISO-Boot" \
  --cpus 1 \
  --memory 128 \
  --vram 8 \
  --rtcuseutc on \
  --graphicscontroller vga
```

### Boot Time

- **Expected:** 1-3 seconds from ISO mount to NSIGII verification
- **Interdependency resolution:** <1 second
- **Total to halt:** <5 seconds

## Differences: ISO vs Floppy Boot

| Feature | ISO Boot | Floppy Boot |
|---------|----------|------------|
| VirtualBox Device | CD/DVD | Floppy |
| Boot Method | El Torito | Direct FDD |
| Compatibility | All modern systems | Legacy/32-bit |
| Boot Speed | Faster (cached) | Slower |
| Media Size | ~50 KB+ | Exactly 512 bytes |
| Script | `iso-boot.sh` | `ringboot.sh` |

## See Also

- README.md - Overall project documentation
- ringboot.sh - Legacy floppy boot script
- build.sh - Build system automation
- iso-creator.py - ISO creation source code

---

**Built with NSIGII verification and interdependency resolution for OBINEXUS.**

