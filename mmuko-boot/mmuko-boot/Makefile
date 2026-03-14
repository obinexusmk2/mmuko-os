# MMUKO OS Boot Makefile
# OBINexus R&D

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -lm
TARGET = mmuko-boot
SRC = mmuko-boot.c

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS = -Wall -Wextra -std=c11 -g -O0 -lm -DDEBUG
debug: $(TARGET)

# Static analysis
analyze:
	cppcheck --enable=all --inconclusive $(SRC)

# Format code
format:
	clang-format -i $(SRC)
