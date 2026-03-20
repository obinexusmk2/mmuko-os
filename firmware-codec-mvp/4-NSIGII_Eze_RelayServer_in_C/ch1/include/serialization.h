/* ============================================================================
 * NSIGII SERIALIZATION MODULE
 * Data Structure Serialization/Deserialization for Cross-Language Communication
 * 
 * Core Philosophy: "All Bindings Are Drivers"
 * - SQUARE (Perfect Binding): Symmetric bidirectional FFI
 * - RECTANGLE (Driver): Asymmetric request/response interface
 * 
 * Serialization Formats:
 * - Binary Protocol: For C-to-C communication
 * - JSON: For WebSocket/JavaScript communication
 * - Canonical IR: For cross-language transformation
 * ============================================================================ */

#ifndef NSIGII_SERIALIZATION_H
#define NSIGII_SERIALIZATION_H

#include "nsigii.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * CANONICAL INTERMEDIATE REPRESENTATION (CIR)
 * For polyglot language binding transformation
 * ============================================================================ */

typedef enum {
    CIR_FLOAT64,
    CIR_INT64,
    CIR_STRING,
    CIR_NESTED,
    CIR_BINARY,
    CIR_BOOL
} cir_type_t;

typedef struct cir_value {
    cir_type_t type;
    union {
        double   f64;
        int64_t  i64;
        char*    str;
        void*    nested;
        struct {
            uint8_t* data;
            size_t   len;
        } binary;
        bool     boolean;
    } value;
} cir_value_t;

typedef struct cir_field {
    char*        key;
    cir_value_t  value;
} cir_field_t;

typedef struct cir_object {
    cir_field_t* fields;
    size_t       field_count;
} cir_object_t;

/* ============================================================================
 * SERIALIZATION FORMATS
 * ============================================================================ */

typedef enum {
    SERIALIZE_BINARY,    /* Native binary protocol */
    SERIALIZE_JSON,      /* JSON for WebSocket/web */
    SERIALIZE_XML,       /* XML for legacy systems */
    SERIALIZE_YAML,      /* YAML for configuration */
    SERIALIZE_CIR        /* Canonical IR for polyglot */
} serialize_format_t;

/* ============================================================================
 * FUNCTION PROTOTYPES - Binary Protocol
 * ============================================================================ */

/* Binary serialization - most efficient for C-to-C */
int nsigii_serialize_binary(const nsigii_packet_t* packet, 
                             uint8_t* buffer, 
                             size_t* len);
int nsigii_deserialize_binary(const uint8_t* buffer, 
                               size_t len, 
                               nsigii_packet_t* packet);
size_t nsigii_binary_size(const nsigii_packet_t* packet);

/* ============================================================================
 * FUNCTION PROTOTYPES - JSON Protocol (WebSocket)
 * ============================================================================ */

/* JSON serialization for WebSocket communication */
int nsigii_serialize_json(const nsigii_packet_t* packet, 
                           char* buffer, 
                           size_t max_len);
int nsigii_deserialize_json(const char* json, 
                             nsigii_packet_t* packet);

/* JSON helper functions */
int nsigii_json_escape_string(const char* input, char* output, size_t max_len);
int nsigii_json_unescape_string(const char* input, char* output, size_t max_len);
int nsigii_json_base64_encode(const uint8_t* data, size_t len, char* output);
int nsigii_json_base64_decode(const char* input, uint8_t* output, size_t* len);

/* ============================================================================
 * FUNCTION PROTOTYPES - Canonical IR (Polyglot)
 * ============================================================================ */

/* CIR for cross-language binding */
cir_object_t* nsigii_cir_from_packet(const nsigii_packet_t* packet);
nsigii_packet_t* nsigii_packet_from_cir(const cir_object_t* cir);
void nsigii_cir_destroy(cir_object_t* cir);

/* CIR serialization */
int nsigii_cir_serialize_binary(const cir_object_t* cir, uint8_t* buffer, size_t* len);
int nsigii_cir_deserialize_binary(const uint8_t* buffer, size_t len, cir_object_t* cir);

/* ============================================================================
 * FUNCTION PROTOTYPES - WebSocket Frame Handling
 * ============================================================================ */

/* WebSocket frame operations */
typedef struct {
    uint8_t  fin;
    uint8_t  rsv;
    uint8_t  opcode;
    uint8_t  masked;
    uint64_t payload_length;
    uint8_t  masking_key[4];
    uint8_t* payload;
} nsigii_ws_frame_t;

/* WebSocket frame encoding/decoding */
int nsigii_ws_encode_frame(const nsigii_ws_frame_t* frame, 
                            uint8_t* buffer, 
                            size_t* len);
