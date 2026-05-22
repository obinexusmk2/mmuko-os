# Makefile - MMUKO-OS Build System
# Supports: Linux, macOS, WSL, Windows (PowerShell + make)

# Compilers
CC = gcc
CXX = g++
ASM = nasm

# Flags
CFLAGS = -Wall -Wextra -std=c11 -I./include -O2
CXXFLAGS = -Wall -Wextra -std=c++17 -I./include -O2
ASMFLAGS = -f bin

# Directories
SRC_DIR = src
INC_DIR = include
CPP_DIR = cpp
CSHARP_DIR = csharp
BUILD_DIR = build
IMG_DIR = img

# Targets
IMG_NAME = mmuko-os.img
IMG_PATH = $(IMG_DIR)/$(IMG_NAME)

# QEMU configuration
QEMU = qemu-system-i386
QEMU_FLAGS = -fda $(IMG_PATH)
QEMU_SERIAL = $(QEMU_FLAGS) -serial mon:stdio -nographic
QEMU_DEBUG  = $(QEMU_FLAGS) -s -S -nographic

# Source files
C_SRCS = $(SRC_DIR)/interdependency.c $(SRC_DIR)/mmuko_boot.c
CPP_SRCS = $(CPP_DIR)/riftbridge.cpp

# Object files
C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))

# Cross-platform directory creation via Python (avoids mkdir -p vs Windows mkdir conflict)
MKDIR_CMD = python3 -c "import os,sys; os.makedirs(sys.argv[1], exist_ok=True)"

# Default target
.PHONY: all clean test build-img cpp csharp verify vbox qemu qemu-serial qemu-debug help

all: build-img test

# Create directories (cross-platform via Python)
$(BUILD_DIR):
	$(MKDIR_CMD) $@

$(IMG_DIR):
	$(MKDIR_CMD) $@

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link test executable
$(BUILD_DIR)/mmuko_test: $(C_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Generate boot image  (renamed from 'img' to avoid conflict with img/ directory)
build-img: $(IMG_PATH)

$(IMG_PATH): build_img.py | $(IMG_DIR)
	python3 build_img.py $@

# Test boot sequence
test: $(BUILD_DIR)/mmuko_test
	@echo "=== Running NSIGII Verification Test ==="
	@./$(BUILD_DIR)/mmuko_test && echo "Test passed" || echo "Test failed"

# Build C++ RiftBridge
cpp: $(BUILD_DIR)/riftbridge.o

$(BUILD_DIR)/riftbridge.o: $(CPP_DIR)/riftbridge.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build C# (requires dotnet)
csharp:
	@if command -v dotnet >/dev/null 2>&1; then \
		cd $(CSHARP_DIR) && dotnet build; \
	else \
		echo "dotnet not found, skipping C# build"; \
	fi

# Clean build artifacts
clean:
	python3 -c "import shutil,os; [shutil.rmtree(d, ignore_errors=True) for d in ['$(BUILD_DIR)','$(IMG_DIR)']]"

# Verify boot image (no longer depends on 'img' phony — depends directly on the file)
verify: $(IMG_PATH)
	@python3 -c "\
import sys; \
d=open('$(IMG_PATH)','rb').read(); \
print('=== Verifying Boot Image ==='); \
print(f'Size: {len(d)} bytes'); \
print(f'RIFT Magic: {d[0:4]}'); \
print(f'Boot Sig: 0x{d[510]:02X}{d[511]:02X}'); \
ok = d[0:4]==b'NXOB' and d[510]==0x55 and d[511]==0xAA; \
print('PASS' if ok else 'FAIL'); \
sys.exit(0 if ok else 1)"

# QEMU boot targets
qemu: $(IMG_PATH)
	$(QEMU) $(QEMU_FLAGS)

qemu-serial: $(IMG_PATH)
	$(QEMU) $(QEMU_SERIAL)

qemu-debug: $(IMG_PATH)
	@echo "GDB stub on :1234 — run: gdb then 'target remote :1234'"
	$(QEMU) $(QEMU_DEBUG)

# VirtualBox test
vbox: $(IMG_PATH)
	./ringboot.sh

# Help
help:
	@echo "MMUKO-OS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build everything (default)"
	@echo "  build-img    - Create bootable floppy image"
	@echo "  test         - Run NSIGII verification test"
	@echo "  verify       - Verify boot image integrity"
	@echo "  qemu         - Boot in QEMU (GUI window)"
	@echo "  qemu-serial  - Boot in QEMU (serial output to terminal)"
	@echo "  qemu-debug   - Boot in QEMU with GDB stub on :1234"
	@echo "  cpp          - Build C++ RiftBridge"
	@echo "  csharp       - Build C# implementation"
	@echo "  vbox         - Test in VirtualBox"
	@echo "  clean        - Remove build artifacts"
	@echo "  help         - Show this help"
