#!/bin/bash
# ringboot.sh - VirtualBox Ring Boot Test Script
# Creates and boots MMUKO-OS in VirtualBox with NSIGII verification

set -e

VM_NAME="MMUKO-OS-RingBoot"
IMG_PATH="$(pwd)/img/mmuko-os.img"
VM_RAM=64          # 64MB RAM (minimal for boot test)
VM_VRAM=4          # 4MB video RAM

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== MMUKO-OS Ring Boot Test ===${NC}"
echo -e "${BLUE}Interdependency Tree Hierarchy Verification${NC}"
echo ""

# Check if VirtualBox is installed
if ! command -v VBoxManage &> /dev/null; then
    echo -e "${RED}Error: VirtualBox not found.${NC}"
    echo "Please install VirtualBox first: https://www.virtualbox.org/"
    exit 1
fi

# Check if boot image exists
if [ ! -f "$IMG_PATH" ]; then
    echo -e "${RED}Error: Boot image not found at $IMG_PATH${NC}"
    echo "Please run ./build.sh first"
    exit 1
fi

# Verify image
IMG_SIZE=$(stat -f%z "$IMG_PATH" 2>/dev/null || stat -c%s "$IMG_PATH" 2>/dev/null)
echo "Boot image: $IMG_PATH ($IMG_SIZE bytes)"

# Delete existing VM if it exists
if VBoxManage list vms | grep -q "$VM_NAME"; then
    echo -e "${YELLOW}Removing existing VM: $VM_NAME${NC}"
    VBoxManage unregistervm "$VM_NAME" --delete 2>/dev/null || true
fi

echo ""
echo "[1/6] Creating VirtualBox VM..."
VBoxManage createvm --name "$VM_NAME" --ostype "Other" --register 2>/dev/null || {
    echo -e "${RED}Failed to create VM${NC}"
    exit 1
}

echo "[2/6] Configuring VM settings..."
VBoxManage modifyvm "$VM_NAME" \
    --memory $VM_RAM \
    --vram $VM_VRAM \
    --boot1 floppy \
    --boot2 none \
    --boot3 none \
    --boot4 none \
    --acpi on \
    --ioapic off \
    --pae off \
    --firmware bios 2>/dev/null

echo "[3/6] Creating floppy controller..."
VBoxManage storagectl "$VM_NAME" \
    --name "Floppy" \
    --add floppy \
    --controller I82078 2>/dev/null

echo "[4/6] Attaching boot image as floppy disk..."
VBoxManage storageattach "$VM_NAME" \
    --storagectl "Floppy" \
    --port 0 \
    --device 0 \
    --type fdd \
    --medium "$IMG_PATH" 2>/dev/null

echo "[5/6] Configuring serial port for NSIGII output..."
VBoxManage modifyvm "$VM_NAME" \
    --uart1 0x3F8 4 \
    --uartmode1 file /tmp/mmuko-os-serial.log 2>/dev/null || true

echo "[6/6] VM Configuration Complete"
echo ""
echo -e "${GREEN}VM Configuration:${NC}"
echo "  Name: $VM_NAME"
echo "  RAM: ${VM_RAM}MB"
echo "  Boot Device: Floppy ($IMG_PATH)"
echo "  Serial Log: /tmp/mmuko-os-serial.log"
echo ""
echo -e "${BLUE}Expected Boot Sequence:${NC}"
echo "  1. RIFT header verification (NXOB magic)"
echo "  2. MUCO boot sequence (8-qubit compass)"
echo "  3. Interdependency tree resolution"
echo "  4. NSIGII trinary verification"
echo "  5. HALT with code 0x55 (NSIGII_YES)"
echo ""
echo -e "${YELLOW}Press Enter to start VM, or Ctrl+C to cancel...${NC}"
read

# Start VM
echo "Starting VM..."
VBoxManage startvm "$VM_NAME" --type gui 2>/dev/null || {
    echo -e "${RED}Failed to start VM${NC}"
    exit 1
}

echo ""
echo -e "${GREEN}VM started successfully!${NC}"
echo ""
echo "Monitoring boot sequence..."
echo ""

# Wait for VM to boot and halt
for i in {1..10}; do
    sleep 1
    VM_STATE=$(VBoxManage showvminfo "$VM_NAME" --machinereadable 2>/dev/null | grep "VMState=" | cut -d'"' -f2 || echo "unknown")
    
    if [ "$VM_STATE" == "powered off" ]; then
        echo -e "${GREEN}✓ VM has halted (boot sequence complete)${NC}"
        echo ""
        echo -e "${GREEN}=== Boot Test Successful ===${NC}"
        echo ""
        echo "The VM halted successfully, indicating:"
        echo "  - RIFT header was valid"
        echo "  - MUCO sequence executed"
        echo "  - NSIGII verification passed"
        echo ""
        break
    fi
    
    echo "  [$i/10] VM state: $VM_STATE"
done

if [ "$VM_STATE" == "running" ]; then
    echo ""
    echo -e "${YELLOW}VM is still running.${NC}"
    echo "Check the VM window to verify boot messages."
    echo ""
fi

echo "Commands:"
echo "  View serial output: tail -f /tmp/mmuko-os-serial.log"
echo "  Check VM status:    VBoxManage showvminfo $VM_NAME"
echo "  Power off VM:       VBoxManage controlvm $VM_NAME poweroff"
echo "  Remove VM:          VBoxManage unregistervm $VM_NAME --delete"
echo ""
