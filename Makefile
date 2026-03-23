# Makefile - MMUKO-OS / NSIGII Heartfull Firmware
# OBINexus Computing | Nnamdi Michael Okpala | 20 March 2026
#
# Works on: WSL/Linux (make) and Windows native (make from Git Bash/MSYS2).
# All source files flat in the same directory.
#
# Targets:
#   make all               build firmware + boot + image
#   make install-deps      install nasm in WSL/Ubuntu (run once)
#   make firmware          compile C firmware -> .so + .a
#   make firmware-cpp      compile C++ wrapper
#   make boot              assemble generated stage-1 source -> build/boot.bin
#   make image             write boot sector into mmuko-os.img
#   make cython-build      build Python wheel + sdist
#   make cython-develop    install editable Cython package
#   make run-ui            run Python console compositor
#   make run               boot with QEMU
#   make verify            NSIGII verification checks
#   make clean             remove build/ directory

# ============================================================
# Toolchain
# ============================================================
CC     := gcc
CXX    := g++
NASM   := nasm
# python3 is used for cross-platform mkdir, image writing, packaging, and verify.
# It is available on both Windows (Conda/Python install) and WSL.
PY     := python3
PIP    := $(PY) -m pip
BUILD_PY := $(PY) -m build

# ============================================================
# Build output directories
# ============================================================
BUILD    := build
OBJ_DIR  := $(BUILD)/obj
LIB_DIR  := $(BUILD)/lib
BOOT_BIN := $(BUILD)/boot.bin
DISK_IMG := mmuko-os.img

CODEGEN_SCRIPT := tools/mmuko_codegen/generate.py
PSC_DIR := mmuko-boot/pseudocode
PRIMARY_PSC := $(PSC_DIR)/mmuko-boot.psc
CODEGEN_STAMP := $(BUILD)/mmuko_codegen.stamp
GENERATED_BOOT_SRC := boot/mmuko_stage1_boot.asm
GENERATED_STAGE2_C := kernel/mmuko_stage2_loader.c
GENERATED_STAGE2_CPP := kernel/mmuko_stage2_bridge.cpp
GENERATED_HEADER := include/mmuko_codegen.h
GENERATED_CYTHON := python/mmuko_codegen.pxd python/mmuko_generated.pyx

# ============================================================
# Compiler flags
# ============================================================
CFLAGS   := -std=c11 -Wall -Wextra -fPIC -O2
CXXFLAGS := -std=c++17 -Wall -Wextra -fPIC -O2
LDFLAGS  := -shared -lm

# ============================================================
# Source files (flat - all in current directory)
# ============================================================
C_SRCS  := heartfull_membrane.c bzy_mpda.c tripartite_discriminant.c $(GENERATED_STAGE2_C)
C_OBJS  := $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.o))
CPP_SRCS := nsigii_cpp_wrapper.cpp $(GENERATED_STAGE2_CPP)
CPP_OBJS := $(addprefix $(OBJ_DIR)/,$(CPP_SRCS:.cpp=.o))
FW_LIB  := $(LIB_DIR)/libnsigii_firmware.so
FW_ARC  := $(LIB_DIR)/libnsigii_firmware.a
CPP_LIB := $(LIB_DIR)/libnsigii_firmware_cpp.so

.PHONY: all
all: codegen dirs firmware boot image
	@echo ""
	@echo "MMUKO-OS build complete."
	@echo "  Firmware : $(FW_LIB)"
	@echo "  Stage-1  : $(STAGE1_BIN)"
	@echo "  Stage-2  : $(STAGE2_BIN)"
	@echo "  Image    : $(DISK_IMG)"
	@echo ""
	@echo "  make cython-build   - build Python wheel + sdist"
	@echo "  make cython-develop - install editable package"
	@echo "  make run-ui         - run Python console compositor"
	@echo "  make run            - boot in QEMU"


# ============================================================
# Code generation from canonical spec + pseudocode
# ============================================================
.PHONY: codegen
codegen: dirs $(CODEGEN_STAMP)

