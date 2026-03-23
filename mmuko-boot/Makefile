# MMUKO OS Boot Makefile
# OBINexus R&D

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lm
TARGET = mmuko-boot
SRC = mmuko-boot.c
HDR = mmuko_boot_contract.h

.PHONY: all clean run debug analyze format

all: $(TARGET)

$(TARGET): $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS = -Wall -Wextra -std=c11 -g -O0 -DDEBUG
debug: $(TARGET)

analyze:
	cppcheck --enable=all --inconclusive $(SRC)

format:
	clang-format -i $(SRC) $(HDR)
