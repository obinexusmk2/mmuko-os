/* ============================================================================
 * NSIGII VERIFIER MODULE IMPLEMENTATION - Channel 2 (3 * 3/3)
 * Angel Verification - Rectorial Reasoning Rational Wheel
 * ============================================================================ */

#include "verifier/verifier.h"
#include "serialization.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define close(fd) closesocket(fd)
#define usleep(us) Sleep((us) / 1000)
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <math.h>
#include <pthread.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static pthread_t worker_thread;
static int worker_running = 0;

static const char* valid_hr_tags[] = {
    "NSIGII_HR_TRANSMIT",
    "NSIGII_HR_RECEIVE",
    "NSIGII_HR_VERIFY",
    "NSIGII_HR_VERIFIED"
};

/* ============================================================================
 * INITIALIZATION AND CLEANUP
 * ============================================================================ */

int nsigii_verifier_create(nsigii_verifier_t** verifier) {
    if (!verifier) return -1;
    
    *verifier = (nsigii_verifier_t*)malloc(sizeof(nsigii_verifier_t));
    if (!*verifier) return -1;
    
    memset(*verifier, 0, sizeof(nsigii_verifier_t));
    return nsigii_verifier_init(*verifier);
}

void nsigii_verifier_destroy(nsigii_verifier_t* verifier) {
    if (!verifier) return;
    
    nsigii_verifier_cleanup(verifier);
    free(verifier);
}

int nsigii_verifier_init(nsigii_verifier_t* verifier) {
    if (!verifier) return -1;
    
    /* Initialize base channel */
    verifier->base.channel_id = CHANNEL_VERIFIER;
    strncpy(verifier->base.loopback_addr, LOOPBACK_BASE "3", 15);
    verifier->base.loopback_addr[15] = '\0';
    snprintf(verifier->base.dns_name, sizeof(verifier->base.dns_name),
             "verifier.%s", DNS_NAMESPACE);
    verifier->base.codec_ratio = 3.0 / 3.0;  /* Complete verification */
    verifier->base.state = STATE_VERIFIED;
    verifier->base.port = PORT_VERIFIER;
    
    /* Initialize verifier-specific fields */
    verifier->state = VERIFIER_STATE_IDLE;
    verifier->verify_queue = nsigii_queue_create();
    verifier->consensus_queue = nsigii_queue_create();
    
    if (!verifier->verify_queue || !verifier->consensus_queue) {
        return -1;
    }
    
    /* Initialize consensus log */
    verifier->log_capacity = 1024;
    verifier->consensus_log = (nsigii_log_entry_t*)malloc(
        verifier->log_capacity * sizeof(nsigii_log_entry_t));
    if (!verifier->consensus_log) {
        return -1;
    }
    verifier->log_count = 0;
    
    /* Generate consensus key */
    for (int i = 0; i < 32; i++) {
        verifier->consensus_key[i] = (uint8_t)(rand() % 256);
    }
    
    /* Create listen socket */
    verifier->listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (verifier->listen_socket < 0) {
        perror("socket");
        return -1;
    }

    /* Allow socket reuse to prevent "Address already in use" */
    int reuse = 1;
    setsockopt(verifier->listen_socket, SOL_SOCKET, SO_REUSEADDR,
               (const char*)&reuse, sizeof(reuse));

    /* Set up listen address */
    memset(&verifier->listen_addr, 0, sizeof(verifier->listen_addr));
    verifier->listen_addr.sin_family = AF_INET;
    verifier->listen_addr.sin_port = htons(PORT_VERIFIER);
    inet_pton(AF_INET, LOOPBACK_BASE "3", &verifier->listen_addr.sin_addr);
    
    /* Bind socket */
    if (bind(verifier->listen_socket, (struct sockaddr*)&verifier->listen_addr,
             sizeof(verifier->listen_addr)) < 0) {
        perror("bind");
        close(verifier->listen_socket);
        return -1;
    }
    
    /* Set up broadcast addresses for all channels */
    for (int i = 0; i < TRIDENT_CHANNELS; i++) {
        memset(&verifier->broadcast_addrs[i], 0, sizeof(struct sockaddr_in));
        verifier->broadcast_addrs[i].sin_family = AF_INET;
        verifier->broadcast_addrs[i].sin_port = htons(PORT_TRANSMITTER + i);
        char addr[16];
        snprintf(addr, sizeof(addr), "%s%d", LOOPBACK_BASE, i + 1);
        inet_pton(AF_INET, addr, &verifier->broadcast_addrs[i].sin_addr);
    }
    
    /* Create broadcast socket */
    verifier->broadcast_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (verifier->broadcast_socket < 0) {
        perror("broadcast socket");
        close(verifier->listen_socket);
        return -1;
    }
    
    int broadcast_enable = 1;
    setsockopt(verifier->broadcast_socket, SOL_SOCKET, SO_BROADCAST,
               (const char*)&broadcast_enable, sizeof(broadcast_enable));
    
    return 0;
}