int nsigii_ws_decode_frame(const uint8_t* buffer, 
                            size_t len, 
                            nsigii_ws_frame_t* frame,
                            size_t* bytes_consumed);
void nsigii_ws_frame_cleanup(nsigii_ws_frame_t* frame);

/* WebSocket handshake */
int nsigii_ws_generate_accept_key(const char* client_key, char* accept_key);
int nsigii_ws_validate_handshake_request(const char* request, char* client_key);
int nsigii_ws_create_handshake_response(const char* client_key, char* response);

/* ============================================================================
 * FUNCTION PROTOTYPES - Data Transformation
 * ============================================================================ */

/* "Encoding Suffering into Silicon" - NSIGII codec transformation */
int nsigii_codec_encode(const uint8_t* input, size_t in_len,
                         uint8_t* output, size_t* out_len);
int nsigii_codec_decode(const uint8_t* input, size_t in_len,
                         uint8_t* output, size_t* out_len);

/* Huffman-AVL AST Isomorphism for schema transformation */
typedef struct nsigii_ast_node {
    char*                    key;
    cir_type_t               type;
    void*                    value;
    struct nsigii_ast_node*  left;
    struct nsigii_ast_node*  right;
    int                      height;
} nsigii_ast_node_t;

nsigii_ast_node_t* nsigii_ast_create(const char* key, cir_type_t type, void* value);
void nsigii_ast_destroy(nsigii_ast_node_t* node);
nsigii_ast_node_t* nsigii_ast_insert(nsigii_ast_node_t* root, const char* key, 
                                      cir_type_t type, void* value);
nsigii_ast_node_t* nsigii_ast_search(nsigii_ast_node_t* root, const char* key);
int nsigii_ast_height(nsigii_ast_node_t* node);

/* ============================================================================
 * FUNCTION PROTOTYPES - Driver Interface (Rectangle Binding)
 * ============================================================================ */

/* Driver registration for polyglot bindings */
typedef struct {
    uint8_t  seed[32];      /* Cryptographic seed */
    uint64_t session;       /* Session identifier */
    uint32_t sequence;      /* Message sequence */
    uint16_t checksum;      /* Data integrity */
} nsigii_driver_header_t;

/* Driver interface functions */
int nsigii_driver_register(const char* binding_name, uint8_t* seed);
int nsigii_driver_send(const nsigii_driver_header_t* header, 
                        const uint8_t* data, 
                        size_t len);
int nsigii_driver_receive(nsigii_driver_header_t* header, 
                           uint8_t* buffer, 
                           size_t* len);

/* Type coercion matrix for cross-language compatibility */
typedef enum {
    COERCE_SAFE,        /* No data loss */
    COERCE_LOSSY,       /* Potential data loss */
    COERCE_INVALID      /* Cannot coerce */
} coerce_result_t;

coerce_result_t nsigii_can_coerce(cir_type_t from, cir_type_t to);
int nsigii_coerce_value(const cir_value_t* from, cir_value_t* to);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/* Hash table for DNS resolution */
typedef struct nsigii_hash_entry {
    char*                    key;
    void*                    value;
    struct nsigii_hash_entry* next;
} nsigii_hash_entry_t;

typedef struct {
    nsigii_hash_entry_t** buckets;
    size_t                size;
    size_t                count;
} nsigii_hash_table_t;

nsigii_hash_table_t* nsigii_hash_create(size_t size);
void nsigii_hash_destroy(nsigii_hash_table_t* table);
int nsigii_hash_insert(nsigii_hash_table_t* table, const char* key, void* value);
void* nsigii_hash_lookup(nsigii_hash_table_t* table, const char* key);
int nsigii_hash_remove(nsigii_hash_table_t* table, const char* key);

/* Message queue for async communication */
typedef struct nsigii_queue_node {
    void*                    data;
    struct nsigii_queue_node* next;
} nsigii_queue_node_t;

typedef struct {
    nsigii_queue_node_t* head;
    nsigii_queue_node_t* tail;
    size_t               count;
} nsigii_queue_t;

nsigii_queue_t* nsigii_queue_create(void);
void nsigii_queue_destroy(nsigii_queue_t* queue);
int nsigii_queue_enqueue(nsigii_queue_t* queue, void* data);
void* nsigii_queue_dequeue(nsigii_queue_t* queue);
bool nsigii_queue_is_empty(const nsigii_queue_t* queue);

#ifdef __cplusplus
}
#endif

#endif /* NSIGII_SERIALIZATION_H */
