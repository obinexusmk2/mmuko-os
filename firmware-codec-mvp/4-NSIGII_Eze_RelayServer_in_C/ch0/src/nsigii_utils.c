/* ============================================================================
 * NSIGII UTILITY FUNCTIONS
 * Core system utilities and helper functions
 * ============================================================================ */

#include "../include/nsigii.h"
#include "../include/serialization.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static uint16_t g_wheel_position = 0;
static nsigii_hash_table_t* g_dns_table = NULL;

/* ============================================================================
 * INITIALIZATION
 * ============================================================================ */

int nsigii_init(void) {
    /* Initialize DNS table */
    g_dns_table = nsigii_hash_create(64);
    if (!g_dns_table) return -1;
    
    /* Seed random */
    srand((unsigned int)time(NULL));
    
    return 0;
}

void nsigii_shutdown(void) {
    if (g_dns_table) {
        nsigii_hash_destroy(g_dns_table);
        g_dns_table = NULL;
    }
}

/* ============================================================================
 * PACKET OPERATIONS
 * ============================================================================ */

nsigii_packet_t* nsigii_packet_create(void) {
    nsigii_packet_t* packet = (nsigii_packet_t*)malloc(sizeof(nsigii_packet_t));
    if (packet) {
        memset(packet, 0, sizeof(nsigii_packet_t));
    }
    return packet;
}

void nsigii_packet_destroy(nsigii_packet_t* packet) {
    free(packet);
}

int nsigii_packet_serialize(const nsigii_packet_t* packet, uint8_t* buffer, size_t* len) {
    return nsigii_serialize_binary(packet, buffer, len);
}

int nsigii_packet_deserialize(const uint8_t* buffer, size_t len, nsigii_packet_t* packet) {
    return nsigii_deserialize_binary(buffer, len, packet);
}

/* ============================================================================
 * CRYPTOGRAPHIC FUNCTIONS
 * ============================================================================ */

void nsigii_sha256(const uint8_t* data, size_t len, uint8_t* hash) {
    if (!data || !hash) return;
    SHA256(data, len, hash);
}

void nsigii_hmac_sha256(const uint8_t* key, size_t key_len,
                        const uint8_t* data, size_t data_len,
                        uint8_t* mac) {
    if (!key || !data || !mac) return;
    
    unsigned int mac_len = SIGNATURE_SIZE;
    HMAC(EVP_sha256(), key, (int)key_len, data, data_len, mac, &mac_len);
}

/* ============================================================================
 * ENCODING/DECODING - "Encoding Suffering into Silicon"
 * ============================================================================ */

int nsigii_suffering_to_silicon_encode(const uint8_t* input, size_t in_len,
                                        uint8_t* output, size_t* out_len) {
    if (!input || !output || !out_len || *out_len < in_len) return -1;
    
    /* Simple XOR encoding with pattern - represents "encoding suffering into silicon" */
    const uint8_t pattern[] = {0x4E, 0x53, 0x49, 0x47, 0x49, 0x49}; /* "NSIGII" */
    
    for (size_t i = 0; i < in_len; i++) {
        output[i] = input[i] ^ pattern[i % sizeof(pattern)];
    }
    
    *out_len = in_len;
    return 0;
}

int nsigii_silicon_to_suffering_decode(const uint8_t* input, size_t in_len,
                                        uint8_t* output, size_t* out_len) {
    /* XOR is symmetric - encoding and decoding are the same operation */
    return nsigii_suffering_to_silicon_encode(input, in_len, output, out_len);
}

/* ============================================================================
 * CONSENSUS FUNCTIONS
 * ============================================================================ */

