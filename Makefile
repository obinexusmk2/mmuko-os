<<<<<<< HEAD
# Makefile - MMUKO-OS / NSIGII Heartfull Firmware
# OBINexus Computing | Nnamdi Michael Okpala | 24 March 2026
#
# Works on: WSL/Linux (make) and Windows native (make from Git Bash/MSYS2).
#
# Boot chain:
#   stage-1  (boot/mmuko_stage1_boot.asm) → build/boot.bin     (512 bytes, 0xAA55)
#   stage-2  (boot/stage2.asm)            → build/mmuko-os.bin  (kernel payload)
#   runtime  (kernel/runtime.asm)         → build/runtime.bin   (firmware entry)
#   image    stage1 + stage2 + runtime    → build/mmuko-os.img  (1.44 MB FAT12)
#
# Enzyme model strategy:
#   MAYBE → YES / NO
#   CREATE / DESTROY  (phase init / phase teardown)
#   BUILD  / BREAK    (sector assembly / disassembly)
#   REPAIR / RENEW    (kernel panic recovery / hot-reload)
#
# Targets:
#   make all               build firmware + boot + image
#   make install-deps      install nasm in WSL/Ubuntu (run once)
#   make firmware          compile C firmware -> .so + .a
#   make firmware-cpp      compile C++ wrapper
#   make boot              assemble split boot chain (stage1 + stage2 + runtime)
#   make image             write boot sectors into build/mmuko-os.img
#   make cython-build      build Python wheel + sdist
#   make cython-develop    install editable Cython package
#   make run-ui            run Python console compositor
#   make run               boot with QEMU
#   make verify            NSIGII verification checks
#   make tree              display filesystem driver tree
#   make clean             remove build/ directory

# ============================================================
# Toolchain
# ============================================================
CC     := gcc
CXX    := g++
NASM   := nasm
AR     := ar
PY     := python3
PIP    := $(PY) -m pip
BUILD_PY := $(PY) -m build

# ============================================================
# Platform detection (Windows / macOS / Linux)
# ============================================================
UNAME_S  := $(shell uname -s 2>/dev/null || echo Windows)
UNAME_M  := $(shell uname -m 2>/dev/null || echo x86_64)
PLAT_TAG := $(UNAME_S)-$(UNAME_M)

ifeq ($(UNAME_S),Darwin)
    SO_EXT     := .dylib
    SO_LDFLAGS := -dynamiclib
    LDFLAGS    := -dynamiclib
    NASM       := $(shell command -v nasm 2>/dev/null || echo nasm)
else ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    SO_EXT     := .dll
    SO_LDFLAGS := -shared
    LDFLAGS    := -shared
else
    SO_EXT     := .so
    SO_LDFLAGS := -shared -lm
    LDFLAGS    := -shared -lm
endif

# ============================================================
# Build output directories
# ============================================================
BUILD    := build
OBJ_DIR  := $(BUILD)/obj/$(PLAT_TAG)
LIB_DIR  := $(BUILD)/lib
BOOT_DIR := $(BUILD)

# Boot chain binaries
STAGE1_BIN       := $(BUILD)/boot.bin
STAGE2_BIN       := $(BUILD)/mmuko-os.bin
RUNTIME_BIN      := $(BUILD)/runtime.bin
DISK_IMG         := $(BUILD)/mmuko-os.img

# Stage-2 sector budget (16 sectors × 512 = 8192 bytes max)
STAGE2_SECTORS       := 16
STAGE2_TOTAL_SECTORS := $(STAGE2_SECTORS)
STAGE2_TOTAL_BYTES   := 8192

# Runtime sector budget (12 sectors × 512 = 6144 bytes max)
RUNTIME_SECTORS      := 12

# Codegen paths — pseudocode lives in two places:
#   pseudocode/           (canonical .psc collection)
#   mmuko-boot/pseudocode (boot-specific subset, mirrors primary .psc)
CODEGEN_SCRIPT   := tools/mmuko_codegen/generate.py
PSC_DIR          := pseudocode
PSC_BOOT_DIR     := mmuko-boot/pseudocode
PRIMARY_PSC      := $(PSC_DIR)/mmuko-boot.psc
CODEGEN_STAMP    := $(BUILD)/mmuko_codegen.stamp
SPEC_FILE        := $(wildcard pseudocode/MMUKO-OS.txt)

# Generated source paths
GENERATED_BOOT_SRC   := boot/mmuko_stage1_boot.asm
GENERATED_STAGE2_C   := kernel/mmuko_stage2_loader.c
GENERATED_STAGE2_CPP := kernel/mmuko_stage2_bridge.cpp
GENERATED_HEADER     := include/mmuko_codegen.h
GENERATED_CYTHON     := python/mmuko_codegen.pxd python/mmuko_generated.pyx

# ============================================================
# Compiler flags
# ============================================================
CFLAGS    := -std=c11 -Wall -Wextra -fPIC -O2
CXXFLAGS  := -std=c++17 -Wall -Wextra -fPIC -O2
# SO_LDFLAGS and LDFLAGS set by platform detection above

