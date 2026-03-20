# NSIGII Trident Command & Control

## Human Rights Verification System

**Version:** 7.0.0
**Framework:** Rectorial Reasoning Rational Wheel
**Topology:** Transmitter | Receiver | Verifier (Bipartite Order & Chaos)

---

## Overview

NSIGII (Network Signal Integrity & Governance Interoperability Interface) is a trident-based command and control system designed for human rights verification. It implements a three-channel topology with tomographic pair resolving and real-time consensus verification through a Rational Wheel mechanism.

### Core Philosophy: "All Bindings Are Drivers"

- **SQUARE (Perfect Binding):** Symmetric bidirectional FFI
- **RECTANGLE (Driver):** Asymmetric request/response interface

### Trident Topology

```
+--------------+      +--------------+      +--------------+
| Transmitter  | -->  |   Receiver   | -->  |   Verifier   |
|   CH0 (1/3)  |      |   CH1 (2/3)  |      |   CH2 (3/3)  |
|    ORDER     |      |    CHAOS     |      |   VERIFIED   |
+--------------+      +--------------+      +--------------+
   127.0.0.1            127.0.0.2             127.0.0.3
      :8001                :8002                 :8003
                                                   |
                                              Consensus
                                              Broadcast
                                                   |
                           WebSocket Server <------+
                           (port 8080)
                           HTTP + WS data
```

---

## Architecture

### Channel Summary

| Channel | Role | Codec Ratio | Loopback | Port | RWX | Wheel Position |
|---------|------|-------------|----------|------|-----|----------------|
| CH0 | Transmitter | 1/3 | 127.0.0.1 | 8001 | WRITE (0x02) | 0 deg |
| CH1 | Receiver | 2/3 | 127.0.0.2 | 8002 | READ (0x04) | 120 deg |
| CH2 | Verifier | 3/3 | 127.0.0.3 | 8003 | EXECUTE (0x01) | 240 deg |

### RWX Verification Chain

The system enforces a strict permission chain: **WRITE -> READ -> EXECUTE**

1. **Transmitter (CH0)** encodes content using silicon-to-suffering encoding and sets WRITE permission
2. **Receiver (CH1)** decodes, verifies hash integrity, sets READ permission, updates bipolar state
3. **Verifier (CH2)** validates RWX chain, computes bipartite consensus, checks human rights tags, verifies wheel position, grants EXECUTE permission, and broadcasts consensus

### Bipartite Consensus

Consensus requires a **2/3 majority threshold (0.67)**. The verifier computes an order-weight ratio from packet content bits, applies a rational wheel correction using `sin(wheel_position)`, and checks against the threshold.

Formula:
```
consensus = |order_weight / total_bits + sin(wheel_position * PI / 180)| / 2
```

### Tomographic Pair Resolving

Packet pairs are validated by computing correlation between paired packets. Valid correlation range: **0.5 to 1.0**.

### Directory Structure

```
nsigii/
  ch0/
    include/
      nsigii.h                    # Core types and constants
      serialization.h             # Serialization API
      transmitter/transmitter.h   # CH0 Transmitter API
      receiver/receiver.h         # CH1 Receiver API
      verifier/verifier.h         # CH2 Verifier API
    src/
      main.c                      # CH0 entry point
      nsigii_utils.c              # Shared utilities (SHA-256, encoding)
      serialization.c             # Binary/JSON/IR serialization
      transmitter/transmitter.c   # Transmitter implementation
      receiver/receiver.c         # Receiver implementation
      verifier/verifier.c         # Verifier implementation
      websocket_server.c          # WebSocket + HTTP static file server
  ch1/
    src/
      main.c                      # CH1 entry point (Receiver + Verifier)
  pages/
    mac2loopback/                  # Web interface
      html/index.html
      js/main.mjs
      js/websocket.mjs
      js/dop-adapter.mjs
      js/xhr.mjs
      styles/nsigii.css
  Makefile                         # Build system with platform detection
  README.md                       # This file
```

---

## Prerequisites

### MSYS2/MinGW (Windows - Primary Build Environment)

```bash
# Install MSYS2 from https://www.msys2.org/
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl mingw-w64-x86_64-make
```

### Linux / WSL

