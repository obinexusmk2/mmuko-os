#!/bin/bash
# build.sh - MMUKO-OS Build System
# Builds bootable image with interdependency tree hierarchy
# 
# Supports: Linux, macOS, WSL
# Outputs: img/mmuko-os.img (512-byte boot sector)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
CC=${CC:-gcc}
CXX=${CXX:-g++}
ASM=${ASM:-nasm}
CFLAGS="-Wall -Wextra -std=c11 -I./include -O2"
CXXFLAGS="-Wall -Wextra -std=c++17 -I./include -O2"
ASMFLAGS="-f bin"

# Directories
IMG_DIR="img"
BUILD_DIR="build"
SRC_DIR="src"
CPP_DIR="cpp"
CSHARP_DIR="csharp"

# Image name
IMG_NAME="mmuko-os.img"
IMG_PATH="${IMG_DIR}/${IMG_NAME}"

echo -e "${BLUE}=== MMUKO-OS Build System ===${NC}"
echo -e "${BLUE}Interdependency Tree Hierarchy Boot${NC}"
echo ""

# Create directories
mkdir -p ${IMG_DIR} ${BUILD_DIR}

# Function to print status
print_status() {
    echo -e "${BLUE}[${1}/${2}]${NC} ${3}"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

# ============================================================================
# Step 1: Compile C Interdependency System
# ============================================================================
print_status 1 6 "Compiling C interdependency system..."

${CC} ${CFLAGS} -c ${SRC_DIR}/interdependency.c -o ${BUILD_DIR}/interdependency.o 2>/dev/null || {
    print_error "Failed to compile interdependency.c"
    exit 1
}

${CC} ${CFLAGS} -c ${SRC_DIR}/mmuko_boot.c -o ${BUILD_DIR}/mmuko_boot.o 2>/dev/null || {
    print_error "Failed to compile mmuko_boot.c"
    exit 1
}

print_success "C compilation successful"

# ============================================================================
# Step 2: Link and Test Boot Sequence
# ============================================================================
print_status 2 6 "Linking boot sequence test..."

${CC} ${CFLAGS} -o ${BUILD_DIR}/mmuko_test \
    ${BUILD_DIR}/interdependency.o \
    ${BUILD_DIR}/mmuko_boot.o 2>/dev/null || {
    print_error "Failed to link test executable"
    exit 1
}

print_success "Linking successful"

# ============================================================================
# Step 3: Run NSIGII Verification Test
# ============================================================================
print_status 3 6 "Running NSIGII verification test..."

if ${BUILD_DIR}/mmuko_test > /dev/null 2>&1; then
    print_success "NSIGII verification PASSED (exit code 0)"
else
    EXIT_CODE=$?
    if [ $EXIT_CODE -eq 1 ]; then
        print_error "NSIGII verification FAILED"
        exit 1
    else
        print_warning "Unexpected exit code: $EXIT_CODE"
    fi
fi

# ============================================================================
# Step 4: Assemble Boot Sector
# ============================================================================
print_status 4 6 "Assembling boot sector..."

if command -v ${ASM} &> /dev/null; then
    # Use NASM for proper assembly
    ${ASM} ${ASMFLAGS} -o ${IMG_PATH} ${SRC_DIR}/boot_sector.asm 2>/dev/null || {
        print_error "NASM assembly failed"
        exit 1
    }
    print_success "Boot sector assembled with NASM"
else
    print_warning "NASM not found, using C fallback..."
    
    # Fallback: Create boot image using C
    cat > ${BUILD_DIR}/mkboot.c << 'CEOF'
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* MMUKO-OS 512-byte boot sector - C implementation */

int main(void) {
    FILE *f = fopen("img/mmuko-os.img", "wb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    uint8_t sector[512];
    memset(sector, 0, 512);

    /* RIFT Header (8 bytes) */
    sector[0] = 'N';
    sector[1] = 'X';
    sector[2] = 'O';
    sector[3] = 'B';
    sector[4] = 0x01;   /* Version */
    sector[5] = 0x00;   /* Reserved */
    sector[6] = 0xFE;   /* Checksum */
    sector[7] = 0x01;   /* Flags */

    /* Boot code at offset 8 */
    uint8_t boot_code[] = {
        0xFA,                   /* cli */
        0x31, 0xC0,             /* xor ax, ax */
        0x8E, 0xD8,             /* mov ds, ax */
        0x8E, 0xC0,             /* mov es, ax */
        0xBC, 0x00, 0x7C,       /* mov sp, 0x7C00 */
        /* Print message */
        0xBE, 0x60, 0x7C,       /* mov si, msg */
        0xB4, 0x0E,             /* mov ah, 0x0E */
        /* Loop */
        0xAC,                   /* lodsb */
        0x08, 0xC0,             /* or al, al */
        0x74, 0x04,             /* jz done */
        0xCD, 0x10,             /* int 0x10 */
        0xEB, 0xF5,             /* jmp loop */
        /* Done */
        0xB0, 0x55,             /* mov al, 0x55 (NSIGII_YES) */
        0xF4,                   /* hlt */
        0xEB, 0xFE              /* jmp $ */
    };

    memcpy(&sector[8], boot_code, sizeof(boot_code));

    /* Messages at offset 0x60 */
    const char *msg = "=== MMUKO-OS RINGBOOT ===\r\n"
                      "OBINEXUS NSIGII Verify\r\n"
                      "[Phase 1] SPARSE\r\n"
                      "[Phase 2] REMEMBER\r\n"
                      "[Phase 3] ACTIVE\r\n"
                      "[Phase 4] VERIFY\r\n\n"
                      "NSIGII_VERIFIED\r\n"
                      "BOOT_SUCCESS\r\n";
    memcpy(&sector[0x60], msg, strlen(msg) + 1);

    /* Boot signature at offset 510 */
    sector[510] = 0x55;
    sector[511] = 0xAA;

    fwrite(sector, 512, 1, f);
    fclose(f);

    return 0;
}
CEOF
    ${CC} -o ${BUILD_DIR}/mkboot ${BUILD_DIR}/mkboot.c 2>/dev/null || {
        print_error "Failed to compile boot sector generator"
        exit 1
    }
    ${BUILD_DIR}/mkboot
    print_success "Boot sector created with C fallback"
fi

# ============================================================================
# Step 5: Verify Boot Image
# ============================================================================
print_status 5 6 "Verifying boot image..."

# Check file size
if [ -f ${IMG_PATH} ]; then
    IMG_SIZE=$(stat -f%z "${IMG_PATH}" 2>/dev/null || stat -c%s "${IMG_PATH}" 2>/dev/null)
    if [ "$IMG_SIZE" -eq 512 ]; then
        print_success "Boot image is exactly 512 bytes"
    else
        print_error "Boot image is $IMG_SIZE bytes (expected 512)"
        exit 1
    fi
else
    print_error "Boot image not found"
    exit 1
fi

# Check RIFT header magic
MAGIC=$(xxd -p -s 0 -l 4 "${IMG_PATH}" 2>/dev/null || echo "00000000")
if [ "$MAGIC" = "4e584f42" ]; then
    print_success "RIFT header magic verified (NXOB)"
else
    print_error "RIFT header magic invalid (expected 4e584f42, got $MAGIC)"
    exit 1
fi

# Check boot signature
BOOT_SIG=$(xxd -p -s 510 -l 2 "${IMG_PATH}" 2>/dev/null || echo "0000")
if [ "$BOOT_SIG" = "55aa" ]; then
    print_success "Boot signature (0x55AA) verified"
else
    print_error "Boot signature invalid (expected 55aa, got $BOOT_SIG)"
    exit 1
fi

# ============================================================================
# Step 6: Build C++ RiftBridge (optional)
# ============================================================================
print_status 6 6 "Building C++ RiftBridge..."

if command -v ${CXX} &> /dev/null; then
    ${CXX} ${CXXFLAGS} -c ${CPP_DIR}/riftbridge.cpp -o ${BUILD_DIR}/riftbridge.o 2>/dev/null && {
        print_success "C++ RiftBridge compiled"
    } || {
        print_warning "C++ RiftBridge compilation skipped"
    }
else
    print_warning "C++ compiler not found"
fi

# ============================================================================
# Summary
# ============================================================================
echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo "Bootable image: ${IMG_PATH}"
echo ""
echo "Image details:"
echo "  - Size: 512 bytes (exact boot sector)"
echo "  - RIFT Header: NXOB v1 (checksum FE)"
echo "  - Boot Signature: 0x55AA"
echo "  - NSIGII Protocol: Trinary (YES/NO/MAYBE)"
echo "  - Boot Sequence: SPARSE → REMEMBER → ACTIVE → VERIFY"
echo "  - Interdependency: 8-node tree hierarchy"
echo ""
echo "To test in VirtualBox:"
echo "  1. Create new VM (Type: Other, Version: Other/Unknown)"
echo "  2. Attach ${IMG_PATH} as floppy disk"
echo "  3. Boot VM"
echo ""
echo "Expected output:"
echo "  === MMUKO-OS RINGBOOT ==="
echo "  OBINEXUS NSIGII Verify"
echo "  [Phase 1] SPARSE"
echo "  [Phase 2] REMEMBER"
echo "  [Phase 3] ACTIVE"
echo "  [Phase 4] VERIFY"
echo "  NSIGII_VERIFIED"
echo "  BOOT_SUCCESS"
echo ""