double nsigii_compute_bipartite_consensus(const nsigii_packet_t* packet) {
    if (!packet || packet->payload.content_length == 0) return 0.0;
    
    /* Count order bits (set bits) vs chaos bits (unset bits) */
    double order_count = 0;
    double total_bits = packet->payload.content_length * 8.0;
    
    for (uint32_t i = 0; i < packet->payload.content_length; i++) {
        uint8_t byte = packet->payload.content[i];
        for (int j = 0; j < 8; j++) {
            if (byte & (1 << j)) {
                order_count++;
            }
        }
    }
    
    double consensus = order_count / total_bits;
    
    /* Apply wheel correction */
    double wheel_rad = g_wheel_position * M_PI / 180.0;
    double correction = sin(wheel_rad);
    
    return fabs(consensus + correction) / 2.0;
}

bool nsigii_validate_rwx_chain(const nsigii_packet_t* packet) {
    if (!packet) return false;
    
    /* Check that RWX flags form a valid progression */
    uint8_t flags = packet->verification.rwx_flags;
    
    /* At minimum, should have read permission at receiver stage */
    return (flags & (RWX_READ | RWX_WRITE)) != 0;
}

bool nsigii_verify_human_rights_tag(const nsigii_packet_t* packet) {
    if (!packet) return false;
    
    const char* valid_tags[] = {
        "NSIGII_HR_TRANSMIT",
        "NSIGII_HR_RECEIVE",
        "NSIGII_HR_VERIFY",
        "NSIGII_HR_VERIFIED"
    };
    
    size_t num_tags = sizeof(valid_tags) / sizeof(valid_tags[0]);
    
    for (size_t i = 0; i < num_tags; i++) {
        if (strcmp(packet->verification.human_rights_tag, valid_tags[i]) == 0) {
            return true;
        }
    }
    
    return false;
}

void nsigii_generate_consensus_signature(const nsigii_packet_t* packet,
                                          uint8_t* signature) {
    if (!packet || !signature) return;
    
    /* Create signature input from packet components */
    uint8_t sig_input[512];
    size_t offset = 0;
    
    /* Add sequence token */
    memcpy(sig_input + offset, &packet->header.sequence_token, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    /* Add message hash */
    memcpy(sig_input + offset, packet->payload.message_hash, HASH_SIZE);
    offset += HASH_SIZE;
    
    /* Add HR tag */
    size_t tag_len = strlen(packet->verification.human_rights_tag);
    memcpy(sig_input + offset, packet->verification.human_rights_tag, tag_len);
    offset += tag_len;
    
    /* Add consensus marker */
    const char* marker = "NSIGII_CONSENSUS";
    memcpy(sig_input + offset, marker, strlen(marker));
    offset += strlen(marker);
    
    /* Generate key from wheel position */
    uint8_t key[32];
    memset(key, 0, sizeof(key));
    key[0] = (uint8_t)(g_wheel_position & 0xFF);
    key[1] = (uint8_t)((g_wheel_position >> 8) & 0xFF);
    
    /* Generate HMAC-SHA256 */
    nsigii_hmac_sha256(key, sizeof(key), sig_input, offset, signature);
}

/* ============================================================================
 * RATIONAL WHEEL FUNCTIONS
 * ============================================================================ */

void nsigii_rotate_rational_wheel(uint16_t degrees) {
    g_wheel_position = (g_wheel_position + degrees) % 360;
}

uint16_t nsigii_get_wheel_position(void) {
    return g_wheel_position;
}

void nsigii_update_all_channels_wheel_position(uint16_t position) {
    g_wheel_position = position % 360;
}

/* ============================================================================
 * LOGGING
 * ============================================================================ */

void nsigii_log_consensus_event(const char* event_type, const nsigii_packet_t* packet) {
    if (!event_type || !packet) return;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    printf("[LOG] %ld.%06ld | EVENT: %s | CH: %d | SEQ: %u | WHEEL: %d°\n",
           (long)tv.tv_sec, (long)tv.tv_usec,
           event_type,
           packet->header.channel_id,
           packet->header.sequence_token,
           packet->topology.wheel_position);
}

void nsigii_log_verification_failure(const nsigii_verification_result_t* result) {
    if (!result) return;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);
    
    printf("[LOG] %ld.%06ld | VERIFICATION FAILED: %s | Score: %.3f\n",
           (long)tv.tv_sec, (long)tv.tv_usec,
           result->status,
           result->consensus_score);
}