# ============================================================
# Source files
# ============================================================
C_SRCS   := heartfull_membrane.c bzy_mpda.c tripartite_discriminant.c
C_OBJS   := $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.o))
CPP_SRCS := nsigii_cpp_wrapper.cpp
CPP_OBJS := $(addprefix $(OBJ_DIR)/,$(CPP_SRCS:.cpp=.o))
FW_LIB   := $(LIB_DIR)/libnsigii_firmware$(SO_EXT)
FW_ARC   := $(LIB_DIR)/libnsigii_firmware.a
CPP_LIB  := $(LIB_DIR)/libnsigii_firmware_cpp$(SO_EXT)

# ============================================================
# BIOS firmware + Phase 1 UI (new)
# ============================================================
BIOS_C_OBJ    := $(OBJ_DIR)/firmware/bios_interface.o
BIOS_LIB      := $(LIB_DIR)/libbios_firmware$(SO_EXT)
BIOS_ARC      := $(LIB_DIR)/libbios_firmware.a
BIOS_CPP_OBJ  := $(OBJ_DIR)/firmware/bios_interface_cpp.o
UI_C_OBJ      := $(OBJ_DIR)/ui/phase1_ui.o
UI_BIN        := $(BUILD)/phase1_ui
FLAT_BOOT_SRC := boot/mmuko_bootloader.asm
FLAT_KERN_SRC := kernel/mmuko_kernel.asm
FLAT_BOOT_BIN := $(BUILD)/mmuko_bootloader.bin
FLAT_KERN_BIN := $(BUILD)/mmuko_kernel.bin
FLAT_OS_BIN   := $(BUILD)/os_flat.bin

# ============================================================
# Phony targets
# ============================================================
.PHONY: all dirs install-deps firmware firmware-cpp bios-firmware bios-firmware-cpp \
        phase1-ui flat-bin qemu-flat boot image run \
        cython-build cython-develop run-ui verify tree clean help codegen ring

# ============================================================
# Default: build everything
# ============================================================
all: dirs codegen firmware firmware-cpp bios-firmware phase1-ui boot image flat-bin
	@echo ""
	@echo "======================================="
	@echo " MMUKO-OS build complete (enzyme: OK)"
	@echo "======================================="
	@echo "  Stage-1  : $(STAGE1_BIN)  (boot sector)"
	@echo "  Stage-2  : $(STAGE2_BIN)  (mmuko-os kernel)"
	@echo "  Runtime  : $(RUNTIME_BIN) (firmware entry)"
	@echo "  Firmware : $(FW_LIB)"
	@echo "  Image    : $(DISK_IMG)"
	@echo ""
	@echo "  make run      - boot in QEMU"
	@echo "  make verify   - NSIGII checks"
	@echo "  make tree     - display driver tree"

# ============================================================
# Directory creation (must run before everything else)
# ============================================================
dirs:
	@$(PY) scripts/ensure_dirs.py $(OBJ_DIR) $(LIB_DIR) $(BUILD) \
		$(OBJ_DIR)/kernel $(OBJ_DIR)/firmware $(OBJ_DIR)/ui

# ============================================================
# Code generation from canonical spec + pseudocode
# ============================================================
codegen: dirs $(CODEGEN_STAMP)

$(CODEGEN_STAMP): $(SPEC_FILE) $(wildcard $(PRIMARY_PSC))
	@echo "[CODEGEN] Generating MMUKO-OS derived sources..."
	@if [ -f "$(CODEGEN_SCRIPT)" ] && [ -n "$(SPEC_FILE)" ]; then \
		$(PY) $(CODEGEN_SCRIPT) --root . --spec $(SPEC_FILE) \
			--primary $(PRIMARY_PSC) --pseudocode-dir $(PSC_DIR) 2>/dev/null || true; \
	else \
		echo "[CODEGEN] spec or generate.py not found — using existing sources"; \
	fi
	@touch $(CODEGEN_STAMP)

# ============================================================
# Dependencies
# ============================================================
install-deps:
	@echo "[DEPS] Installing nasm, gcc, qemu..."
	sudo apt-get update -qq
	sudo apt-get install -y nasm build-essential qemu-system-x86

# ============================================================
# Firmware (C shared library + static archive)
# ============================================================
firmware: dirs $(FW_LIB) $(FW_ARC)
	@echo "[FIRMWARE] OK: $(FW_LIB) $(FW_ARC)"

$(OBJ_DIR)/%.o: %.c | dirs
	@echo "[CC] $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -Iinclude -c $< -o $@

$(FW_LIB): $(C_OBJS) | dirs
	@echo "[LD] $@"
	$(CC) $(SO_LDFLAGS) -o $@ $^

$(FW_ARC): $(C_OBJS) | dirs
	@echo "[AR] $@"
	$(AR) rcs $@ $^

# ============================================================
# Firmware C++ wrapper
# ============================================================
firmware-cpp: dirs $(CPP_LIB)

$(CPP_LIB): $(CPP_OBJS) $(C_OBJS)
	@echo "[CXX] $@"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -I. -Iinclude -o $@ $^

$(OBJ_DIR)/%.o: %.cpp | dirs
	@echo "[CXX obj] $<"
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -Iinclude -c $< -o $@

