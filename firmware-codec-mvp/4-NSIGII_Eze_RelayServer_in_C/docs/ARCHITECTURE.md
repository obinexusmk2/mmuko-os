# NSIGII Architecture Documentation

## System Overview

The NSIGII Trident Command & Control system implements a three-channel verification topology designed for human rights protocol compliance.

## Core Components

### 1. Transmitter (Channel 0)

**Responsibilities:**
- Encode raw messages into NSIGII packet format
- Compute SHA-256 message hashes
- Set RWX WRITE permissions
- Tag with human rights identifiers
- Transmit to Receiver via loopback

**Codec Ratio:** 1/3

**State:** ORDER

### 2. Receiver (Channel 1)

**Responsibilities:**
- Receive packets from Transmitter
- Decode from silicon representation
- Verify hash integrity
- Set RWX READ permissions
- Bipartite order check (Order vs Chaos)
- Forward to Verifier

**Codec Ratio:** 2/3

**State:** CHAOS (bipolar)

### 3. Verifier (Channel 2)

**Responsibilities:**
- RWX Permission Validation
- Bipartite Consensus calculation
- Human Rights Protocol Compliance
- Wheel Position Integrity checks
- Consensus signature generation (HMAC-SHA256)
- Emit consensus messages

**Codec Ratio:** 3/3 (Complete)

**State:** VERIFIED

## Data Flow

```
┌─────────────┐    Encode     ┌─────────────┐    Decode     ┌─────────────┐
│   Raw Msg   │ ────────────→ │   Packet    │ ────────────→ │   Packet    │
│             │  Suffering→   │  CH0:WRITE  │  Silicon→     │  CH1:READ   │
│             │    Silicon    │   SHA-256   │   Suffering   │  Hash Check │
└─────────────┘               └─────────────┘               └──────┬──────┘
                                                                   │
                              ┌─────────────┐    Verify      ┌─────▼───────┐
                              │  Consensus  │ ←───────────── │   Packet    │
                              │   Message   │   RWX+Wheel    │  CH2:EXEC   │
                              │  BROADCAST  │   Consensus    │   HMAC-Sig  │
                              └─────────────┘               └─────────────┘
```

## Serialization Layers

### Layer 1: Binary Protocol
- Native C structure serialization
- Most efficient for inter-process communication
- Used between channels via loopback

### Layer 2: JSON Protocol
- WebSocket communication
- Human-readable format
- Used for web interface

### Layer 3: Canonical IR
- Cross-language binding
- Polyglot compatibility
- Driver interface format

## Consensus Algorithm

### Bipartite Consensus Formula

```
order_weight = count_set_bits(content)
total_bits = content_length * 8
base_consensus = order_weight / total_bits
wheel_correction = sin(wheel_position * π / 180)
final_consensus = |base_consensus + wheel_correction| / 2
```

### Verification Checks

1. **RWX Chain Validation**
   - Transmitter: WRITE (0x02)
   - Receiver: READ (0x04)
   - Verifier: EXECUTE (0x01)

2. **Consensus Threshold**
   - Minimum: 0.67 (2/3 majority)

3. **Human Rights Tag**
   - Must be in valid tag list

4. **Wheel Position**
   - Expected: 120° at receiver
   - Expected: 240° at verifier

## Tomographic Pair Resolving

### Pair Formation

Two consecutive packets form a tomographic pair:

```c
typedef struct {
    nsigii_packet_t* pair[2];
    double correlation;
    bool resolved;
} nsigii_tomographic_pair_t;
```

### Correlation Calculation

```
correlation = f(sequence_diff, wheel_diff)
            = 1/(1+|seq1-seq2|) + 1/(1+|wheel1-wheel2|/180)
```

### Resolution Criteria

- Both packets present
- Different sequence tokens
- Correlation >= 0.5

## WebSocket Protocol

### Frame Structure

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
|     Extended payload length continued, if payload len == 127  |
+ - - - - - - - - - - - - - - - +-------------------------------+
|                               |Masking-key, if MASK set to 1  |
+-------------------------------+-------------------------------+
| Masking-key (continued)       |          Payload Data         |
+-------------------------------- - - - - - - - - - - - - - - - +
:                     Payload Data continued ...                :
+ - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
|                     Payload Data continued ...                |
+---------------------------------------------------------------+
```

### Handshake

```
Client Request:
  GET / HTTP/1.1
  Host: localhost:8080
  Upgrade: websocket
  Connection: Upgrade
  Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
  Sec-WebSocket-Version: 13

Server Response:
  HTTP/1.1 101 Switching Protocols
  Upgrade: websocket
  Connection: Upgrade
  Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
```

## Driver Interface (Polyglot)

### Canonical IR Structure

```c
typedef struct {
    enum { FLOAT64, INT64, STRING, NESTED, BINARY, BOOL } type;
    union {
        double f64;
        int64_t i64;
        char* str;
        void* nested;
        struct { uint8_t* data; size_t len; } binary;
        bool boolean;
    } value;
} cir_value_t;
```

### Language Bindings

| Language | Native Type | CIR Transform | Driver Format |
|----------|-------------|---------------|---------------|
| Python | dict | CIR_Object | Binary protocol |
| Go | struct | CIR_Object | Binary protocol |
| C | struct | CIR_Object | Binary protocol |
| JavaScript | Object | CIR_Object | JSON/WS |
| Java | class | CIR_Object | Binary protocol |

## Security Considerations

### Cryptographic Functions

- **SHA-256:** Message hashing
- **HMAC-SHA256:** Consensus signatures
- **Random seeds:** Session keys

### Permission Model

- RWX flags enforce capability-based access
- Each channel has specific permissions
- Full RWX only after verification

### Network Security

- Loopback-only communication between channels
- WebSocket can be TLS-encrypted (wss://)
- No external network exposure required

## Performance Characteristics

### Latency

- Inter-channel: < 1ms (loopback)
- WebSocket: < 10ms (local)

### Throughput

- Binary protocol: ~100K packets/sec
- WebSocket: ~10K packets/sec

### Memory

- Per-packet: ~4KB max
- Channel state: ~1KB per channel

## Error Handling

### Error Codes

```c
#define NSIGII_OK           0
#define NSIGII_ERROR_INIT  -1
#define NSIGII_ERROR_NET   -2
#define NSIGII_ERROR_HASH  -3
#define NSIGII_ERROR_RWX   -4
#define NSIGII_ERROR_CONSENSUS -5
```

### Recovery Strategies

1. **Network errors:** Automatic reconnection
2. **Hash mismatch:** Packet discard
3. **Consensus failure:** Alert + log
4. **RWX violation:** Security alert

## Future Enhancements

1. **Multi-node clustering**
2. **Blockchain consensus integration**
3. **Quantum-resistant cryptography**
4. **Hardware security module (HSM) support**
