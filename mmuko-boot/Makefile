# MMUKO stage-2 build helpers

CC = gcc
CPPFLAGS = -I../include -DMMUKO_BOOT_DESKTOP_SIM
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = mmuko-boot
SRC = mmuko-boot.c
HDR = mmuko_boot_contract.h

.PHONY: all clean run debug generate-spec

.PHONY: all clean

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS = -Wall -Wextra -std=c11 -g -O0 -DDEBUG
debug: $(TARGET)

analyze:
	cppcheck --enable=all --inconclusive $(SRC)

format:
	clang-format -i $(SRC)

generate-spec:
	python3 generate_from_spec.py