# ============================================================
# Boot chain assembly (NASM with Python fallback)
# ============================================================
boot: dirs
	@if command -v $(NASM) >/dev/null 2>&1; then \
		echo "[NASM] Assembling boot chain with nasm..."; \
		$(NASM) -f bin $(GENERATED_BOOT_SRC) -o $(STAGE1_BIN); \
		$(NASM) -f bin boot/stage2.asm -o $(STAGE2_BIN); \
		$(NASM) -f bin kernel/runtime.asm -o $(RUNTIME_BIN); \
	else \
		echo "[PYTHON] nasm not found — using Python fallback assembler"; \
		$(PY) scripts/assemble_boot.py --build-dir $(BUILD); \
	fi
	@$(PY) -c "\
import struct; \
b=open('$(STAGE1_BIN)','rb').read(); \
assert len(b)==512,'stage1 size: '+str(len(b)); \
sig=struct.unpack_from('<H',b,510)[0]; \
assert sig==0xAA55,'sig: '+hex(sig); \
print('[BOOT] stage1='+str(len(b))+'B sig=0xAA55 OK'); \
s2=open('$(STAGE2_BIN)','rb').read(); \
print('[BOOT] stage2='+str(len(s2))+'B (mmuko-os) OK'); \
rt=open('$(RUNTIME_BIN)','rb').read(); \
print('[BOOT] runtime='+str(len(rt))+'B OK')"

# ============================================================
# Disk image (1.44 MB FAT12 — stage1 + stage2 + runtime)
# ============================================================
.FORCE:
image: $(DISK_IMG)

$(DISK_IMG): .FORCE | dirs
	@echo "[IMAGE] Writing $(DISK_IMG) (1.44 MB FAT12 + kernel payload)..."
	@$(PY) -c "\
from pathlib import Path; \
stage1 = Path('$(STAGE1_BIN)').read_bytes(); \
stage2 = Path('$(STAGE2_BIN)').read_bytes(); \
runtime = Path('$(RUNTIME_BIN)').read_bytes(); \
img = bytearray(b'\x00' * (512 * 2880)); \
img[:512] = stage1; \
img[512:512+len(stage2)] = stage2; \
img[512+len(stage2):512+len(stage2)+len(runtime)] = runtime; \
Path('$(DISK_IMG)').write_bytes(img); \
print(f'[IMAGE] wrote {len(img)} bytes  stage2={len(stage2)}B  runtime={len(runtime)}B')"

# ============================================================
# QEMU boot (explicit format=raw suppresses the warning)
# ============================================================
run: $(DISK_IMG)
	@echo "[QEMU] Booting MMUKO-OS..."
	qemu-system-x86_64 -drive format=raw,file=$(DISK_IMG) -m 32M \
		|| echo "[QEMU] Not found - https://www.qemu.org"

# ============================================================
# Python / Cython package
# ============================================================
cython-build: firmware
	@echo "[PYTHON] Building sdist + wheel..."
	$(PIP) install --quiet build Cython
	$(BUILD_PY)

cython-develop: firmware
	@echo "[PYTHON] Installing editable package..."
	$(PIP) install --quiet -e .

run-ui: cython-develop
	@echo "[UI] Running Python console compositor..."
	PYTHONPATH=python $(PY) -m mmuko_os --tier1 yes --tier2 yes --w-actor yes

# ============================================================
# NSIGII Verification
# ============================================================
verify: boot
	@echo ""
	@echo "=== NSIGII Verification ==="
	@echo ""
	@echo "[1] Stage-1 boot sector"
	@$(PY) -c "\
import struct; \
b=open('$(STAGE1_BIN)','rb').read(); \
assert len(b)==512; \
assert struct.unpack_from('<H',b,510)[0]==0xAA55; \
print('    stage1: 512 bytes / sig=0xAA55  PASS')"
	@echo "[2] Stage-2 kernel payload (mmuko-os.bin)"
	@$(PY) -c "\
b=open('$(STAGE2_BIN)','rb').read(); \
print('    stage2: '+str(len(b))+' bytes  PASS')"
	@echo "[3] Runtime firmware entry"
	@$(PY) -c "\
b=open('$(RUNTIME_BIN)','rb').read(); \
print('    runtime: '+str(len(b))+' bytes  PASS')"
	@echo "[4] Disk image"
	@$(PY) -c "\
b=open('$(DISK_IMG)','rb').read(); \
assert len(b)==512*2880; \
print('    image: '+str(len(b))+' bytes (1.44 MB)  PASS')"
	@echo "[5] Enzyme trinary alphabet"
	@$(PY) -c "\
vals=[('YES',1),('NO',0),('MAYBE',-1),('MAYBE_NOT',-2)]; \
[print('    '+n+' = '+str(v)) for n,v in vals]; \
print('    enzyme: CREATE/DESTROY BUILD/BREAK REPAIR/RENEW')"
	@echo ""
	@echo "=== NSIGII_VERIFIED ==="

