# Makefile - MMUKO-OS Build System
# Supports: Linux, macOS, WSL

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

# Source files
C_SRCS = $(SRC_DIR)/interdependency.c $(SRC_DIR)/mmuko_boot.c
CPP_SRCS = $(CPP_DIR)/riftbridge.cpp

# Object files
C_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))

# Default target
.PHONY: all clean test img cpp csharp

all: img test

# Create directories
$(BUILD_DIR) $(IMG_DIR):
	mkdir -p $@

# Compile C sources
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Link test executable
$(BUILD_DIR)/mmuko_test: $(C_OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^

# Generate boot image
img: $(IMG_PATH)

$(IMG_PATH): build_img.py | $(IMG_DIR)
	python3 build_img.py $@

# Test boot sequence
test: $(BUILD_DIR)/mmuko_test
	@echo "=== Running NSIGII Verification Test ==="
	@./$(BUILD_DIR)/mmuko_test && echo "✓ Test passed" || echo "✗ Test failed"

# Build C++ RiftBridge
cpp: $(BUILD_DIR)/riftbridge.o

$(BUILD_DIR)/riftbridge.o: $(CPP_DIR)/riftbridge.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build C# (requires dotnet)
csharp:
	@if command -v dotnet >/dev/null 2>&1; then \
		cd $(CSHARP_DIR) && dotnet build; \
	else \
		echo "⚠ dotnet not found, skipping C# build"; \
	fi

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(IMG_DIR)
	@cd $(CSHARP_DIR) && dotnet clean 2>/dev/null || true

# Verify boot image
verify: $(IMG_PATH)
	@echo "=== Verifying Boot Image ==="
	@echo "Size: $$(stat -f%z '$(IMG_PATH)' 2>/dev/null || stat -c%s '$(IMG_PATH)' 2>/dev/null) bytes"
	@echo "RIFT Magic: $$(xxd -p -s 0 -l 4 '$(IMG_PATH)')"
	@echo "Boot Sig: $$(xxd -p -s 510 -l 2 '$(IMG_PATH)')"

# VirtualBox test
vbox: $(IMG_PATH)
	./ringboot.sh

# Help
help:
	@echo "MMUKO-OS Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build everything (default)"
	@echo "  img     - Create bootable image"
	@echo "  test    - Run NSIGII verification test"
	@echo "  cpp     - Build C++ RiftBridge"
	@echo "  csharp  - Build C# implementation"
	@echo "  verify  - Verify boot image integrity"
	@echo "  vbox    - Test in VirtualBox"
	@echo "  clean   - Remove build artifacts"
	@echo "  help    - Show this help"
