# Makefile - MMUKO-OS / NSIGII Heartfull Firmware
# BIOS hard-disk target: MBR + FAT16 VBR + stage-2 loader + kernel payload

CC     := gcc
CXX    := g++
NASM   := nasm
DOTNET := dotnet
PY     := python3

BUILD    := build
OBJ_DIR  := $(BUILD)/obj
LIB_DIR  := $(BUILD)/lib
DISK_IMG := $(BUILD)/mmuko-os.img

CFLAGS   := -std=c11 -Wall -Wextra -fPIC -O2
CXXFLAGS := -std=c++17 -Wall -Wextra -fPIC -O2
LDFLAGS  := -shared -lm

C_SRCS  := heartfull_membrane.c bzy_mpda.c tripartite_discriminant.c
C_OBJS  := $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.o))
FW_LIB  := $(LIB_DIR)/libnsigii_firmware.so
FW_ARC  := $(LIB_DIR)/libnsigii_firmware.a
CPP_LIB := $(LIB_DIR)/libnsigii_firmware_cpp.so

.PHONY: all
all: dirs firmware image
	@echo ""
	@echo "MMUKO-OS build complete."
	@echo "  Firmware : $(FW_LIB)"
	@echo "  Archive  : $(FW_ARC)"
	@echo "  Image    : $(DISK_IMG)"
	@echo ""
	@echo "  make run            - boot image in QEMU"
	@echo "  make run-compositor - run C# compositor (dev PASS bypass)"

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

.PHONY: boot
boot: dirs
	@echo "[BOOT] Assembling BIOS boot chain..."
	@$(PY) -c "import shutil,sys; sys.exit(0) if shutil.which('nasm') else (print('[ERROR] nasm not found.'), sys.exit(1))"
	@nasm -f bin boot/mbr.asm -o $(BUILD)/mbr.bin
	@nasm -f bin boot/vbr.asm -o $(BUILD)/vbr.bin
	@nasm -f bin boot/stage2.asm -o $(BUILD)/stage2.bin
	@nasm -f bin boot/kernel.asm -o $(BUILD)/kernel.bin
	@$(PY) -c "from pathlib import Path; req=['mbr.bin','vbr.bin','stage2.bin','kernel.bin']; [print(f'[BOOT] {name}: {(Path('$(BUILD)')/name).stat().st_size} bytes') for name in req]"

.PHONY: image
image: dirs
	@echo "[IMAGE] Building BIOS disk image with MBR + FAT16..."
	@$(PY) tools/build_image.py

.PHONY: run
run: image
	@echo "[QEMU] Booting MMUKO-OS BIOS image..."
	@qemu-system-x86_64 -drive format=raw,file=$(DISK_IMG) -m 32M || echo "[QEMU] Not found - install qemu-system-x86_64 to boot the image."

.PHONY: compositor
compositor: firmware
	@echo "[DOTNET] Building C# compositor..."
	@if command -v $(DOTNET) >/dev/null 2>&1; then \
		$(DOTNET) build mmuko-compositor.csproj -c Release --nologo -v quiet && echo "[DOTNET] Build complete."; \
	else \
		echo "[DOTNET] Not found - https://dot.net"; \
	fi

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
verify: image
	@echo ""
	@echo "=== MMUKO BIOS Image Verification ==="
	@$(PY) tools/build_image.py >/tmp/mmuko-image-build.log && tail -n 6 /tmp/mmuko-image-build.log
	@echo "=== VERIFY_OK ==="
	@echo ""

.PHONY: clean
clean:
	@echo "[CLEAN] Removing $(BUILD)/ ..."
	@$(PY) -c "import shutil; shutil.rmtree('$(BUILD)', ignore_errors=True)"
	@$(PY) -c "from pathlib import Path; [Path(p).unlink() for p in ['$(DISK_IMG)'] if Path(p).exists()]"
	@echo "[CLEAN] Done."

.PHONY: help
help:
	@echo ""
	@echo "MMUKO-OS / NSIGII Heartfull Firmware - Build System"
	@echo "Selected boot target: BIOS hard disk image with MBR + FAT16 + stage-2 loader"
	@echo ""
	@echo "  make firmware             C firmware .so + .a"
	@echo "  make firmware-cpp         C++ wrapper .so"
	@echo "  make boot                 assemble the MBR/VBR/stage2/kernel artifacts"
	@echo "  make image                build the FAT16 BIOS image under tools/"
	@echo "  make run                  boot the disk image in QEMU"
	@echo "  make compositor           dotnet build"
	@echo "  make verify               rebuild + verify the image layout"
	@echo "  make clean                remove build/ artifacts and the generated disk image"
	@echo ""
