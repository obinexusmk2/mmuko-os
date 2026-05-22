# MMUKO-BOOT Makefile — phase-separated build
# Supports: Linux, macOS, WSL, Windows (via Python mkdir)

CC      = gcc
CPPFLAGS = -I../include -DMMUKO_BOOT_DESKTOP_SIM
CFLAGS  = -Wall -Wextra -std=c11 -O2

TARGET  = mmuko-boot
SRC     = mmuko-boot.c

# Shared boot helpers
HELPERS = phases/boot_helpers.c

# Phase files — one per boot phase
PHASES  = \
    phases/phase1_cubit_init.c \
    phases/phase2_compass_alignment.c \
    phases/phase3_superposition_entanglement.c \
    phases/phase4_frame_centering.c \
    phases/phase5_nonlinear_resolution.c \
    phases/phase6_rotation_verification.c

ALL_SRCS = $(SRC) $(HELPERS) $(PHASES)
ALL_OBJS = $(ALL_SRCS:.c=.o)

.PHONY: all clean run debug analyze format generate-spec

all: $(TARGET)

$(TARGET): $(ALL_OBJS)
	$(CC) $(CFLAGS) -o $@ $(ALL_OBJS)

# Pattern rule for all C files in this directory and phases/
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	python3 -c "import glob,os; [os.remove(f) for f in glob.glob('*.o') + glob.glob('phases/*.o') if os.path.exists(f)]"
	python3 -c "import os; os.path.exists('$(TARGET)') and os.remove('$(TARGET)')"

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS = -Wall -Wextra -std=c11 -g -O0 -DDEBUG
debug: $(TARGET)

analyze:
	cppcheck --enable=all --inconclusive $(ALL_SRCS)

format:
	clang-format -i $(ALL_SRCS) phases/*.h

generate-spec:
	python3 generate_from_spec.py
