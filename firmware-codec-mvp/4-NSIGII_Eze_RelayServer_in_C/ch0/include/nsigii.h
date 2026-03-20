/* ============================================================================
 * NSIGII COMMAND AND CONTROL HUMAN RIGHTS VERIFICATION SYSTEM
 * Rectorial Reasoning Rational Wheel Framework - On-the-Fly Realtime Verification
 * Trident Topology: Transmitter | Receiver | Verifier (Bipartite Order & Chaos)
 * ============================================================================
 * 
 * Core Philosophy: "All Bindings Are Drivers"
 * SQUARE (Perfect Binding): Symmetric bidirectional FFI
 * RECTANGLE (Driver): Asymmetric request/response interface
 * 
 * Channel 0: Transmitter (1 * 1/3) - First Channel Codec
 * Channel 1: Receiver (2 * 2/3) - Triangle Second Receiver  
 * Channel 2: Verifier (3 * 3/3) - Angel Verification
 */

#ifndef NSIGII_H
#define NSIGII_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CONSTANTS & DEFINITIONS
 * ============================================================================ */

#define NSIGII_VERSION      "7.0.0"
#define TRIDENT_CHANNELS    3
#define CHANNEL_TRANSMITTER 0    /* 1 * 1/3 - First Channel Codec */
#define CHANNEL_RECEIVER    1    /* 2 * 2/3 - Triangle Second Receiver */
#define CHANNEL_VERIFIER    2    /* 3 * 3/3 - Angel Verification */

#define LOOPBACK_BASE       "127.0.0."
#define DNS_NAMESPACE       "nsigii.humanrights.local"
#define CONSENSUS_THRESHOLD 0.67  /* 2/3 majority for bipartite consensus */

#define STATE_ORDER         0x01  /* Bipolar sequence: Order */
#define STATE_CHAOS         0x00  /* Bipolar sequence: Chaos */
#define STATE_VERIFIED      0xFF  /* RWX Verification passed */

/* RWX Permission Flags */
#define RWX_READ            0x04
#define RWX_WRITE           0x02
#define RWX_EXECUTE         0x01
#define RWX_FULL            0x07

/* Port definitions for channels */
#define PORT_TRANSMITTER    8001
#define PORT_RECEIVER       8002
#define PORT_VERIFIER       8003
#define PORT_WEBSOCKET      8080

/* Buffer sizes */
#define MAX_CONTENT_SIZE    4096
#define HASH_SIZE           32
#define SIGNATURE_SIZE      64
#define MAX_TAG_SIZE        64
#define MAX_MAC_SIZE        18

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

/* Trident Packet Header (Codec Layer) */
typedef struct {
    uint8_t  channel_id;        /* 0, 1, or 2 */
    uint32_t sequence_token;    /* Incrementing thread-safe counter */
    uint64_t timestamp;         /* Nanoseconds since epoch */
    uint8_t  codec_version;     /* 1 * 1/3, 2 * 2/3, 3 * 3/3 scaled */
} nsigii_header_t;

/* Trident Packet Payload (Message Content) */
typedef struct {
    uint8_t  message_hash[HASH_SIZE];  /* SHA-256 of content */
    uint32_t content_length;
    uint8_t  content[MAX_CONTENT_SIZE]; /* Variable length */
} nsigii_payload_t;

/* Verification Layer (RWX - Read/Write/Execute) */
typedef struct {
    uint8_t  rwx_flags;                  /* Read=0x04, Write=0x02, Execute=0x01 */
    uint8_t  consensus_sig[SIGNATURE_SIZE]; /* Bipartite consensus signature */
    char     human_rights_tag[MAX_TAG_SIZE]; /* NSIGII protocol identifier */
} nsigii_verification_t;

/* Trident Topology Link */
typedef struct {
    uint8_t  next_channel;      /* Forward link */
    uint8_t  prev_channel;      /* Backward link */
    uint16_t wheel_position;    /* Position on Rational Wheel (0-360) */
} nsigii_topology_t;

/* Complete Trident Packet */
typedef struct {
    nsigii_header_t       header;
    nsigii_payload_t      payload;
    nsigii_verification_t verification;
    nsigii_topology_t     topology;
} nsigii_packet_t;

/* Verification Result */
typedef struct {
    char     status[32];
    nsigii_packet_t* verified_packet;
    double   consensus_score;
} nsigii_verification_result_t;

/* Consensus Message */
typedef struct {
    uint8_t  trident_hash[HASH_SIZE];
    uint64_t timestamp;
    char     status[32];
    char     wheel_position[16];
} nsigii_consensus_message_t;

/* DNS Record for MAC to Channel mapping */
typedef struct {
    char     name[64];
    char     type[8];
    char     value[16];  /* IP address */
    uint32_t ttl;
} nsigii_dns_record_t;

/* Channel Structure */
typedef struct {
    uint8_t  channel_id;
    char     loopback_addr[16];
    char     dns_name[64];
    double   codec_ratio;
    uint8_t  state;
    int      socket_fd;
    uint16_t port;
} nsigii_channel_t;

/* Log Entry for consensus events */
typedef struct {
    uint64_t timestamp;
    char     event_type[32];
    uint8_t  channel;
    uint32_t sequence;
    uint16_t wheel_pos;
} nsigii_log_entry_t;

