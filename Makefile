# Makefile - MMUKO-OS / NSIGII Heartfull Firmware
# OBINexus Computing | Nnamdi Michael Okpala | 20 March 2026

CC        := gcc
CXX       := g++
NASM      := nasm
DOTNET    := dotnet
PY        := python3
MINGW_CC  ?= x86_64-w64-mingw32-gcc
AR        := ar

BUILD        := build
OBJ_DIR      := $(BUILD)/obj
OBJ_WIN_DIR  := $(BUILD)/obj-win
BIN_DIR      := $(BUILD)/bin
LIB_DIR      := $(BUILD)/lib
TOOLS_DIR    := $(BUILD)

BOOT_BIN      := $(BUILD)/boot.bin
DISK_IMG      := mmuko-os.img
STAGE2_BIN    := $(TOOLS_DIR)/mmuko-stage2-loader
SOM_PACKER    := $(TOOLS_DIR)/mmuko-som-pack
FW_SO         := $(LIB_DIR)/libnsigii_firmware.so
FW_ARC        := $(LIB_DIR)/libnsigii_firmware.a
FW_CPP_SO     := $(LIB_DIR)/libnsigii_firmware_cpp.so
FW_DLL        := $(LIB_DIR)/nsigii_firmware.dll
FW_DLL_IMPLIB := $(LIB_DIR)/libnsigii_firmware.dll.a
FW_SOM        := $(LIB_DIR)/nsigii_firmware.som

CFLAGS      := -std=c11 -Wall -Wextra -Wpedantic -fPIC -O2
CXXFLAGS    := -std=c++17 -Wall -Wextra -Wpedantic -fPIC -O2
WIN_CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2
SO_LDFLAGS  := -shared -lm
DLL_LDFLAGS := -shared -Wl,--out-implib,$(FW_DLL_IMPLIB) -lm

C_SRCS      := heartfull_membrane.c bzy_mpda.c tripartite_discriminant.c
C_OBJS      := $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.o))
WIN_C_OBJS  := $(addprefix $(OBJ_WIN_DIR)/,$(C_SRCS:.c=.o))

.PHONY: all dirs install-deps firmware firmware-so firmware-archive firmware-cpp \
        firmware-dll firmware-som boot stage2 image run compositor run-compositor \
        run-compositor-pass run-compositor-maybe verify clean help

all: firmware-so firmware-archive firmware-cpp stage2 firmware-som boot image
	@echo ""
	@echo "MMUKO-OS build complete."
	@echo "  Boot sector  : $(BOOT_BIN)"
	@echo "  Stage-2      : $(STAGE2_BIN)"
	@echo "  Firmware .so : $(FW_SO)"
	@echo "  Firmware .a  : $(FW_ARC)"
	@echo "  C++ wrapper  : $(FW_CPP_SO)"
	@echo "  Firmware .som: $(FW_SOM)"
	@echo "  Disk image   : $(DISK_IMG)"
	@echo ""
	@echo "Optional targets: make firmware-dll, make compositor, make run"

dirs:
	@$(PY) -c "import os; [os.makedirs(d, exist_ok=True) for d in ['$(OBJ_DIR)', '$(OBJ_WIN_DIR)', '$(BIN_DIR)', '$(LIB_DIR)']]"

install-deps:
	@echo "[DEPS] Installing nasm and build-essential..."
	sudo apt-get update -qq
	sudo apt-get install -y nasm build-essential

firmware: firmware-so firmware-archive

firmware-so: dirs $(FW_SO)
	@echo "[FIRMWARE] ELF shared object ready: $(FW_SO)"

firmware-archive: dirs $(FW_ARC)
	@echo "[FIRMWARE] Static archive ready: $(FW_ARC)"

firmware-cpp: dirs $(FW_CPP_SO)
	@echo "[FIRMWARE] C++ wrapper ready: $(FW_CPP_SO)"

firmware-dll: dirs
	@$(PY) -c "import shutil,sys; exe=shutil.which('$(MINGW_CC)'); print('[DLL] compiler:', exe or 'missing'); sys.exit(0 if exe else 1)"
	@$(MAKE) $(FW_DLL)
	@echo "[FIRMWARE] Windows DLL ready: $(FW_DLL)"

firmware-som: dirs $(FW_SOM)
	@echo "[FIRMWARE] MMUKO container ready: $(FW_SOM)"

boot: dirs $(BOOT_BIN)

stage2: dirs $(STAGE2_BIN)
	@echo "[STAGE2] Reference loader ready: $(STAGE2_BIN)"

$(OBJ_DIR)/%.o: %.c
	@echo "[CC] $<"
	@$(CC) $(CFLAGS) -I. -c $< -o $@

$(OBJ_WIN_DIR)/%.o: %.c
	@echo "[MINGW-CC] $<"
	@$(MINGW_CC) $(WIN_CFLAGS) -I. -c $< -o $@

$(FW_SO): $(C_OBJS)
	@echo "[LD] $@"
	@$(CC) $(SO_LDFLAGS) -o $@ $^

$(FW_ARC): $(C_OBJS)
	@echo "[AR] $@"
	@$(AR) rcs $@ $^

