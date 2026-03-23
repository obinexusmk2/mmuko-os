# Makefile - MMUKO-OS / NSIGII Heartfull Firmware
# OBINexus Computing | Nnamdi Michael Okpala | 20 March 2026

CC     := gcc
CXX    := g++
NASM   := nasm
DOTNET := dotnet
PY     := python3

BUILD                := build
OBJ_DIR              := $(BUILD)/obj
LIB_DIR              := $(BUILD)/lib
BOOT_DIR             := $(BUILD)/boot
STAGE1_SRC           := boot/stage1.asm
STAGE2_LOADER_SRC    := boot/stage2.asm
RUNTIME_SRC          := kernel/runtime.asm
STAGE1_BIN           := $(BOOT_DIR)/stage1.bin
STAGE2_LOADER_BIN    := $(BOOT_DIR)/stage2-loader.bin
RUNTIME_BIN          := $(BOOT_DIR)/runtime.bin
STAGE2_BIN           := $(BOOT_DIR)/stage2.bin
DISK_IMG             := $(BUILD)/mmuko-os.img
COMPOSITOR_PROJECT   := legacy/csharp-compositor/mmuko-compositor.csproj
STAGE2_TOTAL_SECTORS := 8
STAGE2_TOTAL_BYTES   := $(shell expr $(STAGE2_TOTAL_SECTORS) \* 512)

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
	@echo "  Stage-1  : $(STAGE1_BIN)"
	@echo "  Stage-2  : $(STAGE2_BIN)"
	@echo "  Image    : $(DISK_IMG)"

.PHONY: dirs
dirs:
	@$(PY) -c "import os; [os.makedirs(d, exist_ok=True) for d in ['$(OBJ_DIR)', '$(LIB_DIR)', '$(BOOT_DIR)']]"

.PHONY: install-deps
install-deps:
	@echo "[DEPS] Installing nasm and build-essential..."
	sudo apt-get update -qq
	sudo apt-get install -y nasm build-essential

.PHONY: firmware
firmware: dirs $(FW_LIB) $(FW_ARC)
	@echo "[FIRMWARE] OK: $(FW_LIB)"

$(OBJ_DIR)/%.o: %.c | dirs
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -I. -c $< -o $@

$(FW_LIB): $(C_OBJS) | dirs
	@echo "[LD] $@"
	$(CC) $(LDFLAGS) -o $@ $^

$(FW_ARC): $(C_OBJS) | dirs
	@echo "[AR] $@"
	ar rcs $@ $^

.PHONY: firmware-cpp
firmware-cpp: dirs $(CPP_LIB)

$(CPP_LIB): nsigii_cpp_wrapper.cpp $(C_OBJS) | dirs
	@echo "[CXX] $@"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -I. -o $@ $^

.PHONY: boot
boot: dirs $(STAGE1_BIN) $(STAGE2_BIN)
	@echo "[BOOT] Stage-1 + Stage-2 ready"

$(STAGE1_BIN): $(STAGE1_SRC) boot/contract.inc | dirs
	@echo "[NASM] $(STAGE1_SRC) -> $(STAGE1_BIN)"
	@$(PY) -c "import shutil,sys; sys.exit(0) if shutil.which('nasm') else (print('[ERROR] nasm not found.'), sys.exit(1))"
	$(NASM) -f bin $(STAGE1_SRC) -o $(STAGE1_BIN)
	@$(PY) -c "b=open('$(STAGE1_BIN)','rb').read(); assert len(b)==512, len(b); assert b[510:512]==b'\x55\xAA', b[510:512]; print('[BOOT] stage1 size=512 sig=0xAA55 OK')"

$(STAGE2_LOADER_BIN): $(STAGE2_LOADER_SRC) boot/contract.inc | dirs
	@echo "[NASM] $(STAGE2_LOADER_SRC) -> $(STAGE2_LOADER_BIN)"
	$(NASM) -f bin $(STAGE2_LOADER_SRC) -o $(STAGE2_LOADER_BIN)
	@$(PY) -c "b=open('$(STAGE2_LOADER_BIN)','rb').read(); assert len(b)==512, len(b); print('[BOOT] stage2 loader size=512 OK')"