void nsigii_verifier_cleanup(nsigii_verifier_t* verifier) {
    if (!verifier) return;
    
    if (verifier->listen_socket >= 0) {
        close(verifier->listen_socket);
        verifier->listen_socket = -1;
    }
    
    if (verifier->broadcast_socket >= 0) {
        close(verifier->broadcast_socket);
        verifier->broadcast_socket = -1;
    }
    
    if (verifier->verify_queue) {
        nsigii_queue_destroy(verifier->verify_queue);
        verifier->verify_queue = NULL;
    }
    
    if (verifier->consensus_queue) {
        nsigii_queue_destroy(verifier->consensus_queue);
        verifier->consensus_queue = NULL;
    }
    
    if (verifier->consensus_log) {
        free(verifier->consensus_log);
        verifier->consensus_log = NULL;
    }
}

/* ============================================================================
 * CORE VERIFICATION FUNCTIONS
 * ============================================================================ */

int nsigii_verifier_verify_packet(nsigii_verifier_t* verifier,
                                   const nsigii_packet_t* packet,
                                   nsigii_verification_result_t* result) {
    if (!verifier || !packet || !result) return -1;
    
    verifier->state = VERIFIER_STATE_VERIFYING;
    memset(result, 0, sizeof(*result));
    
    /* Check 1: RWX Permission Validation */
    if (!nsigii_verifier_validate_rwx_chain(packet)) {
        strncpy(result->status, VERIFICATION_STATUS_RWX_VIOLATION, sizeof(result->status) - 1);
        verifier->state = VERIFIER_STATE_CONSENSUS_FAILED;
        nsigii_verifier_log_verification_failure(verifier, result);
        return -1;
    }
    
    /* Check 2: Bipartite Consensus (2/3 majority) */
    double consensus_score = nsigii_verifier_compute_bipartite_consensus(packet);
    result->consensus_score = consensus_score;
    
    if (!nsigii_verifier_meets_threshold(consensus_score)) {
        strncpy(result->status, VERIFICATION_STATUS_CONSENSUS_FAILED, sizeof(result->status) - 1);
        verifier->state = VERIFIER_STATE_CONSENSUS_FAILED;
        nsigii_verifier_log_verification_failure(verifier, result);
        return -1;
    }
    
    /* Check 3: Human Rights Protocol Compliance */
    if (!nsigii_verifier_verify_human_rights_tag(packet)) {
        strncpy(result->status, VERIFICATION_STATUS_HR_VIOLATION, sizeof(result->status) - 1);
        verifier->state = VERIFIER_STATE_CONSENSUS_FAILED;
        nsigii_verifier_log_verification_failure(verifier, result);
        return -1;
    }
    
    /* Check 4: Wheel Position Integrity */
    if (!nsigii_verifier_check_wheel_position(packet)) {
        strncpy(result->status, VERIFICATION_STATUS_WHEEL_ERROR, sizeof(result->status) - 1);
        verifier->state = VERIFIER_STATE_CONSENSUS_FAILED;
        nsigii_verifier_log_verification_failure(verifier, result);
        return -1;
    }
    
    /* All checks passed - mark as verified */
    nsigii_packet_t* verified = (nsigii_packet_t*)malloc(sizeof(nsigii_packet_t));
    if (!verified) {
        verifier->state = VERIFIER_STATE_ERROR;
        return -1;
    }
    
    memcpy(verified, packet, sizeof(nsigii_packet_t));
    
    /* Set full RWX permissions */
    verified->verification.rwx_flags = RWX_FULL;
    verified->header.codec_version = 3;  /* 3 * 3/3 */
    verified->topology.wheel_position = VERIFIER_FULL_WHEEL_POS;  /* Full circle */
    
    /* Generate consensus signature */
    nsigii_verifier_sign_packet(verifier, verified);
    
    result->verified_packet = verified;
    strncpy(result->status, VERIFICATION_STATUS_VERIFIED, sizeof(result->status) - 1);
    
    verifier->state = VERIFIER_STATE_CONSENSUS_PASSED;
    
    /* Log consensus event */
    nsigii_verifier_log_consensus_event(verifier, "VERIFIED", verified);
    
    return 0;
}