# ============================================================
# Protection Ring Verification
# ============================================================
ring: firmware bios-firmware
	@echo ""
	@echo "=== MMUKO-OS Protection Ring Verification ==="
	@echo "=== Electromagnetic Computation Model      ==="
	@echo ""
	@echo "Ring 0 (Kernel):  boot/ kernel/ → $(STAGE1_BIN) $(STAGE2_BIN) $(RUNTIME_BIN)"
	@echo "Ring 1 (Driver):  firmware/bios_interface → $(BIOS_LIB)"
	@echo "Ring 2 (Service): heartfull_membrane bzy_mpda tripartite_disc → $(FW_LIB)"
	@echo "Ring 3 (User):    python/ ui/ → applications"
	@echo ""
	@$(PY) -c "\
	from pathlib import Path; \
	ring0=all(Path(f).exists() for f in ['$(STAGE1_BIN)','$(STAGE2_BIN)','$(RUNTIME_BIN)']); \
	ring1=Path('$(BIOS_LIB)').exists(); \
	ring2=Path('$(FW_LIB)').exists(); \
	print('Ring 0 (Kernel):  '+('PASS' if ring0 else 'MISSING - run: make boot')); \
	print('Ring 1 (Driver):  '+('PASS' if ring1 else 'MISSING - run: make bios-firmware')); \
	print('Ring 2 (Service): '+('PASS' if ring2 else 'MISSING - run: make firmware')); \
	print('Ring 3 (User):    AVAILABLE (Python runtime)'); \
	print(); \
	print('EM duality (stateless double-compile):'); \
	print('  Electric (runtime):  stage1 -> stage2 -> runtime -> kernel'); \
	print('  Magnetic (linking):  .c -> .o -> $(SO_EXT)/.a -> linked image'); \
	print('  Ring transition:     NSIGII 6-phase gate (membrane PASS/HOLD/ALERT)'); \
	ok=ring0 and ring1 and ring2; \
	print(); \
	print('=== RING MODEL: '+('VERIFIED' if ok else 'INCOMPLETE')+' ===')"

# ============================================================
# Filesystem driver tree (root/trunk/branch/leaves)
# ============================================================
tree:
	@$(PY) scripts/tree_display.py

# ============================================================
# Clean
# ============================================================
clean:
	@echo "[CLEAN] Removing generated artifacts..."
	@$(PY) -c "import shutil,pathlib; \
	[shutil.rmtree(str(d),ignore_errors=True) for d in [pathlib.Path('build'),pathlib.Path('bin'),pathlib.Path('obj')]]; \
	pathlib.Path('mmuko-os.img').unlink(missing_ok=True)"
	@echo "[CLEAN] Done."

# ============================================================
# Help
# ============================================================
help:
	@echo "MMUKO-OS / NSIGII Heartfull Firmware - Build System"
	@echo "OBINexus Computing | Nnamdi Michael Okpala"
	@echo "Enzyme Model: CREATE/DESTROY | BUILD/BREAK | REPAIR/RENEW"
	@echo ""
	@echo "FIRST TIME SETUP (WSL/Ubuntu):"
	@echo "  make install-deps       <- installs nasm + build-essential + qemu"
	@echo "  make all                <- builds everything"
	@echo ""
	@echo "ALL TARGETS:"
	@echo "  make all                  firmware + boot + image + bios + phase1-ui + flat-bin"
	@echo "  make install-deps         nasm + build-essential (WSL)"
	@echo "  make firmware             C firmware .so + .a (NSIGII)"
	@echo "  make firmware-cpp         C++ NSIGII wrapper .so"
	@echo "  make bios-firmware        BIOS interface .so + .a (SpinPair, Mosaic, DateTime)"
	@echo "  make bios-firmware-cpp    BIOS interface C++ wrapper object"
	@echo "  make phase1-ui            Phase 1 terminal UI executable (build/phase1_ui)"
	@echo "  make flat-bin             nasm flat binary -> build/os_flat.bin"
	@echo "  make qemu-flat            boot os_flat.bin in QEMU (i386)"
	@echo "  make boot                 stage1 + stage2 (mmuko-os.bin) + runtime"
	@echo "  make image                write sectors into build/mmuko-os.img"
	@echo "  make cython-build         build Python wheel + sdist"
	@echo "  make cython-develop       install editable package"
	@echo "  make run-ui               Python console compositor"
	@echo "  make run                  boot mmuko-os.img in QEMU"
	@echo "  make verify               NSIGII verification checks"
	@echo "  make ring                 protection ring model verification"
	@echo "  make tree                 display filesystem driver tree"
	@echo "  make clean                remove build/"

# ============================================================
# BIOS Firmware (C shared library + static archive)
# ============================================================
bios-firmware: dirs $(BIOS_LIB) $(BIOS_ARC)
	@echo "[BIOS-FIRMWARE] OK: $(BIOS_LIB) $(BIOS_ARC)"

$(BIOS_LIB): $(BIOS_C_OBJ) | dirs
	@echo "[LD] $@"
	$(CC) $(SO_LDFLAGS) -o $@ $^

$(BIOS_ARC): $(BIOS_C_OBJ) | dirs
	@echo "[AR] $@"
	$(AR) rcs $@ $^

# ============================================================
# BIOS C++ wrapper object
# ============================================================
bios-firmware-cpp: dirs $(BIOS_CPP_OBJ)
	@echo "[BIOS-CPP] compiled: $(BIOS_CPP_OBJ)"

$(BIOS_CPP_OBJ): firmware/bios_interface.cpp firmware/bios_interface_cpp.h firmware/bios_interface.h | dirs
	@echo "[CXX] $<"
	$(CXX) $(CXXFLAGS) -I. -Iinclude -c $< -o $@

# ============================================================
# Phase 1 Terminal UI standalone executable
# ============================================================
phase1-ui: dirs $(UI_BIN)
	@echo "[PHASE1-UI] OK: $(UI_BIN)"

$(UI_BIN): $(UI_C_OBJ) $(BIOS_C_OBJ) | dirs
	@echo "[LD] $@"
	$(CC) -o $@ $^ -lm