$(FW_CPP_SO): nsigii_cpp_wrapper.cpp $(C_OBJS)
	@echo "[CXX] $@"
	@$(CXX) $(CXXFLAGS) $(SO_LDFLAGS) -I. -o $@ $^

$(FW_DLL): $(WIN_C_OBJS)
	@echo "[DLL] $@"
	@$(MINGW_CC) $(DLL_LDFLAGS) -I. -o $@ $^

$(SOM_PACKER): mmuko-boot/mmuko-som-pack.c mmuko-boot/mmuko_som.h
	@echo "[CC] $@"
	@$(CC) $(CFLAGS) -Immuko-boot -o $@ mmuko-boot/mmuko-som-pack.c

$(STAGE2_BIN): mmuko-boot/stage2_loader.c mmuko-boot/mmuko_som.h
	@echo "[CC] $@"
	@$(CC) $(CFLAGS) -Immuko-boot -o $@ mmuko-boot/stage2_loader.c

$(FW_SOM): $(FW_SO) $(SOM_PACKER)
	@echo "[PACK] $@"
	@$(SOM_PACKER) $(FW_SO) $@ elf-so

$(BOOT_BIN): boot.asm
	@echo "[NASM] boot.asm -> $(BOOT_BIN)"
	@$(PY) -c "import shutil,sys; nasm=shutil.which('$(NASM)'); sys.exit(0 if nasm else 1)"
	@$(NASM) -f bin boot.asm -o $(BOOT_BIN)
	@$(PY) -c "b=open('$(BOOT_BIN)','rb').read();assert len(b)==512,'size: '+str(len(b));assert b[510:512]==b'\x55\xAA','bad signature';print('[BOOT] validated 512-byte boot sector with 0x55AA signature')"

image: $(DISK_IMG)

$(DISK_IMG): $(BOOT_BIN)
	@echo "[IMAGE] Writing $(DISK_IMG) (1.44 MB FAT12)..."
	@$(PY) -c "boot=open('$(BOOT_BIN)','rb').read(); img=bytearray(b'\x00'*(512*2880)); img[:512]=boot; open('$(DISK_IMG)','wb').write(img); print('[IMAGE] wrote', len(img), 'bytes to $(DISK_IMG)')"

run: $(DISK_IMG)
	@echo "[QEMU] Booting MMUKO-OS..."
	@qemu-system-x86_64 -drive format=raw,file=$(DISK_IMG) -m 32M || echo "[QEMU] Not found - https://www.qemu.org"

compositor: firmware-so
	@$(PY) -c "import shutil,sys; d=shutil.which('$(DOTNET)'); sys.exit(0) if d else (print('[DOTNET] Not found - https://dot.net'), sys.exit(0))"
	$(DOTNET) build mmuko-compositor.csproj -c Release --nologo -v quiet && echo "[DOTNET] Build complete." || echo "[DOTNET] Build failed - check errors above."

run-compositor:
	$(DOTNET) run --project mmuko-compositor.csproj -- --simulate-pass --tier1 yes --tier2 yes --w-actor yes

run-compositor-pass:
	$(DOTNET) run --project mmuko-compositor.csproj -- --boot-passed true --tier1 yes --tier2 yes --w-actor yes

run-compositor-maybe:
	$(DOTNET) run --project mmuko-compositor.csproj -- --boot-passed true --tier1 maybe --tier2 maybe --w-actor maybe

verify: firmware-so firmware-som boot stage2
	@echo "=== MMUKO verification ==="
	@$(PY) -c "import os,struct; b=open('$(BOOT_BIN)','rb').read(); assert len(b)==512; assert b[510:512]==b'\x55\xAA'; print('boot.bin OK')"
	@$(STAGE2_BIN) $(FW_SOM)
	@$(PY) -c "import ctypes; h=open('$(FW_SOM)','rb').read(64); print('som magic:', h[:4].decode())"

clean:
	@echo "[CLEAN] Removing build artifacts..."
	@$(PY) -c "import os,shutil; shutil.rmtree('$(BUILD)', ignore_errors=True); [os.remove(p) for p in ['$(DISK_IMG)'] if os.path.exists(p)]"

help:
	@echo "MMUKO-OS / NSIGII Heartfull Firmware - Build System"
	@echo ""
	@echo "Named artifact targets:"
	@echo "  make boot           -> $(BOOT_BIN)"
	@echo "  make stage2         -> $(STAGE2_BIN)"
	@echo "  make firmware-so    -> $(FW_SO)"
	@echo "  make firmware-dll   -> $(FW_DLL) (requires $(MINGW_CC))"
	@echo "  make firmware-som   -> $(FW_SOM)"
	@echo "  make image          -> $(DISK_IMG)"
	@echo "  make firmware-cpp   -> $(FW_CPP_SO)"
	@echo "  make firmware       -> $(FW_SO) + $(FW_ARC)"
	@echo ""
	@echo "Container tooling:"
	@echo "  $(SOM_PACKER) wraps ELF/DLL/raw payloads in the custom MSOM header"
	@echo "  $(STAGE2_BIN) validates MSOM headers and payload CRC32"