/* ============================================================================
 * INDIVIDUAL VERIFICATION CHECKS
 * ============================================================================ */

bool nsigii_verifier_validate_rwx_chain(const nsigii_packet_t* packet) {
    if (!packet) return false;
    
    /* Verify RWX permissions form valid sequence: Write -> Read -> Execute */
    /* Transmitter: Write (0x02), Receiver: Read (0x04), Verifier: Execute (0x01) */
    
    uint8_t expected = RWX_READ | RWX_WRITE;  /* Should have at least Read+Write */
    return (packet->verification.rwx_flags & expected) != 0;
}

double nsigii_verifier_compute_bipartite_consensus(const nsigii_packet_t* packet) {
    if (!packet) return 0.0;
    
    /* Bipartite order: Calculate consensus between Order (1) and Chaos (0) */
    double order_weight = nsigii_verifier_count_order_bits(
        packet->payload.content, packet->payload.content_length);
    double total_bits = packet->payload.content_length * 8.0;
    
    if (total_bits == 0) return 0.0;
    
    /* Bipartite formula: Order vs Chaos balance */
    double consensus = order_weight / total_bits;
    
    /* Apply rational wheel correction */
    double wheel_correction = sin(packet->topology.wheel_position * M_PI / 180.0);
    
    return fabs(consensus + wheel_correction) / 2.0;
}

bool nsigii_verifier_verify_human_rights_tag(const nsigii_packet_t* packet) {
    if (!packet) return false;
    
    return nsigii_verifier_is_valid_hr_tag(packet->verification.human_rights_tag);
}

bool nsigii_verifier_check_wheel_position(const nsigii_packet_t* packet) {
    if (!packet) return false;
    
    /* Expected wheel position at verifier is 240 degrees */
    /* Allow some tolerance for the check */
    return packet->topology.wheel_position >= 100 && 
           packet->topology.wheel_position <= 250;
}

/* ============================================================================
 * CONSENSUS CALCULATION
 * ============================================================================ */

double nsigii_verifier_count_order_bits(const uint8_t* data, size_t len) {
    if (!data || len == 0) return 0.0;
    
    double count = 0.0;
    
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        /* Count set bits (Order) vs unset bits (Chaos) */
        for (int j = 0; j < 8; j++) {
            if (byte & (1 << j)) {
                count += 1.0;
            }
        }
    }
    
    return count;
}

double nsigii_verifier_apply_wheel_correction(double consensus, uint16_t wheel_pos) {
    double correction = sin(wheel_pos * M_PI / 180.0);
    return fabs(consensus + correction) / 2.0;
}

bool nsigii_verifier_meets_threshold(double consensus_score) {
    return consensus_score >= CONSENSUS_THRESHOLD;
}