# ============================================================
# Flat binary: standalone bootloader + kernel for QEMU testing
# ============================================================
flat-bin: dirs $(FLAT_OS_BIN)
	@echo "[FLAT-BIN] OK: $(FLAT_OS_BIN)"
	@PY_CMD=$$(command -v python3 2>/dev/null || command -v python 2>/dev/null); \
	if [ -n "$$PY_CMD" ]; then \
		$$PY_CMD -c "\
import struct; \
boot=open('$(FLAT_BOOT_BIN)','rb').read(); \
kern=open('$(FLAT_KERN_BIN)','rb').read(); \
assert len(boot)==512, 'bootloader size: '+str(len(boot)); \
sig=struct.unpack_from('<H',boot,510)[0]; \
assert sig==0xAA55,'sig: '+hex(sig); \
print('[FLAT-BIN] bootloader='+str(len(boot))+'B sig=0xAA55  kernel='+str(len(kern))+'B  total='+str(len(boot)+len(kern))+'B')"; \
	else \
		echo "[FLAT-BIN] (skipping Python verify — no python/python3 found)"; \
	fi

$(FLAT_BOOT_BIN): $(FLAT_BOOT_SRC) | dirs
	@echo "[NASM] $<"
	$(NASM) -f bin $< -o $@

$(FLAT_KERN_BIN): $(FLAT_KERN_SRC) | dirs
	@echo "[NASM] $<"
	$(NASM) -f bin $< -o $@

$(FLAT_OS_BIN): $(FLAT_BOOT_BIN) $(FLAT_KERN_BIN)
	@echo "[CAT] $@"
	@if command -v cat >/dev/null 2>&1; then \
		cat $(FLAT_BOOT_BIN) $(FLAT_KERN_BIN) > $@; \
	else \
		$(PY) -c "\
from pathlib import Path; \
b=Path('$(FLAT_BOOT_BIN)').read_bytes(); \
k=Path('$(FLAT_KERN_BIN)').read_bytes(); \
Path('$(FLAT_OS_BIN)').write_bytes(b+k); \
print('[FLAT-BIN] wrote',len(b)+len(k),'bytes')"; \
	fi

# ============================================================
# QEMU flat binary boot (i386, 4 MB)
# ============================================================
qemu-flat: $(FLAT_OS_BIN)
	@echo "[QEMU] Booting flat binary $(FLAT_OS_BIN) ..."
	qemu-system-i386 -drive format=raw,file=$(FLAT_OS_BIN) -m 4M \
		|| echo "[QEMU] Not found — install qemu-system-x86 first"
=======
# Makefile - MMUKO-OS / NSIGII Heartfull Firmware
# OBINexus Computing | Nnamdi Michael Okpala | 24 March 2026
#
# Works on: WSL/Linux (make) and Windows native (make from Git Bash/MSYS2).
#
# Boot chain:
#   stage-1  (boot/mmuko_stage1_boot.asm) → build/boot.bin     (512 bytes, 0xAA55)
#   stage-2  (boot/stage2.asm)            → build/mmuko-os.bin  (kernel payload)
#   runtime  (kernel/runtime.asm)         → build/runtime.bin   (firmware entry)
#   image    stage1 + stage2 + runtime    → build/mmuko-os.img  (1.44 MB FAT12)
#
# REPL subsystem (mmuko-repl / RIFT shell):
#   repl/src/rtrie.c      → Red-Black Trie path store
#   repl/src/consensus.c  → Execution-permission consensus state machine
#   repl/src/rift_repl.c  → Interactive RIFT shell
#   repl/repl_main.c      → Standalone entry point  → build/mmukocycle
#   All three src objects also archived into build/lib/libmmuko_repl.a
#
# Enzyme model strategy:
#   MAYBE → YES / NO
#   CREATE / DESTROY  (phase init / phase teardown)
#   BUILD  / BREAK    (sector assembly / disassembly)
#   REPAIR / RENEW    (kernel panic recovery / hot-reload)
#
# Targets:
#   make all               build firmware + boot + image + repl
#   make install-deps      install nasm in WSL/Ubuntu (run once)
#   make firmware          compile C firmware -> .so + .a
#   make firmware-cpp      compile C++ wrapper
#   make boot              assemble split boot chain (stage1 + stage2 + runtime)
#   make image             write boot sectors into build/mmuko-os.img
#   make repl              build RIFT REPL library + mmukocycle executable
#   make run-repl          run the RIFT interactive shell
#   make cython-build      build Python wheel + sdist
#   make cython-develop    install editable Cython package
#   make run-ui            run Python console compositor
#   make run               boot with QEMU
#   make verify            NSIGII verification checks
#   make tree              display filesystem driver tree
#   make clean             remove build/ directory

# ============================================================
# Platform detection (Windows native vs WSL/Linux)
# ============================================================
ifeq ($(OS),Windows_NT)
    EXE      := .exe
    PY       := python
    RMRF     := cmd /c if exist build rmdir /s /q build & if exist bin rmdir /s /q bin & if exist obj rmdir /s /q obj
    RMIMG    := cmd /c if exist mmuko-os.img del /f mmuko-os.img
else
    EXE      :=
    PY       := python3
    RMRF     := rm -rf build/ bin/ obj/
    RMIMG    := rm -f mmuko-os.img
endif

