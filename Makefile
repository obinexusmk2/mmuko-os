# Makefile - MMUKO-OS / NSIGII Heartfull Firmware
# Builds firmware libraries plus a raw BIOS image with stage-1, stage-2, and runtime artifacts.

CC     := gcc
CXX    := g++
NASM   := nasm
DOTNET := dotnet
PY     := python3

BUILD        := build
OBJ_DIR      := $(BUILD)/obj
LIB_DIR      := $(BUILD)/lib
BOOT_BIN     := $(BUILD)/boot.bin
STAGE2_BIN   := $(BUILD)/stage2.bin
RUNTIME_BIN  := $(BUILD)/mmuko-runtime.bin
IMAGE_TOOL   := $(BUILD)/mkmmukoimg
DISK_IMG     := mmuko-os.img

CFLAGS   := -std=c11 -Wall -Wextra -fPIC -O2
CXXFLAGS := -std=c++17 -Wall -Wextra -fPIC -O2
LDFLAGS  := -shared -lm

C_SRCS  := heartfull_membrane.c bzy_mpda.c tripartite_discriminant.c
C_OBJS  := $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.o))
FW_LIB  := $(LIB_DIR)/libnsigii_firmware.so
FW_ARC  := $(LIB_DIR)/libnsigii_firmware.a
CPP_LIB := $(LIB_DIR)/libnsigii_firmware_cpp.so

.PHONY: all
all: dirs firmware boot image
	@echo ""
	@echo "MMUKO-OS build complete."
	@echo "  Firmware : $(FW_LIB)"
	@echo "  Boot     : $(BOOT_BIN)"
	@echo "  Stage-2  : $(STAGE2_BIN)"
	@echo "  Runtime  : $(RUNTIME_BIN)"
	@echo "  Image    : $(DISK_IMG)"

.PHONY: dirs
dirs:
	@$(PY) -c "import os; [os.makedirs(d, exist_ok=True) for d in ['$(OBJ_DIR)', '$(LIB_DIR)', '$(BUILD)']]"

.PHONY: install-deps
install-deps:
	@echo "[DEPS] Installing nasm and build-essential..."
	sudo apt-get update -qq
	sudo apt-get install -y nasm build-essential

.PHONY: firmware
firmware: dirs $(FW_LIB) $(FW_ARC)
	@echo "[FIRMWARE] OK: $(FW_LIB)"

$(OBJ_DIR)/%.o: %.c
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(FW_LIB): $(C_OBJS)
	@echo "[LD] $@"
	$(CC) $(LDFLAGS) -o $@ $^

$(FW_ARC): $(C_OBJS)
	@echo "[AR] $@"
	ar rcs $@ $^

.PHONY: firmware-cpp
firmware-cpp: dirs $(CPP_LIB)

$(CPP_LIB): nsigii_cpp_wrapper.cpp $(C_OBJS)
	@echo "[CXX] $@"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -I. -o $@ $^

$(OBJ_DIR)/%.o: %.cpp
	@echo "[CXX obj] $<"
	$(CXX) $(CXXFLAGS) -I. -c $< -o $@

.PHONY: boot
boot: dirs $(BOOT_BIN) $(STAGE2_BIN) $(RUNTIME_BIN) $(IMAGE_TOOL)
	@echo "[BOOT] stage-1, stage-2, runtime, and image tool built"

$(BOOT_BIN): boot.asm | dirs
	@echo "[NASM] $< -> $@"
	$(NASM) -f bin $< -o $@
	@$(PY) -c "b=open('$(BOOT_BIN)','rb').read(); assert len(b)==512, len(b); assert b[510]==0x55 and b[511]==0xAA, 'missing boot signature'; print('[BOOT] stage-1 verified: 512 bytes')"

$(STAGE2_BIN): mmuko-boot/stage2.asm | dirs
	@echo "[NASM] $< -> $@"
	$(NASM) -f bin $< -o $@

$(RUNTIME_BIN): mmuko-boot/runtime.asm | dirs
	@echo "[NASM] $< -> $@"
	$(NASM) -f bin $< -o $@