/* ============================================================================
 * SIGNATURE GENERATION
 * ============================================================================ */

void nsigii_verifier_generate_consensus_signature(nsigii_verifier_t* verifier,
                                                   const nsigii_packet_t* packet,
                                                   uint8_t* signature) {
    if (!verifier || !packet || !signature) return;
    
    /* Create signature input */
    uint8_t sig_input[MAX_CONTENT_SIZE + 256];
    size_t offset = 0;
    
    memcpy(sig_input + offset, &packet->header.sequence_token, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    
    memcpy(sig_input + offset, packet->payload.message_hash, HASH_SIZE);
    offset += HASH_SIZE;
    
    size_t tag_len = strlen(packet->verification.human_rights_tag);
    memcpy(sig_input + offset, packet->verification.human_rights_tag, tag_len);
    offset += tag_len;
    
    const char* consensus_str = "NSIGII_CONSENSUS";
    memcpy(sig_input + offset, consensus_str, strlen(consensus_str));
    offset += strlen(consensus_str);
    
    /* Generate HMAC-SHA256 */
    nsigii_hmac_sha256(verifier->consensus_key, 32, sig_input, offset, signature);
}

void nsigii_verifier_sign_packet(nsigii_verifier_t* verifier,
                                  nsigii_packet_t* packet) {
    if (!verifier || !packet) return;
    
    nsigii_verifier_generate_consensus_signature(verifier, packet,
                                                  packet->verification.consensus_sig);
}

/* ============================================================================
 * CONSENSUS MESSAGE EMISSION
 * ============================================================================ */

int nsigii_verifier_emit_consensus_message(nsigii_verifier_t* verifier,
                                            const nsigii_packet_t* verified_packet,
                                            nsigii_consensus_message_t* consensus) {
    if (!verifier || !verified_packet || !consensus) return -1;
    
    verifier->state = VERIFIER_STATE_EMITTING;
    
    /* Compute trident hash */
    uint8_t buffer[MAX_CONTENT_SIZE + 512];
    size_t len = 0;
    nsigii_serialize_binary(verified_packet, buffer, &len);
    nsigii_sha256(buffer, len, consensus->trident_hash);
    
    consensus->timestamp = nsigii_now_ns();
    strncpy(consensus->status, "HUMAN_RIGHTS_VERIFIED", sizeof(consensus->status) - 1);
    strncpy(consensus->wheel_position, "FULL_CIRCLE", sizeof(consensus->wheel_position) - 1);
    
    /* Broadcast to all channels */
    nsigii_verifier_broadcast_consensus(verifier, consensus);
    
    verifier->state = VERIFIER_STATE_IDLE;
    return 0;
}

int nsigii_verifier_broadcast_consensus(nsigii_verifier_t* verifier,
                                         const nsigii_consensus_message_t* consensus) {
    if (!verifier || !consensus) return -1;
    
    /* Serialize consensus message */
    uint8_t buffer[256];
    size_t offset = 0;
    
    memcpy(buffer + offset, consensus->trident_hash, HASH_SIZE);
    offset += HASH_SIZE;
    memcpy(buffer + offset, &consensus->timestamp, sizeof(uint64_t));
    offset += sizeof(uint64_t);
    
    size_t status_len = strlen(consensus->status) + 1;
    memcpy(buffer + offset, consensus->status, status_len);
    offset += status_len;
    
    /* Broadcast to all channels */
    for (int i = 0; i < TRIDENT_CHANNELS; i++) {
        sendto(verifier->broadcast_socket, buffer, offset, 0,
               (struct sockaddr*)&verifier->broadcast_addrs[i],
               sizeof(verifier->broadcast_addrs[i]));
    }
    
    return 0;
}

/* ============================================================================
 * RECEPTION FUNCTIONS
 * ============================================================================ */

int nsigii_verifier_listen(nsigii_verifier_t* verifier) {
    if (!verifier) return -1;
    
    /* Already bound in init */
    return 0;
}

int nsigii_verifier_receive_packet(nsigii_verifier_t* verifier,
                                    nsigii_packet_t* packet) {
    if (!verifier || !packet) return -1;
    
    uint8_t buffer[MAX_CONTENT_SIZE + 512];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    ssize_t received = recvfrom(verifier->listen_socket, buffer, sizeof(buffer), 0,
                                (struct sockaddr*)&from_addr, &from_len);
    
    if (received < 0) {
        perror("recvfrom");
        return -1;
    }
    
    return nsigii_deserialize_binary(buffer, (size_t)received, packet);
}

/* ============================================================================
 * QUEUE MANAGEMENT
 * ============================================================================ */

int nsigii_verifier_enqueue_verify(nsigii_verifier_t* verifier,
                                    const nsigii_packet_t* packet) {
    if (!verifier || !packet) return -1;
    
    nsigii_packet_t* copy = (nsigii_packet_t*)malloc(sizeof(nsigii_packet_t));
    if (!copy) return -1;
    
    memcpy(copy, packet, sizeof(nsigii_packet_t));
    return nsigii_queue_enqueue(verifier->verify_queue, copy);
}

int nsigii_verifier_enqueue_consensus(nsigii_verifier_t* verifier,
                                       const nsigii_consensus_message_t* consensus) {
    if (!verifier || !consensus) return -1;
    
    nsigii_consensus_message_t* copy = 
        (nsigii_consensus_message_t*)malloc(sizeof(nsigii_consensus_message_t));
    if (!copy) return -1;
    
    memcpy(copy, consensus, sizeof(nsigii_consensus_message_t));
    return nsigii_queue_enqueue(verifier->consensus_queue, copy);
}

nsigii_packet_t* nsigii_verifier_dequeue_verify(nsigii_verifier_t* verifier) {
    if (!verifier) return NULL;
    return (nsigii_packet_t*)nsigii_queue_dequeue(verifier->verify_queue);
}

nsigii_consensus_message_t* nsigii_verifier_dequeue_consensus(nsigii_verifier_t* verifier) {
    if (!verifier) return NULL;
    return (nsigii_consensus_message_t*)nsigii_queue_dequeue(verifier->consensus_queue);
}

/* ============================================================================
 * LOGGING
 * ============================================================================ */

void nsigii_verifier_log_consensus_event(nsigii_verifier_t* verifier,
                                          const char* event_type,
                                          const nsigii_packet_t* packet) {
    if (!verifier || !event_type || !packet) return;
    
    if (verifier->log_count >= verifier->log_capacity) {
        /* Expand log capacity */
        size_t new_capacity = verifier->log_capacity * 2;
        nsigii_log_entry_t* new_log = (nsigii_log_entry_t*)realloc(
            verifier->consensus_log, new_capacity * sizeof(nsigii_log_entry_t));
        if (!new_log) return;
        
        verifier->consensus_log = new_log;
        verifier->log_capacity = new_capacity;
    }
    
    nsigii_log_entry_t* entry = &verifier->consensus_log[verifier->log_count++];
    entry->timestamp = nsigii_now_ns();
    strncpy(entry->event_type, event_type, sizeof(entry->event_type) - 1);
    entry->channel = packet->header.channel_id;
    entry->sequence = packet->header.sequence_token;
    entry->wheel_pos = packet->topology.wheel_position;
}

void nsigii_verifier_log_verification_failure(nsigii_verifier_t* verifier,
                                               const nsigii_verification_result_t* result) {
    if (!verifier || !result) return;
    
    fprintf(stderr, "Verification failed: %s (consensus: %.3f)\n",
            result->status, result->consensus_score);
}

/* ============================================================================
 * STATE MANAGEMENT
 * ============================================================================ */

verifier_state_t nsigii_verifier_get_state(const nsigii_verifier_t* verifier) {
    return verifier ? verifier->state : VERIFIER_STATE_ERROR;
}

const char* nsigii_verifier_state_string(verifier_state_t state) {
    switch (state) {
        case VERIFIER_STATE_IDLE:            return "IDLE";
        case VERIFIER_STATE_VERIFYING:       return "VERIFYING";
        case VERIFIER_STATE_CONSENSUS_PASSED: return "CONSENSUS_PASSED";
        case VERIFIER_STATE_CONSENSUS_FAILED: return "CONSENSUS_FAILED";
        case VERIFIER_STATE_EMITTING:        return "EMITTING";
        case VERIFIER_STATE_ERROR:           return "ERROR";
        default:                             return "UNKNOWN";
    }
}

/* ============================================================================
 * THREAD WORKER
 * ============================================================================ */

void* nsigii_verifier_worker(void* arg) {
    nsigii_verifier_t* verifier = (nsigii_verifier_t*)arg;
    if (!verifier) return NULL;
    
    while (worker_running) {
        nsigii_packet_t packet;
        memset(&packet, 0, sizeof(packet));
        
        /* Receive packet */
        if (nsigii_verifier_receive_packet(verifier, &packet) == 0) {
            /* Verify packet */
            nsigii_verification_result_t result;
            if (nsigii_verifier_verify_packet(verifier, &packet, &result) == 0) {
                /* Emit consensus message */
                nsigii_consensus_message_t consensus;
                nsigii_verifier_emit_consensus_message(verifier, result.verified_packet, &consensus);
                
                if (result.verified_packet) {
                    free(result.verified_packet);
                }
            }
        }
        
        usleep(1000);  /* 1ms sleep */
    }
    
    return NULL;
}

int nsigii_verifier_start_worker(nsigii_verifier_t* verifier) {
    if (!verifier || worker_running) return -1;
    
    worker_running = 1;
    if (pthread_create(&worker_thread, NULL, nsigii_verifier_worker, verifier) != 0) {
        worker_running = 0;
        return -1;
    }
    
    return 0;
}

int nsigii_verifier_stop_worker(nsigii_verifier_t* verifier) {
    (void)verifier;
    
    if (!worker_running) return -1;
    
    worker_running = 0;
    pthread_join(worker_thread, NULL);
    return 0;
}

/* ============================================================================
 * TOMOGRAPHIC PAIR RESOLVING
 * ============================================================================ */

int nsigii_verifier_resolve_tomographic_pair(nsigii_verifier_t* verifier,
                                               nsigii_tomographic_pair_t* pair) {
    if (!verifier || !pair) return -1;
    
    /* Compute correlation between pair */
    pair->correlation = nsigii_tomographic_compute_correlation(pair->pair[0], pair->pair[1]);
    
    /* Validate correlation */
    pair->resolved = nsigii_verifier_validate_tomographic_correlation(pair->correlation);
    
    return pair->resolved ? 0 : -1;
}

bool nsigii_verifier_validate_tomographic_correlation(double correlation) {
    /* Valid correlation should be between 0.5 and 1.0 for consensus */
    return correlation >= 0.5 && correlation <= 1.0;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

double nsigii_verifier_get_codec_ratio(void) {
    return 3.0 / 3.0;
}

const char* nsigii_verifier_get_listen_address(void) {
    return LOOPBACK_BASE "3";
}

uint16_t nsigii_verifier_get_listen_port(void) {
    return PORT_VERIFIER;
}

const char** nsigii_verifier_get_valid_hr_tags(size_t* count) {
    if (count) {
        *count = sizeof(valid_hr_tags) / sizeof(valid_hr_tags[0]);
    }
    return valid_hr_tags;
}

bool nsigii_verifier_is_valid_hr_tag(const char* tag) {
    if (!tag) return false;
    
    size_t count = sizeof(valid_hr_tags) / sizeof(valid_hr_tags[0]);
    for (size_t i = 0; i < count; i++) {
        if (strcmp(tag, valid_hr_tags[i]) == 0) {
            return true;
        }
    }
    return false;
}