```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

### Required Libraries

- **OpenSSL** (libssl, libcrypto) - SHA-256 hashing and HMAC-SHA256 consensus signatures
- **pthreads** - Multi-threaded worker processes for each channel
- **Winsock2** (Windows only) - Socket communication via ws2_32

---

## Building

### Build All Components

```bash
make all
```

### Build Individual Components

```bash
make ch0        # Build Channel 0 (Transmitter)
make ch1        # Build Channel 1 (Receiver + Verifier)
make websocket  # Build WebSocket Server
make lib        # Build shared library (libnsigii.so)
make build-all  # Build everything including shared library
```

### Debug Build

```bash
make debug      # Build with -g -O0 debug symbols
```

### Build Targets Summary

| Target | Description |
|--------|-------------|
| `make all` | Build ch0, ch1, websocket |
| `make ch0` | Channel 0 Transmitter binary |
| `make ch1` | Channel 1 Receiver + Verifier binary |
| `make websocket` | WebSocket + HTTP server binary |
| `make lib` | Shared library (libnsigii.so) |
| `make build-all` | All binaries + shared library |
| `make clean` | Remove build artifacts |
| `make debug` | Debug build |
| `make test` | Build and run tests |

---

## Running the System

### Start Order

1. **WebSocket Server** (`make run-websocket`) - Serves web UI and accepts WebSocket connections
2. **Channel 0** (`make run-ch0`) - Transmitter begins encoding and sending packets
3. **Channel 1** (`make run-ch1`) - Receiver listens on port 8002, Verifier on port 8003

### Run Commands

```bash
# Terminal 1: WebSocket + HTTP server
make run-websocket

# Terminal 2: Transmitter
make run-ch0

# Terminal 3: Receiver + Verifier
make run-ch1
```

### Run Targets

| Target | Description |
|--------|-------------|
| `make run-ch0` | Run Channel 0 Transmitter |
| `make run-ch1` | Run Channel 1 Receiver + Verifier |
| `make run-websocket` | Run WebSocket server (port 8080, serves HTTP + WS) |
| `make run-web` | Python HTTP server fallback (port 8000) |
| `make run-all` | Display run instructions |

---

## Web Interface

### Embedded HTTP Server (Primary)

The WebSocket server (port 8080) now serves both WebSocket data and static HTML pages. Navigate to:

```
http://localhost:8080/
```

This serves `pages/mac2loopback/html/index.html` and all associated JS/CSS assets.

### Python Fallback Server

If the embedded server is not running or you need a separate HTTP server:

```bash
make run-web
# Then open: http://localhost:8000/html/index.html
```

### Web Interface Features

- Real-time WebSocket connection status indicator
- Rational Wheel position display
- Consensus score monitoring
- Trident topology visualization (Transmitter -> Receiver -> Verifier)
- MAC-to-Loopback address mapping table
- Message transmission interface

### Web Interface Files

```
pages/mac2loopback/
  html/index.html        # Main UI - Trident C&C dashboard
  js/main.mjs            # Application entry point
  js/websocket.mjs       # WebSocket client module
  js/dop-adapter.mjs     # Data-Oriented Programming adapter
  js/xhr.mjs             # XHR fallback communication
  styles/nsigii.css      # NSIGII styling
```

---

## Protocol Specification

### Trident Packet Structure

```c
typedef struct {
    nsigii_header_t       header;       // Channel, sequence, timestamp
    nsigii_payload_t      payload;      // Content + SHA-256 hash
    nsigii_verification_t verification; // RWX flags + consensus signature
    nsigii_topology_t     topology;     // Wheel position + channel links
} nsigii_packet_t;
```

### RWX Permissions

- **WRITE (0x02):** Transmitter - Can encode and send
- **READ (0x04):** Receiver - Can decode and verify hash
- **EXECUTE (0x01):** Verifier - Can validate and sign
- **FULL (0x07):** Verified packet - All permissions granted

### Serialization Layers

| Layer | Format | Usage |
|-------|--------|-------|
| Binary | Custom binary wire format | Inter-channel UDP communication |
| JSON | Structured JSON | WebSocket client communication |
| Canonical IR | Intermediate representation | Polyglot FFI bridge |

### Human Rights Tags

Valid protocol verification identifiers:
- `NSIGII_HR_TRANSMIT`
- `NSIGII_HR_RECEIVE`
- `NSIGII_HR_VERIFY`
- `NSIGII_HR_VERIFIED`

### DNS Namespace

All channels register under `nsigii.humanrights.local`:
- `transmitter.nsigii.humanrights.local`
- `receiver.nsigii.humanrights.local`
- `verifier.nsigii.humanrights.local`

---

## Data Flow

```
1. CH0 Transmitter (WRITE):
   Content -> Silicon-to-Suffering Encoding -> SHA-256 Hash
   -> Binary Serialize -> UDP sendto 127.0.0.2:8002

2. CH1 Receiver (READ):
   UDP recvfrom 8002 -> Binary Deserialize -> Silicon Decode
   -> Hash Verify -> Bipolar State Update (Order/Chaos)
   -> Forward to Verifier -> UDP sendto 127.0.0.3:8003

3. CH2 Verifier (EXECUTE):
   UDP recvfrom 8003 -> RWX Chain Validate
   -> Bipartite Consensus (2/3 threshold)
   -> Human Rights Tag Check -> Wheel Position Check
   -> HMAC-SHA256 Consensus Signature
   -> Broadcast consensus to all channels