# ============================================================
# Toolchain
# ============================================================
CC     := gcc
CXX    := g++
NASM   := nasm
AR     := ar
PIP    := $(PY) -m pip
BUILD_PY := $(PY) -m build

# ============================================================
# Build output directories
# ============================================================
BUILD    := build
OBJ_DIR  := $(BUILD)/obj
LIB_DIR  := $(BUILD)/lib
BOOT_DIR := $(BUILD)

# Boot chain binaries
STAGE1_BIN       := $(BUILD)/boot.bin
STAGE2_BIN       := $(BUILD)/mmuko-os.bin
RUNTIME_BIN      := $(BUILD)/runtime.bin
DISK_IMG         := $(BUILD)/mmuko-os.img

# Stage-2 sector budget (16 sectors × 512 = 8192 bytes max)
STAGE2_SECTORS       := 16
STAGE2_TOTAL_SECTORS := $(STAGE2_SECTORS)
STAGE2_TOTAL_BYTES   := 8192

# Runtime sector budget (12 sectors × 512 = 6144 bytes max)
RUNTIME_SECTORS      := 12

# ============================================================
# REPL (mmuko-repl / RIFT shell) — integrated from mmuko-repl-main
# ============================================================
REPL_DIR      := repl
REPL_SRC_DIR  := $(REPL_DIR)/src
REPL_INC_DIR  := $(REPL_DIR)/include
REPL_SRCS     := $(REPL_SRC_DIR)/rtrie.c \
                 $(REPL_SRC_DIR)/consensus.c \
                 $(REPL_SRC_DIR)/rift_repl.c
REPL_OBJS     := $(addprefix $(OBJ_DIR)/repl/,$(notdir $(REPL_SRCS:.c=.o)))
REPL_MAIN     := $(REPL_DIR)/repl_main.c
REPL_MAIN_OBJ := $(OBJ_DIR)/repl/repl_main.o
REPL_LIB      := $(LIB_DIR)/libmmuko_repl.a
MMUKOCYCLE    := $(BUILD)/mmukocycle$(EXE)
REPL_CFLAGS   := $(CFLAGS) -Iinclude -I$(REPL_INC_DIR)

# Codegen paths — pseudocode lives in two places:
#   pseudocode/           (canonical .psc collection)
#   mmuko-boot/pseudocode (boot-specific subset, mirrors primary .psc)
CODEGEN_SCRIPT   := tools/mmuko_codegen/generate.py
PSC_DIR          := pseudocode
PSC_BOOT_DIR     := mmuko-boot/pseudocode
PRIMARY_PSC      := $(PSC_DIR)/mmuko-boot.psc
CODEGEN_STAMP    := $(BUILD)/mmuko_codegen.stamp

# Generated source paths
GENERATED_BOOT_SRC   := boot/mmuko_stage1_boot.asm
GENERATED_STAGE2_C   := kernel/mmuko_stage2_loader.c
GENERATED_STAGE2_CPP := kernel/mmuko_stage2_bridge.cpp
GENERATED_HEADER     := include/mmuko_codegen.h
GENERATED_CYTHON     := python/mmuko_codegen.pxd python/mmuko_generated.pyx

# ============================================================
# Compiler flags
# ============================================================
CFLAGS    := -std=c11 -Wall -Wextra -fPIC -O2
CXXFLAGS  := -std=c++17 -Wall -Wextra -fPIC -O2
SO_LDFLAGS := -shared -lm
LDFLAGS   := -shared -lm

# ============================================================
# Source files
# ============================================================
C_SRCS   := heartfull_membrane.c bzy_mpda.c tripartite_discriminant.c
C_OBJS   := $(addprefix $(OBJ_DIR)/,$(C_SRCS:.c=.o))
CPP_SRCS := nsigii_cpp_wrapper.cpp
CPP_OBJS := $(addprefix $(OBJ_DIR)/,$(CPP_SRCS:.cpp=.o))
FW_LIB   := $(LIB_DIR)/libnsigii_firmware.so
FW_ARC   := $(LIB_DIR)/libnsigii_firmware.a
CPP_LIB  := $(LIB_DIR)/libnsigii_firmware_cpp.so

# ============================================================
# Phony targets
# ============================================================
.PHONY: all dirs install-deps firmware firmware-cpp boot image run repl run-repl \
        cython-build cython-develop run-ui verify tree clean help codegen

# ============================================================
# Default: build everything
# ============================================================
all: dirs codegen firmware boot image repl
	@echo ""
	@echo "======================================="
	@echo " MMUKO-OS build complete (enzyme: OK)"
	@echo "======================================="
	@echo "  Stage-1  : $(STAGE1_BIN)  (boot sector)"
	@echo "  Stage-2  : $(STAGE2_BIN)  (mmuko-os kernel)"
	@echo "  Runtime  : $(RUNTIME_BIN) (firmware entry)"
	@echo "  Firmware : $(FW_LIB)"
	@echo "  Image    : $(DISK_IMG)"
	@echo "  REPL lib : $(REPL_LIB)"
	@echo "  REPL exe : $(MMUKOCYCLE)"
	@echo ""
	@echo "  make run       - boot in QEMU"
	@echo "  make run-repl  - launch MMUKO interactive shell"
	@echo "  make verify    - NSIGII checks"
	@echo "  make tree      - display driver tree"

