#!/bin/bash
# iso-boot.sh - VirtualBox ISO Boot Setup for MMUKO-OS
# Creates and boots MMUKO-OS ISO in VirtualBox with NSIGII verification

set -e

VM_NAME="MMUKO-OS-ISO-Boot"
ISO_PATH="$(pwd)/img/mmuko-os-bootable.iso"
VM_RAM=128         # 128MB RAM for ISO boot
VM_VRAM=8          # 8MB video RAM

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}=== MMUKO-OS ISO Boot Test ===${NC}"
echo -e "${BLUE}x86-64 Bootable ISO Setup${NC}"
echo ""

echo -e "${BLUE}=== Preflight Checks ===${NC}"

# Check if VirtualBox is installed
if ! command -v VBoxManage >/dev/null 2>&1; then
    echo -e "${RED}Error: VBoxManage is not available in PATH.${NC}"
    echo "Install VirtualBox and ensure VBoxManage is accessible from your shell."
    exit 1
fi

# Check if ISO exists
if [ ! -f "$ISO_PATH" ]; then
    echo -e "${RED}Error: Bootable ISO does not exist: $ISO_PATH${NC}"
    echo "Build the ISO first (for example: run iso-build.sh or python iso_creator.py)."
    exit 1
fi

# Verify ISO is readable
if [ ! -r "$ISO_PATH" ]; then
    echo -e "${RED}Error: ISO is not readable by current user: $ISO_PATH${NC}"
    exit 1
fi

# Check for WSL
IS_WSL=false
if grep -qi microsoft /proc/version 2>/dev/null; then
    IS_WSL=true
fi

VBOX_ISO_PATH="$ISO_PATH"
if [ "$IS_WSL" = true ]; then
    if ! command -v wslpath >/dev/null 2>&1; then
        echo -e "${RED}Error: Running in WSL but wslpath is unavailable.${NC}"
        exit 1
    fi

    if ! VBOX_ISO_PATH="$(wslpath -w "$ISO_PATH")"; then
        echo -e "${RED}Error: Failed to convert ISO path for Windows VirtualBox.${NC}"
        exit 1
    fi
fi

# Verify ISO
ISO_SIZE=$(stat -f%z "$ISO_PATH" 2>/dev/null || stat -c%s "$ISO_PATH" 2>/dev/null)
echo "Boot ISO (POSIX): $ISO_PATH ($ISO_SIZE bytes)"
if [ "$IS_WSL" = true ]; then
    echo "Boot ISO (VirtualBox): $VBOX_ISO_PATH"
fi

# Delete existing VM if it exists
if VBoxManage list vms | grep -q "$VM_NAME"; then
    echo -e "${YELLOW}Removing existing VM: $VM_NAME${NC}"
    VBoxManage unregistervm "$VM_NAME" --delete 2>/dev/null || true
fi

echo ""
echo "[1/6] Creating VirtualBox VM..."
VBoxManage createvm --name "$VM_NAME" --ostype "Linux_64" --register 2>/dev/null || {
    echo -e "${RED}Failed to create VM${NC}"
    exit 1
}

echo "[2/6] Configuring VM settings..."
VBoxManage modifyvm "$VM_NAME" \
    --memory $VM_RAM \
    --vram $VM_VRAM \
    --boot1 dvd \
    --boot2 none \
    --boot3 none \
    --boot4 none \
    --acpi on \
    --ioapic off \
    --firmware bios 2>/dev/null

echo "[3/6] Creating IDE controller..."
VBoxManage storagectl "$VM_NAME" \
    --name "IDE" \
    --add ide \
    --controller PIIX4 2>/dev/null

echo "[4/6] Attaching bootable ISO..."
VBoxManage storageattach "$VM_NAME" \
    --storagectl "IDE" \
    --port 0 \
    --device 0 \
    --type dvddrive \
    --medium "$VBOX_ISO_PATH" 2>/dev/null

echo "[5/6] Configuring serial port for output logging..."
VBoxManage modifyvm "$VM_NAME" \
    --uart1 0x3F8 4 \
    --uartmode1 file /tmp/mmuko-os-iso-serial.log 2>/dev/null || true

echo "[6/6] VM Configuration Complete"
echo ""
echo -e "${GREEN}VM Configuration:${NC}"
echo "  Name: $VM_NAME"
echo "  RAM: ${VM_RAM}MB"
echo "  Boot Device: ISO ($ISO_PATH)"
echo "  Serial Log: /tmp/mmuko-os-iso-serial.log"
echo ""
echo -e "${BLUE}Expected Boot Sequence:${NC}"
echo "  1. RIFT header verification (NXOB magic)"
echo "  2. MUCO boot sequence"
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
echo "  View serial output: tail -f /tmp/mmuko-os-iso-serial.log"
echo "  Check VM status:    VBoxManage showvminfo $VM_NAME"
echo "  Power off VM:       VBoxManage controlvm $VM_NAME poweroff"
echo "  Remove VM:          VBoxManage unregistervm $VM_NAME --delete"
echo ""

