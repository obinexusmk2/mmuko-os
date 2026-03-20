# ─────────────────────────────────────────────────────────────────────────────
# Makefile — OBINexus NSIGII Tripolar Algebra Core
# obinexusmk2/nsigii_ltcodec_showcase/core
# ─────────────────────────────────────────────────────────────────────────────

CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pedantic -g -I. -D_POSIX_C_SOURCE=200809L
LDFLAGS = -lm

SRC     = tripolar_engine.c tripolar_lexer.c
OBJ     = $(SRC:.c=.o)
TEST_SRC = tests/tripolar_test.c
TEST_BIN = tests/tripolar_test

.PHONY: all clean test

all: $(OBJ)

%.o: %.c tripolar_algebra.h
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(SRC) tripolar_algebra.h
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(SRC) $(LDFLAGS)

clean:
	rm -f $(OBJ) $(TEST_BIN)
