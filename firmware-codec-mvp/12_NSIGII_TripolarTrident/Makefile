# ─────────────────────────────────────────────────────────────────────────────
# Makefile — OBINexus NSIGII Tripolar Algebra Core
# obinexusmk2/nsigii_ltcodec_showcase/core
#
# Cross-platform: Linux / macOS / Windows (MinGW/MSYS2)
# On Windows run: make (inside MinGW or MSYS2 terminal)
# ─────────────────────────────────────────────────────────────────────────────

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic -g -I.
LDFLAGS = -lm

# Windows (MinGW) needs no extra libs for QPC timestamp.
# POSIX systems need -lrt on older Linux (not needed on modern glibc).
UNAME := $(shell uname -s 2>/dev/null || echo Windows)
ifneq ($(filter Linux%,$(UNAME)),)
    LDFLAGS += -lrt
endif

SRC      = tripolar_engine.c tripolar_lexer.c
OBJ      = $(SRC:.c=.o)
TEST_SRC = tests/tripolar_test.c
TEST_BIN = tests/tripolar_test

# Windows binary extension
ifeq ($(OS),Windows_NT)
    TEST_BIN = tests/tripolar_test.exe
endif

.PHONY: all clean test

all: $(OBJ)

%.o: %.c tripolar_algebra.h
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_BIN)
	$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(SRC) tripolar_algebra.h
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(SRC) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(TEST_BIN)