$(CODEGEN_STAMP): MMUKO-OS.txt $(CODEGEN_SCRIPT) $(PRIMARY_PSC) $(PSC_DIR)
	@echo "[CODEGEN] Generating MMUKO-OS derived sources..."
	$(PY) $(CODEGEN_SCRIPT) --root . --spec MMUKO-OS.txt --primary $(PRIMARY_PSC) --pseudocode-dir $(PSC_DIR)
	@$(PY) -c "from pathlib import Path; Path('$(CODEGEN_STAMP)').write_text('generated\n', encoding='utf-8')"

.PHONY: dirs
dirs:
	@$(PY) -c "import os; [os.makedirs(d, exist_ok=True) for d in ['$(OBJ_DIR)', '$(LIB_DIR)', '$(BOOT_DIR)']]"

.PHONY: install-deps
install-deps:
	@echo "[DEPS] Installing nasm and build-essential..."
	sudo apt-get update -qq
	sudo apt-get install -y nasm build-essential

.PHONY: firmware
firmware: codegen dirs $(FW_LIB) $(FW_ARC)
	@echo "[FIRMWARE] OK: $(FW_LIB)"

$(OBJ_DIR)/%.o: %.c | dirs
	@echo "[CC] $<"
	@$(PY) -c "from pathlib import Path; Path('$@').parent.mkdir(parents=True, exist_ok=True)"
	$(CC) $(CFLAGS) -I. -Iinclude -c $< -o $@

$(FW_LIB): $(C_OBJS) | dirs
	@echo "[LD] $@"
	@$(CC) $(SO_LDFLAGS) -o $@ $^

$(FW_ARC): $(C_OBJS) | dirs
	@echo "[AR] $@"
	@$(AR) rcs $@ $^

.PHONY: firmware-cpp
firmware-cpp: codegen dirs $(CPP_LIB)

$(CPP_LIB): $(CPP_OBJS) $(C_OBJS)
	@echo "[CXX] $@"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -I. -Iinclude -o $@ $^

$(OBJ_DIR)/%.o: %.cpp
	@echo "[CXX obj] $<"
	@$(PY) -c "from pathlib import Path; Path('$@').parent.mkdir(parents=True, exist_ok=True)"
	$(CXX) $(CXXFLAGS) -I. -Iinclude -c $< -o $@

.PHONY: boot
boot: codegen dirs $(BOOT_BIN)

$(BOOT_BIN): $(GENERATED_BOOT_SRC)
	@echo "[NASM] $(GENERATED_BOOT_SRC) -> $(BOOT_BIN)"
	@$(PY) -c "import shutil,sys; nasm=shutil.which('nasm'); sys.exit(0) if nasm else (print('[ERROR] nasm not found.'), print('  WSL:     make install-deps'), print('  Windows: use pre-built boot.bin (already included)'), sys.exit(1))"
	$(NASM) -f bin $(GENERATED_BOOT_SRC) -o $(BOOT_BIN)
	@$(PY) -c "b=open('$(BOOT_BIN)','rb').read();assert len(b)==512,'size: '+str(len(b));sig=b[510]|(b[511]<<8);assert sig==0xAA55,'sig: '+hex(sig);print('[BOOT] '+str(len(b))+' bytes  sig=0x'+format(sig,'04X')+'  OK')"

# ============================================================
# Disk image (1.44 MB FAT12 - cross-platform Python write)
# Uses boot.bin if present; falls back to pre-built boot.bin
# ============================================================
.PHONY: image
image: $(DISK_IMG)

$(DISK_IMG): $(STAGE1_BIN) $(STAGE2_BIN) | dirs
	@echo "[IMAGE] Writing $(DISK_IMG) (1.44 MB FAT12 + raw stage2 payload)..."
	@$(PY) -c "from pathlib import Path; stage1=Path('$(STAGE1_BIN)').read_bytes(); stage2=Path('$(STAGE2_BIN)').read_bytes(); img=bytearray(b'\0'*(512*2880)); img[:512]=stage1; img[512:512+len(stage2)]=stage2; Path('$(DISK_IMG)').write_bytes(img); print(f'[IMAGE] wrote {len(img)} bytes with stage2 payload {len(stage2)} bytes')"

.PHONY: run
run: $(DISK_IMG)
	@echo "[QEMU] Booting MMUKO-OS..."
	qemu-system-x86_64 -drive format=raw,file=$(DISK_IMG) -m 32M || echo "[QEMU] Not found - https://www.qemu.org"