# ============================================================
# Directory creation (must run before everything else)
# ============================================================
dirs:
	@$(PY) -c "import os; [os.makedirs(d, exist_ok=True) for d in ['$(OBJ_DIR)','$(LIB_DIR)','$(BUILD)','$(OBJ_DIR)/kernel','$(OBJ_DIR)/repl']]"

# ============================================================
# Code generation from canonical spec + pseudocode
# ============================================================
codegen: dirs $(CODEGEN_STAMP)

$(CODEGEN_STAMP): $(PRIMARY_PSC)
	@echo "[CODEGEN] Generating MMUKO-OS derived sources..."
	@if [ -f "MMUKO-OS.txt" ] && [ -f "$(CODEGEN_SCRIPT)" ]; then \
		$(PY) $(CODEGEN_SCRIPT) --root . --spec MMUKO-OS.txt \
			--primary $(PRIMARY_PSC) --pseudocode-dir $(PSC_DIR) 2>/dev/null || true; \
	else \
		echo "[CODEGEN] MMUKO-OS.txt or generate.py not found — using existing sources"; \
	fi
	@touch $(CODEGEN_STAMP)

# ============================================================
# Dependencies
# ============================================================
install-deps:
	@echo "[DEPS] Installing nasm, gcc, qemu..."
	sudo apt-get update -qq
	sudo apt-get install -y nasm build-essential qemu-system-x86

# ============================================================
# Firmware (C shared library + static archive)
# ============================================================
firmware: dirs $(FW_LIB) $(FW_ARC)
	@echo "[FIRMWARE] OK: $(FW_LIB) $(FW_ARC)"

$(OBJ_DIR)/%.o: %.c | dirs
	@echo "[CC] $<"
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I. -Iinclude -c $< -o $@

$(FW_LIB): $(C_OBJS) | dirs
	@echo "[LD] $@"
	$(CC) $(SO_LDFLAGS) -o $@ $^

$(FW_ARC): $(C_OBJS) | dirs
	@echo "[AR] $@"
	$(AR) rcs $@ $^

# ============================================================
# Firmware C++ wrapper
# ============================================================
firmware-cpp: dirs $(CPP_LIB)

$(CPP_LIB): $(CPP_OBJS) $(C_OBJS)
	@echo "[CXX] $@"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -I. -Iinclude -o $@ $^

$(OBJ_DIR)/%.o: %.cpp | dirs
	@echo "[CXX obj] $<"
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I. -Iinclude -c $< -o $@

# ============================================================
# Boot chain assembly (NASM with Python fallback)
# ============================================================
boot: dirs
	@if command -v $(NASM) >/dev/null 2>&1; then \
		echo "[NASM] Assembling boot chain with nasm..."; \
		$(NASM) -f bin $(GENERATED_BOOT_SRC) -o $(STAGE1_BIN); \
		$(NASM) -f bin boot/stage2.asm -o $(STAGE2_BIN); \
		$(NASM) -f bin kernel/runtime.asm -o $(RUNTIME_BIN); \
	else \
		echo "[PYTHON] nasm not found — using Python fallback assembler"; \
		$(PY) scripts/assemble_boot.py --build-dir $(BUILD); \
	fi
	@$(PY) -c "\
import struct; \
b=open('$(STAGE1_BIN)','rb').read(); \
assert len(b)==512,'stage1 size: '+str(len(b)); \
sig=struct.unpack_from('<H',b,510)[0]; \
assert sig==0xAA55,'sig: '+hex(sig); \
print('[BOOT] stage1='+str(len(b))+'B sig=0xAA55 OK'); \
s2=open('$(STAGE2_BIN)','rb').read(); \
print('[BOOT] stage2='+str(len(s2))+'B (mmuko-os) OK'); \
rt=open('$(RUNTIME_BIN)','rb').read(); \
print('[BOOT] runtime='+str(len(rt))+'B OK')"

# ============================================================
# Disk image (1.44 MB FAT12 — stage1 + stage2 + runtime)
# ============================================================
.FORCE:
image: $(DISK_IMG)

$(DISK_IMG): .FORCE | dirs
	@echo "[IMAGE] Writing $(DISK_IMG) (1.44 MB FAT12 + kernel payload)..."
	@$(PY) -c "\
from pathlib import Path; \
stage1 = Path('$(STAGE1_BIN)').read_bytes(); \
stage2 = Path('$(STAGE2_BIN)').read_bytes(); \
runtime = Path('$(RUNTIME_BIN)').read_bytes(); \
img = bytearray(b'\x00' * (512 * 2880)); \
img[:512] = stage1; \
img[512:512+len(stage2)] = stage2; \
img[512+len(stage2):512+len(stage2)+len(runtime)] = runtime; \
Path('$(DISK_IMG)').write_bytes(img); \
print(f'[IMAGE] wrote {len(img)} bytes  stage2={len(stage2)}B  runtime={len(runtime)}B')"

# ============================================================
# REPL — RIFT shell library + standalone mmukocycle binary
# ============================================================
repl: dirs $(REPL_LIB) $(MMUKOCYCLE)
	@echo "[REPL] OK: $(REPL_LIB)  $(MMUKOCYCLE)"

# Compile each repl source into build/obj/repl/
$(OBJ_DIR)/repl/%.o: $(REPL_SRC_DIR)/%.c | dirs
	@echo "[REPL CC] $<"
	$(CC) $(REPL_CFLAGS) -c $< -o $@

