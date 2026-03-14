# ============================================================
# MMUKO OS Boot Makefile (CAB-Integrated)
# OBINexus R&D
# ============================================================

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -lm
TARGET = mmuko-boot
SRC = mmuko-boot.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean run run-default run-research debug analyze format help memcheck

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	@echo "[BUILD] Complete: $(TARGET)"

# Object file (if needed for separate compilation)
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c -o $(OBJ) $(SRC)

# Clean build artifacts
clean:
	rm -f $(TARGET) *.o *.a
	@echo "[CLEAN] Build artifacts removed"

# Run with default profile.cab
run: $(TARGET)
	./$(TARGET)

# Run with specific CAB file
run-default: $(TARGET)
	./$(TARGET) profile.cab

run-research: $(TARGET)
	./$(TARGET) research.cab

run-secure: $(TARGET)
	./$(TARGET) secure.cab

# Debug build with symbols
debug: CFLAGS = -Wall -Wextra -std=c11 -g -O0 -lm -DDEBUG
debug: clean $(TARGET)
	@echo "[DEBUG] Build complete with symbols"

# Static analysis with cppcheck
analyze:
	@echo "[ANALYZE] Running cppcheck..."
	cppcheck --enable=all --inconclusive $(SRC)

# Format code with clang-format
format:
	@echo "[FORMAT] Running clang-format..."
	clang-format -i $(SRC)

# Memory check with valgrind
memcheck: debug
	@echo "[MEMCHECK] Running valgrind..."
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

# Help
help:
	@echo ""
	@echo "MMUKO OS Boot Makefile"
	@echo "======================="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build mmuko-boot executable (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  run          - Run with default profile.cab"
	@echo "  run-default  - Run with profile.cab"
	@echo "  run-research - Run with research.cab"
	@echo "  run-secure   - Run with secure.cab"
	@echo "  debug        - Build with debug symbols (-g -O0)"
	@echo "  analyze      - Run static analysis (cppcheck)"
	@echo "  format       - Format source code (clang-format)"
	@echo "  memcheck     - Run with valgrind memory checker"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Profile CAB Files:"
	@echo "  profile.cab  - Default profile"
	@echo "  research.cab - Research configuration"
	@echo "  secure.cab   - Secure configuration"
	@echo ""
	@echo "Example usage:"
	@echo "  make                   # Build mmuko-boot"
	@echo "  make run               # Build and run with default CAB"
	@echo "  ./mmuko-boot           # Run directly with profile.cab"
	@echo "  ./mmuko-boot other.cab # Run with custom CAB"
	@echo ""

.PHONY: all clean run run-default run-research run-secure debug analyze format memcheck help
