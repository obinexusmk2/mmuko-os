/* ============================================================================
 * NSIGII SERIALIZATION MODULE IMPLEMENTATION
 * Data Structure Serialization/Deserialization for Cross-Language Communication
 * ============================================================================ */

#include "serialization.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>

/* ============================================================================
 * BINARY PROTOCOL SERIALIZATION
 * ============================================================================ */

int nsigii_serialize_binary(const nsigii_packet_t* packet, 
                             uint8_t* buffer, 
                             size_t* len) {
    if (!packet || !buffer || !len) return -1;
    
    size_t offset = 0;
    
    /* Serialize header */
    buffer[offset++] = packet->header.channel_id;
    memcpy(buffer + offset, &packet->header.sequence_token, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(buffer + offset, &packet->header.timestamp, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    buffer[offset++] = packet->header.codec_version;
    
    /* Serialize payload */
    memcpy(buffer + offset, packet->payload.message_hash, HASH_SIZE);
    offset += HASH_SIZE;
    memcpy(buffer + offset, &packet->payload.content_length, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(buffer + offset, packet->payload.content, packet->payload.content_length);
    offset += packet->payload.content_length;
    
    /* Serialize verification */
    buffer[offset++] = packet->verification.rwx_flags;
    memcpy(buffer + offset, packet->verification.consensus_sig, SIGNATURE_SIZE);
    offset += SIGNATURE_SIZE;
    size_t tag_len = strlen(packet->verification.human_rights_tag) + 1;
    memcpy(buffer + offset, packet->verification.human_rights_tag, tag_len);
    offset += tag_len;
    
    /* Serialize topology */
    buffer[offset++] = packet->topology.next_channel;
    buffer[offset++] = packet->topology.prev_channel;
    memcpy(buffer + offset, &packet->topology.wheel_position, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    
    *len = offset;
    return 0;
}

int nsigii_deserialize_binary(const uint8_t* buffer, 
                               size_t len, 
                               nsigii_packet_t* packet) {
    if (!buffer || !packet || len < 100) return -1;
    
    size_t offset = 0;
    
    /* Deserialize header */
    packet->header.channel_id = buffer[offset++];
    memcpy(&packet->header.sequence_token, buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&packet->header.timestamp, buffer + offset, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    packet->header.codec_version = buffer[offset++];
    
    /* Deserialize payload */
    memcpy(packet->payload.message_hash, buffer + offset, HASH_SIZE);
    offset += HASH_SIZE;
    memcpy(&packet->payload.content_length, buffer + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    if (packet->payload.content_length > MAX_CONTENT_SIZE) return -1;
    
    memcpy(packet->payload.content, buffer + offset, packet->payload.content_length);
    offset += packet->payload.content_length;
    
    /* Deserialize verification */
    packet->verification.rwx_flags = buffer[offset++];
    memcpy(packet->verification.consensus_sig, buffer + offset, SIGNATURE_SIZE);
    offset += SIGNATURE_SIZE;
    strncpy(packet->verification.human_rights_tag, (char*)(buffer + offset), MAX_TAG_SIZE - 1);
    packet->verification.human_rights_tag[MAX_TAG_SIZE - 1] = '\0';
    offset += strlen(packet->verification.human_rights_tag) + 1;
    
    /* Deserialize topology */
    packet->topology.next_channel = buffer[offset++];
    packet->topology.prev_channel = buffer[offset++];
    memcpy(&packet->topology.wheel_position, buffer + offset, sizeof(uint16_t));
    offset += sizeof(uint16_t);
    
    return 0;
}

size_t nsigii_binary_size(const nsigii_packet_t* packet) {
    if (!packet) return 0;
    
    size_t size = 0;
    size += 1 + sizeof(uint32_t) + sizeof(uint64_t) + 1;  /* Header */
    size += HASH_SIZE + sizeof(uint32_t) + packet->payload.content_length;  /* Payload */
    size += 1 + SIGNATURE_SIZE + strlen(packet->verification.human_rights_tag) + 1;  /* Verification */
    size += 1 + 1 + sizeof(uint16_t);  /* Topology */
    
    return size;
}

/* ============================================================================
 * JSON SERIALIZATION (WebSocket)
 * ============================================================================ */

static int json_encode_base64(const uint8_t* data, size_t len, char* output) {
    static const char base64_chars[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    size_t i, j;
    for (i = 0, j = 0; i < len; i += 3, j += 4) {
        uint32_t triple = (data[i] << 16);
        if (i + 1 < len) triple |= (data[i + 1] << 8);
        if (i + 2 < len) triple |= data[i + 2];
        
        output[j] = base64_chars[(triple >> 18) & 0x3F];
        output[j + 1] = base64_chars[(triple >> 12) & 0x3F];
        output[j + 2] = (i + 1 < len) ? base64_chars[(triple >> 6) & 0x3F] : '=';
        output[j + 3] = (i + 2 < len) ? base64_chars[triple & 0x3F] : '=';
    }
    output[j] = '\0';
    return 0;
}

int nsigii_serialize_json(const nsigii_packet_t* packet, 
                           char* buffer, 
                           size_t max_len) {
    if (!packet || !buffer || max_len < 512) return -1;
    
    char hash_b64[64];
    char sig_b64[128];
    char content_b64[MAX_CONTENT_SIZE * 2];
    
    json_encode_base64(packet->payload.message_hash, HASH_SIZE, hash_b64);
    json_encode_base64(packet->verification.consensus_sig, SIGNATURE_SIZE, sig_b64);
    json_encode_base64(packet->payload.content, packet->payload.content_length, content_b64);
    
    int written = snprintf(buffer, max_len,
        "{"
        "\"header\":{"
            "\"channel_id\":%d,"
            "\"sequence_token\":%u,"
            "\"timestamp\":%llu,"
            "\"codec_version\":%d"
        "},"
        "\"payload\":{"
            "\"message_hash\":\"%s\","
            "\"content_length\":%u,"
            "\"content\":\"%s\""
        "},"
        "\"verification\":{"
            "\"rwx_flags\":%d,"
            "\"consensus_sig\":\"%s\","
            "\"human_rights_tag\":\"%s\""
        "},"
        "\"topology\":{"
            "\"next_channel\":%d,"
            "\"prev_channel\":%d,"
            "\"wheel_position\":%d"
        "}"
        "}",
        packet->header.channel_id,
        packet->header.sequence_token,
        (unsigned long long)packet->header.timestamp,
        packet->header.codec_version,
        hash_b64,
        packet->payload.content_length,
        content_b64,
        packet->verification.rwx_flags,
        sig_b64,
        packet->verification.human_rights_tag,
        packet->topology.next_channel,
        packet->topology.prev_channel,
        packet->topology.wheel_position
    );
    
    return (written > 0 && (size_t)written < max_len) ? 0 : -1;
}

/* Simple JSON parser - extracts integer value */
static int json_extract_int(const char* json, const char* key, int* value) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":%%d", key);
    return sscanf(json, pattern, value) == 1 ? 0 : -1;
}

/* Simple JSON parser - extracts string value */
static int json_extract_string(const char* json, const char* key, char* value, size_t max_len) {
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\":\"%%[^\"]\"", key);
    return sscanf(json, pattern, value) == 1 ? 0 : -1;
}

int nsigii_deserialize_json(const char* json, 
                             nsigii_packet_t* packet) {
    if (!json || !packet) return -1;
    
    /* Extract header fields */
    json_extract_int(json, "channel_id", (int*)&packet->header.channel_id);
    json_extract_int(json, "sequence_token", (int*)&packet->header.sequence_token);
    
    long long timestamp;
    if (sscanf(json, "\"timestamp\":%lld", &timestamp) == 1) {
        packet->header.timestamp = (uint64_t)timestamp;
    }
    
    json_extract_int(json, "codec_version", (int*)&packet->header.codec_version);
    
    /* Extract payload fields */
    json_extract_int(json, "content_length", (int*)&packet->payload.content_length);
    
    /* Extract verification fields */
    json_extract_int(json, "rwx_flags", (int*)&packet->verification.rwx_flags);
    json_extract_string(json, "human_rights_tag", 
                        packet->verification.human_rights_tag, MAX_TAG_SIZE);
    
    /* Extract topology fields */
    json_extract_int(json, "next_channel", (int*)&packet->topology.next_channel);
    json_extract_int(json, "prev_channel", (int*)&packet->topology.prev_channel);
    json_extract_int(json, "wheel_position", (int*)&packet->topology.wheel_position);
    
    return 0;
}

/* ============================================================================
 * WEBSOCKET FRAME HANDLING
 * ============================================================================ */

int nsigii_ws_encode_frame(const nsigii_ws_frame_t* frame, 
                            uint8_t* buffer, 
                            size_t* len) {
    if (!frame || !buffer || !len) return -1;
    
    size_t offset = 0;
    
    /* First byte: FIN, RSV, OPCODE */
    buffer[offset] = (frame->fin << 7) | (frame->opcode & 0x0F);
    offset++;
    
    /* Second byte: MASK, PAYLOAD LEN */
    uint8_t mask_bit = frame->masked ? 0x80 : 0x00;
    
    if (frame->payload_length < 126) {
        buffer[offset++] = mask_bit | (frame->payload_length & 0x7F);
    } else if (frame->payload_length < 65536) {
        buffer[offset++] = mask_bit | 126;
        buffer[offset++] = (frame->payload_length >> 8) & 0xFF;
        buffer[offset++] = frame->payload_length & 0xFF;
    } else {
        buffer[offset++] = mask_bit | 127;
        for (int i = 7; i >= 0; i--) {
            buffer[offset++] = (frame->payload_length >> (i * 8)) & 0xFF;
        }
    }
    
    /* Masking key if masked */
    if (frame->masked) {
        memcpy(buffer + offset, frame->masking_key, 4);
        offset += 4;
    }
    
    /* Payload (apply mask if needed) */
    if (frame->masked) {
        for (size_t i = 0; i < frame->payload_length; i++) {
            buffer[offset + i] = frame->payload[i] ^ frame->masking_key[i % 4];
        }
    } else {
        memcpy(buffer + offset, frame->payload, frame->payload_length);
    }
    offset += frame->payload_length;
    
    *len = offset;
    return 0;
}

int nsigii_ws_decode_frame(const uint8_t* buffer, 
                            size_t len, 
                            nsigii_ws_frame_t* frame,
                            size_t* bytes_consumed) {
    if (!buffer || !frame || len < 2) return -1;
    
    size_t offset = 0;
    
    /* First byte: FIN, RSV, OPCODE */
    frame->fin = (buffer[offset] >> 7) & 0x01;
    frame->rsv = (buffer[offset] >> 4) & 0x07;
    frame->opcode = buffer[offset] & 0x0F;
    offset++;
    
    /* Second byte: MASK, PAYLOAD LEN */
    frame->masked = (buffer[offset] >> 7) & 0x01;
    uint8_t payload_len = buffer[offset] & 0x7F;
    offset++;
    
    /* Extended payload length */
    if (payload_len == 126) {
        if (len < offset + 2) return -1;
        frame->payload_length = ((uint64_t)buffer[offset] << 8) | buffer[offset + 1];
        offset += 2;
    } else if (payload_len == 127) {
        if (len < offset + 8) return -1;
        frame->payload_length = 0;
        for (int i = 0; i < 8; i++) {
            frame->payload_length = (frame->payload_length << 8) | buffer[offset + i];
        }
        offset += 8;
    } else {
        frame->payload_length = payload_len;
    }
    
    /* Masking key */
    if (frame->masked) {
        if (len < offset + 4) return -1;
        memcpy(frame->masking_key, buffer + offset, 4);
        offset += 4;
    }
    
    /* Payload */
    if (len < offset + frame->payload_length) return -1;
    
    frame->payload = (uint8_t*)malloc(frame->payload_length);
    if (!frame->payload) return -1;
    
    if (frame->masked) {
        for (size_t i = 0; i < frame->payload_length; i++) {
            frame->payload[i] = buffer[offset + i] ^ frame->masking_key[i % 4];
        }
    } else {
        memcpy(frame->payload, buffer + offset, frame->payload_length);
    }
    offset += frame->payload_length;
    
    *bytes_consumed = offset;
    return 0;
}

void nsigii_ws_frame_cleanup(nsigii_ws_frame_t* frame) {
    if (frame && frame->payload) {
        free(frame->payload);
        frame->payload = NULL;
    }
}

/* ============================================================================
 * WEBSOCKET HANDSHAKE
 * ============================================================================ */

int nsigii_ws_generate_accept_key(const char* client_key, char* accept_key) {
    if (!client_key || !accept_key) return -1;
    
    const char* magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char concat[128];
    
    snprintf(concat, sizeof(concat), "%s%s", client_key, magic);
    
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1((unsigned char*)concat, strlen(concat), hash);
    
    /* Base64 encode */
    static const char base64[] = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
    int j = 0;
    for (int i = 0; i < SHA_DIGEST_LENGTH; i += 3) {
        uint32_t triple = (hash[i] << 16);
        if (i + 1 < SHA_DIGEST_LENGTH) triple |= (hash[i + 1] << 8);
        if (i + 2 < SHA_DIGEST_LENGTH) triple |= hash[i + 2];
        
        accept_key[j++] = base64[(triple >> 18) & 0x3F];
        accept_key[j++] = base64[(triple >> 12) & 0x3F];
        accept_key[j++] = (i + 1 < SHA_DIGEST_LENGTH) ? 
                          base64[(triple >> 6) & 0x3F] : '=';
        accept_key[j++] = (i + 2 < SHA_DIGEST_LENGTH) ? 
                          base64[triple & 0x3F] : '=';
    }
    accept_key[j] = '\0';
    
    return 0;
}

int nsigii_ws_create_handshake_response(const char* client_key, char* response) {
    if (!client_key || !response) return -1;
    
    char accept_key[32];
    nsigii_ws_generate_accept_key(client_key, accept_key);
    
    snprintf(response, 512,
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        accept_key
    );
    
    return 0;
}

/* ============================================================================
 * HASH TABLE IMPLEMENTATION
 * ============================================================================ */

nsigii_hash_table_t* nsigii_hash_create(size_t size) {
    nsigii_hash_table_t* table = (nsigii_hash_table_t*)malloc(sizeof(nsigii_hash_table_t));
    if (!table) return NULL;
    
    table->buckets = (nsigii_hash_entry_t**)calloc(size, sizeof(nsigii_hash_entry_t*));
    if (!table->buckets) {
        free(table);
        return NULL;
    }
    
    table->size = size;
    table->count = 0;
    return table;
}

void nsigii_hash_destroy(nsigii_hash_table_t* table) {
    if (!table) return;
    
    for (size_t i = 0; i < table->size; i++) {
        nsigii_hash_entry_t* entry = table->buckets[i];
        while (entry) {
            nsigii_hash_entry_t* next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    
    free(table->buckets);
    free(table);
}

static unsigned int hash_function(const char* key, size_t size) {
    unsigned int hash = 5381;
    int c;
    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % size;
}

int nsigii_hash_insert(nsigii_hash_table_t* table, const char* key, void* value) {
    if (!table || !key) return -1;
    
    unsigned int index = hash_function(key, table->size);
    
    nsigii_hash_entry_t* entry = (nsigii_hash_entry_t*)malloc(sizeof(nsigii_hash_entry_t));
    if (!entry) return -1;
    
    entry->key = strdup(key);
    entry->value = value;
    entry->next = table->buckets[index];
    table->buckets[index] = entry;
    table->count++;
    
    return 0;
}

void* nsigii_hash_lookup(nsigii_hash_table_t* table, const char* key) {
    if (!table || !key) return NULL;
    
    unsigned int index = hash_function(key, table->size);
    nsigii_hash_entry_t* entry = table->buckets[index];
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;
}

/* ============================================================================
 * QUEUE IMPLEMENTATION
 * ============================================================================ */

nsigii_queue_t* nsigii_queue_create(void) {
    nsigii_queue_t* queue = (nsigii_queue_t*)malloc(sizeof(nsigii_queue_t));
    if (!queue) return NULL;
    
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    return queue;
}

void nsigii_queue_destroy(nsigii_queue_t* queue) {
    if (!queue) return;
    
    while (!nsigii_queue_is_empty(queue)) {
        nsigii_queue_dequeue(queue);
    }
    free(queue);
}

int nsigii_queue_enqueue(nsigii_queue_t* queue, void* data) {
    if (!queue) return -1;
    
    nsigii_queue_node_t* node = (nsigii_queue_node_t*)malloc(sizeof(nsigii_queue_node_t));
    if (!node) return -1;
    
    node->data = data;
    node->next = NULL;
    
    if (queue->tail) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->count++;
    
    return 0;
}

void* nsigii_queue_dequeue(nsigii_queue_t* queue) {
    if (!queue || !queue->head) return NULL;
    
    nsigii_queue_node_t* node = queue->head;
    void* data = node->data;
    
    queue->head = node->next;
    if (!queue->head) {
        queue->tail = NULL;
    }
    queue->count--;
    
    free(node);
    return data;
}

bool nsigii_queue_is_empty(const nsigii_queue_t* queue) {
    return !queue || queue->count == 0;
}