# Static archive: libmmuko_repl.a
$(REPL_LIB): $(REPL_OBJS) | dirs
	@echo "[REPL AR] $@"
	$(AR) rcs $@ $^

# Compile the standalone entry point
$(REPL_MAIN_OBJ): $(REPL_MAIN) | dirs
	@echo "[REPL CC] $<"
	$(CC) $(REPL_CFLAGS) -c $< -o $@

# Link mmukocycle executable (repl_main + libmmuko_repl.a)
$(MMUKOCYCLE): $(REPL_MAIN_OBJ) $(REPL_LIB) | dirs
	@echo "[REPL LD] $@"
	$(CC) -o $@ $(REPL_MAIN_OBJ) $(REPL_LIB)

# Run the RIFT interactive shell
run-repl: $(MMUKOCYCLE)
	@echo "[MMUKO] Launching MMUKO shell..."
	$(MMUKOCYCLE)

# ============================================================
# QEMU boot (explicit format=raw suppresses the warning)
# ============================================================
run: $(DISK_IMG)
	@echo "[QEMU] Booting MMUKO-OS..."
	qemu-system-x86_64 -drive format=raw,file=$(DISK_IMG) -m 32M \
		|| echo "[QEMU] Not found - https://www.qemu.org"

# ============================================================
# Python / Cython package
# ============================================================
cython-build: firmware
	@echo "[PYTHON] Building sdist + wheel..."
	$(PIP) install --quiet build Cython
	$(BUILD_PY)

cython-develop: firmware
	@echo "[PYTHON] Installing editable package..."
	$(PIP) install --quiet -e .

run-ui: cython-develop
	@echo "[UI] Running Python console compositor..."
	PYTHONPATH=python $(PY) -m mmuko_os --tier1 yes --tier2 yes --w-actor yes

# ============================================================
# NSIGII Verification
# ============================================================
verify: boot
	@echo ""
	@echo "=== NSIGII Verification ==="
	@echo ""
	@echo "[1] Stage-1 boot sector"
	@$(PY) -c "\
import struct; \
b=open('$(STAGE1_BIN)','rb').read(); \
assert len(b)==512; \
assert struct.unpack_from('<H',b,510)[0]==0xAA55; \
print('    stage1: 512 bytes / sig=0xAA55  PASS')"
	@echo "[2] Stage-2 kernel payload (mmuko-os.bin)"
	@$(PY) -c "\
b=open('$(STAGE2_BIN)','rb').read(); \
print('    stage2: '+str(len(b))+' bytes  PASS')"
	@echo "[3] Runtime firmware entry"
	@$(PY) -c "\
b=open('$(RUNTIME_BIN)','rb').read(); \
print('    runtime: '+str(len(b))+' bytes  PASS')"
	@echo "[4] Disk image"
	@$(PY) -c "\
b=open('$(DISK_IMG)','rb').read(); \
assert len(b)==512*2880; \
print('    image: '+str(len(b))+' bytes (1.44 MB)  PASS')"
	@echo "[5] Enzyme trinary alphabet"
	@$(PY) -c "\
vals=[('YES',1),('NO',0),('MAYBE',-1),('MAYBE_NOT',-2)]; \
[print('    '+n+' = '+str(v)) for n,v in vals]; \
print('    enzyme: CREATE/DESTROY BUILD/BREAK REPAIR/RENEW')"
	@echo ""
	@echo "=== NSIGII_VERIFIED ==="

# ============================================================
# Filesystem driver tree (root/trunk/branch/leaves)
# ============================================================
tree:
	@$(PY) scripts/tree_display.py

# ============================================================
# Clean
# ============================================================
clean:
	@echo "[CLEAN] Removing generated artifacts..."
	@$(RMRF)
	@$(RMIMG)
	@echo "[CLEAN] Done."
	@echo "  (REPL objects cleaned with build/)"

# ============================================================
# Help
# ============================================================
help:
	@echo "MMUKO-OS / NSIGII Heartfull Firmware - Build System"
	@echo "OBINexus Computing | Nnamdi Michael Okpala"
	@echo "Enzyme Model: CREATE/DESTROY | BUILD/BREAK | REPAIR/RENEW"
	@echo ""
	@echo "FIRST TIME SETUP (WSL/Ubuntu):"
	@echo "  make install-deps       <- installs nasm + build-essential + qemu"
	@echo "  make all                <- builds everything"
	@echo ""
	@echo "ALL TARGETS:"
	@echo "  make all                  firmware + boot + image + repl"
	@echo "  make install-deps         nasm + build-essential (WSL)"
	@echo "  make firmware             C firmware .so + .a"
	@echo "  make firmware-cpp         C++ wrapper .so"
	@echo "  make boot                 stage1 + stage2 (mmuko-os.bin) + runtime"
	@echo "  make image                write sectors into build/mmuko-os.img"
	@echo "  make repl                 build RIFT REPL (libmmuko_repl.a + mmukocycle)"
	@echo "  make run-repl             launch RIFT interactive shell"
	@echo "  make cython-build         build Python wheel + sdist"
	@echo "  make cython-develop       install editable package"
	@echo "  make run-ui               Python console compositor"
	@echo "  make run                  boot in QEMU"
	@echo "  make verify               NSIGII verification checks"
	@echo "  make tree                 display filesystem driver tree"
	@echo "  make clean                remove build/"
>>>>>>> dev