/* ============================================================================
 * UTILITY
 * ============================================================================ */

uint64_t nsigii_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void nsigii_generate_virtual_mac(int index, char* mac_buffer) {
    if (!mac_buffer) return;
    
    /* Generate virtual MAC address based on index */
    snprintf(mac_buffer, MAX_MAC_SIZE,
             "02:00:00:00:00:%02X", index & 0xFF);
}

const char* nsigii_get_loopback_for_channel(uint8_t channel_id) {
    switch (channel_id) {
        case CHANNEL_TRANSMITTER: return LOOPBACK_BASE "1";
        case CHANNEL_RECEIVER:    return LOOPBACK_BASE "2";
        case CHANNEL_VERIFIER:    return LOOPBACK_BASE "3";
        default:                  return LOOPBACK_BASE "1";
    }
}

/* ============================================================================
 * DNS RESOLVER
 * ============================================================================ */

int nsigii_dns_register_mac_channel(const char* mac_address, uint8_t channel_id) {
    if (!mac_address || !g_dns_table) return -1;
    
    uint8_t* id_copy = (uint8_t*)malloc(sizeof(uint8_t));
    if (!id_copy) return -1;
    
    *id_copy = channel_id;
    
    return nsigii_hash_insert(g_dns_table, mac_address, id_copy);
}

const char* nsigii_dns_resolve_channel(const char* mac_address) {
    if (!mac_address || !g_dns_table) return NULL;
    
    uint8_t* channel_id = (uint8_t*)nsigii_hash_lookup(g_dns_table, mac_address);
    if (!channel_id) return NULL;
    
    return nsigii_get_loopback_for_channel(*channel_id);
}

/* ============================================================================
 * TOMOGRAPHIC PAIR RESOLVING
 * ============================================================================ */

int nsigii_tomographic_pair_create(nsigii_tomographic_pair_t* pair) {
    if (!pair) return -1;
    
    memset(pair, 0, sizeof(nsigii_tomographic_pair_t));
    return 0;
}

double nsigii_tomographic_compute_correlation(const nsigii_packet_t* p1,
                                               const nsigii_packet_t* p2) {
    if (!p1 || !p2) return 0.0;
    
    /* Compute correlation based on sequence tokens and wheel positions */
    double seq_diff = (double)(p1->header.sequence_token - p2->header.sequence_token);
    double wheel_diff = (double)(p1->topology.wheel_position - p2->topology.wheel_position);
    
    /* Normalize */
    double seq_corr = 1.0 / (1.0 + fabs(seq_diff));
    double wheel_corr = 1.0 / (1.0 + fabs(wheel_diff) / 180.0);
    
    return (seq_corr + wheel_corr) / 2.0;
}

bool nsigii_tomographic_validate_pair(const nsigii_tomographic_pair_t* pair) {
    if (!pair) return false;
    
    /* Valid pair requires:
     * - Both packets present
     * - Correlation above threshold
     * - Different sequence tokens
     */
    if (!pair->pair[0] || !pair->pair[1]) return false;
    
    if (pair->pair[0]->header.sequence_token == pair->pair[1]->header.sequence_token) {
        return false;
    }
    
    return pair->correlation >= 0.5;
}

int nsigii_tomographic_pair_resolve(nsigii_tomographic_pair_t* pair) {
    if (!pair) return -1;
    
    pair->correlation = nsigii_tomographic_compute_correlation(pair->pair[0], pair->pair[1]);
    pair->resolved = nsigii_tomographic_validate_pair(pair);
    
    return pair->resolved ? 0 : -1;
}