/* ============================================================================
 * FUNCTION PROTOTYPES - Core System
 * ============================================================================ */

/* Initialization */
int nsigii_init(void);
void nsigii_shutdown(void);

/* Packet Operations */
nsigii_packet_t* nsigii_packet_create(void);
void nsigii_packet_destroy(nsigii_packet_t* packet);
int nsigii_packet_serialize(const nsigii_packet_t* packet, uint8_t* buffer, size_t* len);
int nsigii_packet_deserialize(const uint8_t* buffer, size_t len, nsigii_packet_t* packet);

/* Cryptographic Functions */
void nsigii_sha256(const uint8_t* data, size_t len, uint8_t* hash);
void nsigii_hmac_sha256(const uint8_t* key, size_t key_len, 
                        const uint8_t* data, size_t data_len, 
                        uint8_t* mac);

/* Encoding/Decoding - "Encoding Suffering into Silicon" */
int nsigii_suffering_to_silicon_encode(const uint8_t* input, size_t in_len, 
                                        uint8_t* output, size_t* out_len);
int nsigii_silicon_to_suffering_decode(const uint8_t* input, size_t in_len,
                                        uint8_t* output, size_t* out_len);

/* Consensus Functions */
double nsigii_compute_bipartite_consensus(const nsigii_packet_t* packet);
bool nsigii_validate_rwx_chain(const nsigii_packet_t* packet);
bool nsigii_verify_human_rights_tag(const nsigii_packet_t* packet);
void nsigii_generate_consensus_signature(const nsigii_packet_t* packet, 
                                          uint8_t* signature);

/* Rational Wheel Functions */
void nsigii_rotate_rational_wheel(uint16_t degrees);
uint16_t nsigii_get_wheel_position(void);
void nsigii_update_all_channels_wheel_position(uint16_t position);

/* Logging */
void nsigii_log_consensus_event(const char* event_type, const nsigii_packet_t* packet);
void nsigii_log_verification_failure(const nsigii_verification_result_t* result);

/* Utility */
uint64_t nsigii_now_ns(void);
void nsigii_generate_virtual_mac(int index, char* mac_buffer);
const char* nsigii_get_loopback_for_channel(uint8_t channel_id);

/* ============================================================================
 * CHANNEL-SPECIFIC FUNCTION PROTOTYPES
 * ============================================================================ */

/* Transmitter (Channel 0) - see transmitter/transmitter.h for full API */

/* Receiver (Channel 1) - see receiver/receiver.h for full API */

/* Verifier (Channel 2) - see verifier/verifier.h for full API */

/* DNS Resolver */
int nsigii_dns_register_mac_channel(const char* mac_address, uint8_t channel_id);
const char* nsigii_dns_resolve_channel(const char* mac_address);

/* ============================================================================
 * TOMOGRAPHIC PAIR RESOLVING
 * ============================================================================ */

/* Tomographic pair structure for cross-channel verification */
typedef struct {
    nsigii_packet_t* pair[2];      /* Pair of packets for tomographic analysis */
    double           correlation;   /* Correlation coefficient */
    bool             resolved;      /* Resolution status */
} nsigii_tomographic_pair_t;

/* Tomographic resolving functions */
int nsigii_tomographic_pair_create(nsigii_tomographic_pair_t* pair);
int nsigii_tomographic_pair_resolve(nsigii_tomographic_pair_t* pair);
double nsigii_tomographic_compute_correlation(const nsigii_packet_t* p1, 
                                               const nsigii_packet_t* p2);
bool nsigii_tomographic_validate_pair(const nsigii_tomographic_pair_t* pair);

/* ============================================================================
 * WEBSOCKET SERIALIZATION
 * ============================================================================ */

/* WebSocket frame types */
typedef enum {
    NSIGII_WS_CONTINUATION = 0x0,
    NSIGII_WS_TEXT = 0x1,
    NSIGII_WS_BINARY = 0x2,
    NSIGII_WS_CLOSE = 0x8,
    NSIGII_WS_PING = 0x9,
    NSIGII_WS_PONG = 0xA
} nsigii_ws_opcode_t;

/* WebSocket frame structure */
typedef struct {
    uint8_t  fin;
    uint8_t  rsv;
    uint8_t  opcode;
    uint8_t  masked;
    uint64_t payload_length;
    uint8_t  masking_key[4];
    uint8_t* payload;
} nsigii_ws_frame_t;

/* WebSocket serialization functions */
int nsigii_ws_serialize_packet(const nsigii_packet_t* packet, 
                                char* json_buffer, size_t* len);
int nsigii_ws_deserialize_packet(const char* json_buffer, size_t len,
                                  nsigii_packet_t* packet);
int nsigii_ws_frame_encode(const nsigii_ws_frame_t* frame, 
                            uint8_t* buffer, size_t* len);
int nsigii_ws_frame_decode(const uint8_t* buffer, size_t len,
                            nsigii_ws_frame_t* frame);

/* JSON serialization for WebSocket */
int nsigii_json_serialize_packet(const nsigii_packet_t* packet, char* buffer, size_t max_len);
int nsigii_json_deserialize_packet(const char* json, nsigii_packet_t* packet);

#ifdef __cplusplus
}
#endif

#endif /* NSIGII_H */
