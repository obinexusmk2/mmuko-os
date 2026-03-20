# ============================================================================
# NSIGII Trident Command & Control - Master Makefile
# Human Rights Verification System
# ============================================================================

# Compiler settings
CC = gcc

# Platform detection
ifeq ($(OS),Windows_NT)
    # MSYS2/MinGW
    CFLAGS = -Wall -Wextra -O2 -fPIC -pthread -I/c/msys64/mingw64/include
    LDFLAGS = -L/c/msys64/mingw64/lib -lpthread -lssl -lcrypto -lm -lws2_32
else
    # Linux / WSL
    CFLAGS = -Wall -Wextra -O2 -fPIC -pthread
    LDFLAGS = -lpthread -lssl -lcrypto -lm
endif

# Directories
CH0_DIR = ch0
CH1_DIR = ch1
PAGES_DIR = pages
DOCS_DIR = docs

# Build directories
BUILD_DIR = build
BIN_DIR = bin

# Targets
.PHONY: all clean ch0 ch1 websocket docs install run-web

# Default target
all: directories ch0 ch1 websocket

# Create build directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# Build Channel 0 (Transmitter)
ch0: directories
	@echo "Building Channel 0 (Transmitter)..."
	$(CC) $(CFLAGS) -I$(CH0_DIR)/include \
		$(CH0_DIR)/src/nsigii_utils.c \
		$(CH0_DIR)/src/serialization.c \
		$(CH0_DIR)/src/transmitter/transmitter.c \
		$(CH0_DIR)/src/main.c \
		-o $(BIN_DIR)/nsigii_ch0 \
		$(LDFLAGS)
	@echo "Channel 0 built: $(BIN_DIR)/nsigii_ch0"

# Build Channel 1 (Receiver + Verifier)
ch1: directories
	@echo "Building Channel 1 (Receiver + Verifier)..."
	$(CC) $(CFLAGS) -I$(CH0_DIR)/include -I$(CH1_DIR)/include \
		$(CH0_DIR)/src/nsigii_utils.c \
		$(CH0_DIR)/src/serialization.c \
		$(CH0_DIR)/src/receiver/receiver.c \
		$(CH0_DIR)/src/verifier/verifier.c \
		$(CH1_DIR)/src/main.c \
		-o $(BIN_DIR)/nsigii_ch1 \
		$(LDFLAGS)
	@echo "Channel 1 built: $(BIN_DIR)/nsigii_ch1"

# Build WebSocket Server
websocket: directories
	@echo "Building WebSocket Server..."
	$(CC) $(CFLAGS) -I$(CH0_DIR)/include \
		$(CH0_DIR)/src/nsigii_utils.c \
		$(CH0_DIR)/src/serialization.c \
		$(CH0_DIR)/src/websocket_server.c \
		-o $(BIN_DIR)/nsigii_websocket \
		$(LDFLAGS) -DWS_STANDALONE
	@echo "WebSocket Server built: $(BIN_DIR)/nsigii_websocket"

# Build shared library
lib: directories
	@echo "Building NSIGII shared library..."
	$(CC) $(CFLAGS) -shared \
		-I$(CH0_DIR)/include \
		$(CH0_DIR)/src/nsigii_utils.c \
		$(CH0_DIR)/src/serialization.c \
		$(CH0_DIR)/src/transmitter/transmitter.c \
		$(CH0_DIR)/src/receiver/receiver.c \
		$(CH0_DIR)/src/verifier/verifier.c \
		-o $(BUILD_DIR)/libnsigii.so \
		$(LDFLAGS)
	@echo "Shared library built: $(BUILD_DIR)/libnsigii.so"

# Build all components
build-all: all lib

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
	@echo "Clean complete"

# Install binaries
install: all
	@echo "Installing NSIGII binaries..."
	@mkdir -p /usr/local/bin
	@cp $(BIN_DIR)/* /usr/local/bin/
	@echo "Installed to /usr/local/bin/"

# Uninstall
uninstall:
	@echo "Uninstalling NSIGII binaries..."
	@rm -f /usr/local/bin/nsigii_ch0
	@rm -f /usr/local/bin/nsigii_ch1
	@rm -f /usr/local/bin/nsigii_websocket
	@echo "Uninstall complete"

# Run Channel 0
run-ch0: ch0
	./$(BIN_DIR)/nsigii_ch0

# Run Channel 1
run-ch1: ch1
	./$(BIN_DIR)/nsigii_ch1

# Run WebSocket Server
run-websocket: websocket
	./$(BIN_DIR)/nsigii_websocket

# Run all (in separate terminals)
run-all: all
	@echo "Starting NSIGII Trident System..."
	@echo "Start WebSocket: make run-websocket"
	@echo "Start Channel 0: make run-ch0"
	@echo "Start Channel 1: make run-ch1"

# Run Python HTTP server for web interface (fallback)
run-web:
	@echo "Starting Python HTTP server for NSIGII web interface..."
	@echo "Open http://localhost:8000/html/index.html in your browser"
	cd $(PAGES_DIR)/mac2loopback && python3 -m http.server 8000 || python -m http.server 8000

# Debug build
debug: CFLAGS = -Wall -Wextra -g -O0 -fPIC -pthread -DDEBUG
debug: all

# Test build
test: CFLAGS = -Wall -Wextra -O2 -fPIC -pthread -DTEST
test: all
	@echo "Running tests..."
	./$(BIN_DIR)/nsigii_ch0 --test || true
	./$(BIN_DIR)/nsigii_ch1 --test || true

# Documentation
docs:
	@echo "Generating documentation..."
	@mkdir -p $(DOCS_DIR)/html
	@echo "Documentation placeholder"

# Help
help:
	@echo "NSIGII Trident Command & Control - Makefile Targets"
	@echo "=================================================="
	@echo ""
	@echo "Build Targets:"
	@echo "  make all          - Build all components"
	@echo "  make ch0          - Build Channel 0 (Transmitter)"
	@echo "  make ch1          - Build Channel 1 (Receiver + Verifier)"
	@echo "  make websocket    - Build WebSocket Server"
	@echo "  make lib          - Build shared library"
	@echo "  make build-all    - Build all including library"
	@echo ""
	@echo "Run Targets:"
	@echo "  make run-ch0      - Run Channel 0"
	@echo "  make run-ch1      - Run Channel 1"
	@echo "  make run-websocket- Run WebSocket Server"
	@echo "  make run-all      - Show run instructions"
	@echo "  make run-web      - Serve web UI via Python (fallback)"
	@echo ""
	@echo "Other Targets:"
	@echo "  make clean        - Clean build artifacts"
	@echo "  make install      - Install binaries"
	@echo "  make uninstall    - Uninstall binaries"
	@echo "  make debug        - Build with debug symbols"
	@echo "  make test         - Build and run tests"
	@echo "  make docs         - Generate documentation"
	@echo "  make help         - Show this help"