4. WebSocket Server (port 8080):
   HTTP GET requests -> Serve static files (pages/mac2loopback/)
   WebSocket connections -> JSON serialize packets -> Broadcast to clients
```

---

## MAC to Loopback Mapping

| MAC Address | Channel | Loopback | DNS Name |
|-------------|---------|----------|----------|
| 02:00:00:00:00:00 | Transmitter | 127.0.0.1 | transmitter.nsigii.humanrights.local |
| 02:00:00:00:00:01 | Receiver | 127.0.0.2 | receiver.nsigii.humanrights.local |
| 02:00:00:00:00:02 | Verifier | 127.0.0.3 | verifier.nsigii.humanrights.local |

---

## WebSocket API

### Connection

```javascript
const ws = new WebSocket('ws://localhost:8080');
```

### Sending Messages

```javascript
// Send text
ws.send('Hello, NSIGII!');

// Send packet (JSON)
ws.send(JSON.stringify({
    header: { channel_id: 0, sequence_token: 1 },
    payload: { content: "data" },
    verification: { rwx_flags: 0x02 },
    topology: { wheel_position: 0 }
}));
```

### Receiving Messages

```javascript
ws.onmessage = (event) => {
    const packet = JSON.parse(event.data);
    console.log('Verified packet:', packet);
};
```

---

## DOP Adapter

### Data-Oriented Programming Component

```javascript
import { DOPAdapter, ButtonLogic } from './dop-adapter.mjs';

const adapter = new DOPAdapter(ButtonLogic);
const FuncButton = adapter.toFunctional();
const OOPButton = adapter.toOOP();
```

---

## Platform Notes

### Windows (MSYS2/MinGW)

- Uses Winsock2 (`ws2_32`) for socket operations
- `closesocket()` instead of `close()`
- `ioctlsocket(FIONBIO)` instead of `fcntl(O_NONBLOCK)`
- `Sleep()` instead of `sleep()`/`usleep()`
- OpenSSL path: `/c/msys64/mingw64/include/` and `/c/msys64/mingw64/lib/`
- Build with: `mingw32-make` or `make` (from MSYS2 MinGW64 shell)

### Linux / WSL

- Standard POSIX sockets (`sys/socket.h`, `netinet/in.h`, `arpa/inet.h`)
- Install `libssl-dev` for OpenSSL development headers
- Build with: `make`
- The Makefile auto-detects platform via `$(OS)` to exclude Windows-only `-lws2_32`

---

## Troubleshooting

### "bind: Address already in use"

The socket port is still held by a previous process. Solutions:
1. Wait a few seconds for the OS to release the port
2. Kill running processes:
   - Linux: `pkill nsigii` or `kill $(lsof -t -i:8002)`
   - Windows: `taskkill /IM nsigii_ch1.exe /F`
3. The system uses `SO_REUSEADDR` on all sockets to minimize this issue

### "openssl/sha.h: No such file or directory"

Install OpenSSL development headers:
- **MSYS2**: `pacman -S mingw-w64-x86_64-openssl`
- **Ubuntu/WSL**: `sudo apt-get install libssl-dev`
- **Fedora**: `sudo dnf install openssl-devel`

### "-lws2_32: not found" (Building from WSL)

This occurs when Windows linker flags are used on Linux. The Makefile auto-detects via `$(OS)`. Ensure you're running `make` from the correct shell:
- MSYS2/MinGW: use `mingw32-make` or `make` from MinGW64 shell
- WSL/Linux: use `make` from bash

### WebSocket server not serving web pages

The WebSocket server embeds HTTP static file serving. When you navigate to `http://localhost:8080/`, it serves `pages/mac2loopback/html/index.html`. Ensure:
1. The server is running: `make run-websocket`
2. The `pages/mac2loopback/` directory exists relative to where the server was launched
3. Fallback: use `make run-web` to start a Python HTTP server on port 8000

### Channel 1 fails to start

Ensure no other processes are using ports 8002 (Receiver) or 8003 (Verifier). Check with:
- Linux: `ss -tlnp | grep -E '800[23]'`
- Windows: `netstat -ano | findstr "800[23]"`

---

## License

NSIGII Human Rights Verification System - OBINexus

## Author

NSIGII Protocol Team - OBINexus

## References

- Rectorial Reasoning Rational Wheel Framework
- Trident Topology: Bipartite Order & Chaos
- "Encoding Suffering into Silicon" - NSIGII Codec Philosophy
- Brachistochrone Optimization - Cycloid curve descent for relay optimization
- OBIFFT - Triangular matrix FFT transformation pipeline
- LibPolyCall - "All Bindings Are Drivers" polyglot FFI framework