# ============================================================
# Python / Cython package
# ============================================================
.PHONY: cython-build
cython-build: firmware
	@echo "[PYTHON] Building sdist + wheel..."
	$(PIP) install --quiet build Cython
	$(BUILD_PY)

.PHONY: cython-develop
cython-develop: firmware
	@echo "[PYTHON] Installing editable package..."
	$(PIP) install --quiet -e .

.PHONY: run-ui
run-ui: cython-develop
	@echo "[UI] Running Python console compositor..."
	PYTHONPATH=python $(PY) -m mmuko_os --tier1 yes --tier2 yes --w-actor yes

.PHONY: verify
verify: boot
	@echo ""
	@echo "=== NSIGII Verification ==="
	@echo ""
	@echo "[1] Stage-1 boot sector"
	@$(PY) -c "import struct; b=open('$(STAGE1_BIN)','rb').read(); assert len(b)==512; assert struct.unpack_from('<H',b,510)[0]==0xAA55; print('    stage1: 512 bytes / sig=0xAA55')"
	@echo "[2] Stage-2 payload"
	@$(PY) -c "b=open('$(STAGE2_BIN)','rb').read(); assert len(b)==$(STAGE2_TOTAL_BYTES); print('    stage2: '+str(len(b))+' bytes / sectors=$(STAGE2_TOTAL_SECTORS)')"
	@echo "[3] Trinary alphabet"
	@$(PY) -c "[print('    '+n+' = '+str(v)) for n,v in [('YES',1),('NO',0),('MAYBE',-1),('MAYBE_NOT',-2)]]"
	@echo ""
	@echo "=== NSIGII_VERIFIED ==="

.PHONY: clean
clean:
	@echo "[CLEAN] Removing generated artifacts ..."
	@$(PY) -c "import shutil, pathlib; [shutil.rmtree(p, ignore_errors=True) for p in ['build', 'bin', 'obj', 'legacy/csharp-compositor/bin', 'legacy/csharp-compositor/obj']]; [pathlib.Path(p).unlink(missing_ok=True) for p in ['boot.bin', 'boot-stage2.bin', 'mmuko-os.img']]; print('[CLEAN] Done.')"

.PHONY: help
help:
	@echo "MMUKO-OS / NSIGII Heartfull Firmware - Build System"
	@echo "Primary workflow: Python/Cython bindings and console UI"
	@echo "OBINexus Computing | Nnamdi Michael Okpala"
	@echo ""
	@echo "FIRST TIME SETUP (WSL/Ubuntu):"
	@echo "  make install-deps       <- installs nasm + build-essential"
	@echo "  make all                <- builds everything"
	@echo ""
	@echo "WINDOWS (PowerShell / native make):"
	@echo "  make firmware           <- GCC compiles fine via conda"
	@echo "  make image              <- uses pre-built boot.bin"
	@echo "  make cython-develop     <- install editable Python package"
	@echo "  make run                <- QEMU boot"
	@echo "  make run-ui             <- Python console compositor"
	@echo "  boot: run from WSL (nasm not in conda path)"
	@echo ""
	@echo "ALL TARGETS:"
	@echo "  make all                  firmware + boot + image"
	@echo "  make install-deps         nasm + build-essential (WSL)"
	@echo "  make firmware             C firmware .so + .a"
	@echo "  make firmware-cpp         C++ wrapper .so"
	@echo "  make boot                 boot/mmuko_stage1_boot.asm -> build/boot.bin (WSL)"
	@echo "  make image                boot sector -> mmuko-os.img"
	@echo "  make cython-build         build Python wheel + sdist"
	@echo "  make cython-develop       install editable package"
	@echo "  make run-ui               Python console compositor"
	@echo "  make run                  QEMU boot"
	@echo "  make verify               NSIGII checks"
	@echo "  make clean                remove build/"
	@echo ""
	@echo "  make all            - build firmware, stage1, stage2, image"
	@echo "  make boot           - assemble split boot chain"
	@echo "  make image          - write stage1 + raw stage2 payload into build/mmuko-os.img"
	@echo "  make run            - boot build/mmuko-os.img with QEMU"
	@echo "  make compositor     - build the legacy C# compositor"
	@echo "  make verify         - verify stage sizes and signatures"
	@echo "  make clean          - remove generated artifacts and legacy .NET outputs"