$(RUNTIME_BIN): $(RUNTIME_SRC) boot/contract.inc | dirs
	@echo "[NASM] $(RUNTIME_SRC) -> $(RUNTIME_BIN)"
	$(NASM) -f bin $(RUNTIME_SRC) -o $(RUNTIME_BIN)
	@$(PY) -c "b=open('$(RUNTIME_BIN)','rb').read(); print('[BOOT] runtime size='+str(len(b))+' bytes')"

$(STAGE2_BIN): $(STAGE2_LOADER_BIN) $(RUNTIME_BIN) | dirs
	@echo "[BOOT] combining stage2 loader + runtime -> $(STAGE2_BIN)"
	@$(PY) -c "from pathlib import Path; limit=$(STAGE2_TOTAL_BYTES); loader=Path('$(STAGE2_LOADER_BIN)').read_bytes(); runtime=Path('$(RUNTIME_BIN)').read_bytes(); data=loader+runtime; assert len(data)<=limit, f'stage2 payload too large: {len(data)} > {limit}'; data=data.ljust(limit,b'\0'); Path('$(STAGE2_BIN)').write_bytes(data); print(f'[BOOT] stage2 payload {len(loader)+len(runtime)} bytes padded to {limit}')"

.PHONY: image
image: $(DISK_IMG)

$(DISK_IMG): $(STAGE1_BIN) $(STAGE2_BIN) | dirs
	@echo "[IMAGE] Writing $(DISK_IMG) (1.44 MB FAT12 + raw stage2 payload)..."
	@$(PY) -c "from pathlib import Path; stage1=Path('$(STAGE1_BIN)').read_bytes(); stage2=Path('$(STAGE2_BIN)').read_bytes(); img=bytearray(b'\0'*(512*2880)); img[:512]=stage1; img[512:512+len(stage2)]=stage2; Path('$(DISK_IMG)').write_bytes(img); print(f'[IMAGE] wrote {len(img)} bytes with stage2 payload {len(stage2)} bytes')"

.PHONY: run
run: $(DISK_IMG)
	@echo "[QEMU] Booting MMUKO-OS..."
	qemu-system-x86_64 -drive format=raw,file=$(DISK_IMG) -m 32M || echo "[QEMU] Not found - https://www.qemu.org"

.PHONY: compositor
compositor: firmware
	@echo "[DOTNET] Building legacy C# compositor..."
	@if command -v $(DOTNET) >/dev/null 2>&1; then \
		$(DOTNET) build $(COMPOSITOR_PROJECT) -c Release --nologo -v quiet && echo "[DOTNET] Build complete."; \
	else \
		echo "[DOTNET] Not found - https://dot.net"; \
	fi

.PHONY: run-compositor
run-compositor:
	@echo "[COMPOSITOR] dev boot-gate bypass with explicit PASS inputs..."
	$(DOTNET) run --project $(COMPOSITOR_PROJECT) -- --simulate-pass --tier1 yes --tier2 yes --w-actor yes

.PHONY: run-compositor-pass
run-compositor-pass:
	@echo "[COMPOSITOR] explicit PASS path (boot-passed + T1/T2/W = yes)..."
	$(DOTNET) run --project $(COMPOSITOR_PROJECT) -- --boot-passed true --tier1 yes --tier2 yes --w-actor yes

.PHONY: run-compositor-maybe
run-compositor-maybe:
	@echo "[COMPOSITOR] diagnostic HOLD path (boot-passed + unresolved inputs)..."
	$(DOTNET) run --project $(COMPOSITOR_PROJECT) -- --boot-passed true --tier1 maybe --tier2 maybe --w-actor maybe

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
	@echo ""
	@echo "MMUKO-OS / NSIGII Heartfull Firmware - Build System"
	@echo ""
	@echo "  make all            - build firmware, stage1, stage2, image"
	@echo "  make boot           - assemble split boot chain"
	@echo "  make image          - write stage1 + raw stage2 payload into build/mmuko-os.img"
	@echo "  make run            - boot build/mmuko-os.img with QEMU"
	@echo "  make compositor     - build the legacy C# compositor"
	@echo "  make verify         - verify stage sizes and signatures"
	@echo "  make clean          - remove generated artifacts and legacy .NET outputs"
