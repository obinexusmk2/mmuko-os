# MMUKO stage-2 build helpers

CC ?= gcc
NASM ?= nasm
CFLAGS ?= -std=c11 -Wall -Wextra -O2

BUILD ?= ../build
STAGE2_BIN ?= $(BUILD)/stage2.bin
RUNTIME_BIN ?= $(BUILD)/mmuko-runtime.bin
IMAGE_TOOL ?= $(BUILD)/mkmmukoimg

.PHONY: all clean

all: $(STAGE2_BIN) $(RUNTIME_BIN) $(IMAGE_TOOL)

$(STAGE2_BIN): stage2.asm | $(BUILD)
	$(NASM) -f bin $< -o $@

$(RUNTIME_BIN): runtime.asm | $(BUILD)
	$(NASM) -f bin $< -o $@

$(IMAGE_TOOL): mkimage.c image_layout.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ mkimage.c

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -f $(STAGE2_BIN) $(RUNTIME_BIN) $(IMAGE_TOOL)