$(IMAGE_TOOL): mmuko-boot/mkimage.c mmuko-boot/image_layout.h | dirs
	@echo "[CC] $< -> $@"
	$(CC) -std=c11 -Wall -Wextra -O2 -o $@ mmuko-boot/mkimage.c

.PHONY: image
image: $(DISK_IMG)

$(DISK_IMG): $(BOOT_BIN) $(STAGE2_BIN) $(RUNTIME_BIN) $(IMAGE_TOOL)
	@echo "[IMAGE] Building raw MMUKO image..."
	$(IMAGE_TOOL) $(BOOT_BIN) $(STAGE2_BIN) $(RUNTIME_BIN) $(DISK_IMG)

.PHONY: run
run: $(DISK_IMG)
	@echo "[QEMU] Booting MMUKO-OS..."
	qemu-system-x86_64 -drive format=raw,file=$(DISK_IMG) -m 32M || echo "[QEMU] Not found - https://www.qemu.org"

.PHONY: compositor
compositor: firmware
	@echo "[DOTNET] Building C# compositor..."
	@$(PY) -c "import shutil,sys; d=shutil.which('dotnet'); sys.exit(0) if d else (print('[DOTNET] Not found - https://dot.net'), sys.exit(0))"
	$(DOTNET) build mmuko-compositor.csproj -c Release --nologo -v quiet && echo "[DOTNET] Build complete." || echo "[DOTNET] Build failed - check errors above."

.PHONY: run-compositor
run-compositor:
	@echo "[COMPOSITOR] dev boot-gate bypass with explicit PASS inputs..."
	$(DOTNET) run --project mmuko-compositor.csproj -- --simulate-pass --tier1 yes --tier2 yes --w-actor yes

.PHONY: run-compositor-pass
run-compositor-pass:
	@echo "[COMPOSITOR] explicit PASS path (boot-passed + T1/T2/W = yes)..."
	$(DOTNET) run --project mmuko-compositor.csproj -- --boot-passed true --tier1 yes --tier2 yes --w-actor yes

.PHONY: run-compositor-maybe
run-compositor-maybe:
	@echo "[COMPOSITOR] diagnostic HOLD path (boot-passed + unresolved inputs)..."
	$(DOTNET) run --project mmuko-compositor.csproj -- --boot-passed true --tier1 maybe --tier2 maybe --w-actor maybe

.PHONY: verify
verify: $(BOOT_BIN) $(STAGE2_BIN) $(RUNTIME_BIN)
	@echo ""
	@echo "=== MMUKO boot image verification ==="
	@$(PY) -c "from pathlib import Path; files=['$(BOOT_BIN)','$(STAGE2_BIN)','$(RUNTIME_BIN)']; [print(f'    {name}: {Path(name).stat().st_size} bytes') for name in files]"
	@$(PY) -c "import struct; b=open('$(RUNTIME_BIN)','rb').read(); magic, sig = struct.unpack_from('<II', b, 0); entry = struct.unpack_from('<I', b, 16)[0]; load = struct.unpack_from('<I', b, 20)[0]; print(f'    runtime magic=0x{magic:08X} signature=0x{sig:08X} entry_offset={entry} load=0x{load:08X}')"
	@echo "=== VERIFIED ==="

.PHONY: clean
clean:
	@echo "[CLEAN] Removing $(BUILD)/ and $(DISK_IMG)..."
	@$(PY) -c "import os, shutil; shutil.rmtree('$(BUILD)', ignore_errors=True); [os.remove(p) for p in ['$(DISK_IMG)'] if os.path.exists(p)]; print('[CLEAN] Done.')"

.PHONY: help
help:
	@echo "MMUKO-OS build targets"
	@echo "  make all      - build firmware plus stage-1/stage-2/runtime image"
	@echo "  make boot     - assemble stage-1, stage-2, runtime, and image tool"
	@echo "  make image    - write all boot artifacts into mmuko-os.img"
	@echo "  make verify   - inspect stage binaries and runtime header"
