/* ============================================================================
 * NSIGII VERIFIER MODULE - Channel 2 (3 * 3/3)
 * Angel Verification - Rectorial Reasoning Rational Wheel
 * 
 * The Verifier is responsible for:
 * - RWX Permission Validation (Write -> Read -> Execute chain)
 * - Bipartite Consensus calculation (2/3 majority)
 * - Human Rights Protocol Compliance verification
 * - Wheel Position Integrity checks
 * - Consensus signature generation (HMAC-SHA256)
 * - Emitting consensus messages
 * ============================================================================ */

#ifndef NSIGII_VERIFIER_H
#define NSIGII_VERIFIER_H

#include "../nsigii.h"
#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * VERIFICATION CONSTANTS
 * ============================================================================ */

#define VERIFIER_EXPECTED_WHEEL_POS 240  /* Expected position at verification */
#define VERIFIER_FULL_WHEEL_POS     360  /* Full circle completion */

/* ============================================================================
 * VERIFIER STATE
 * ============================================================================ */

typedef enum {
    VERIFIER_STATE_IDLE,
    VERIFIER_STATE_VERIFYING,
    VERIFIER_STATE_CONSENSUS_PASSED,
    VERIFIER_STATE_CONSENSUS_FAILED,
    VERIFIER_STATE_EMITTING,
    VERIFIER_STATE_ERROR
} verifier_state_t;

typedef struct {
    nsigii_channel_t      base;
    verifier_state_t      state;
    nsigii_queue_t*       verify_queue;
    nsigii_queue_t*       consensus_queue;
    nsigii_log_entry_t*   consensus_log;
    size_t                log_count;
    size_t                log_capacity;
    int                   listen_socket;
    int                   broadcast_socket;
    struct sockaddr_in    listen_addr;
    struct sockaddr_in    broadcast_addrs[TRIDENT_CHANNELS];
    uint8_t               consensus_key[32];  /* HMAC key */
} nsigii_verifier_t;

/* ============================================================================
 * VERIFICATION RESULT CODES
 * ============================================================================ */

#define VERIFICATION_STATUS_VERIFIED        "VERIFIED"
#define VERIFICATION_STATUS_RWX_VIOLATION   "RWX_VIOLATION"
#define VERIFICATION_STATUS_CONSENSUS_FAILED "CONSENSUS_FAILED"
#define VERIFICATION_STATUS_HR_VIOLATION    "HR_VIOLATION"
#define VERIFICATION_STATUS_WHEEL_ERROR     "WHEEL_POSITION_ERROR"

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================ */

/* Initialization and Cleanup */
int nsigii_verifier_create(nsigii_verifier_t** verifier);
void nsigii_verifier_destroy(nsigii_verifier_t* verifier);
int nsigii_verifier_init(nsigii_verifier_t* verifier);
void nsigii_verifier_cleanup(nsigii_verifier_t* verifier);

/* Core Verification Functions */
int nsigii_verifier_verify_packet(nsigii_verifier_t* verifier,
                                   const nsigii_packet_t* packet,
                                   nsigii_verification_result_t* result);

/* Individual Verification Checks */
bool nsigii_verifier_validate_rwx_chain(const nsigii_packet_t* packet);
double nsigii_verifier_compute_bipartite_consensus(const nsigii_packet_t* packet);
bool nsigii_verifier_verify_human_rights_tag(const nsigii_packet_t* packet);
bool nsigii_verifier_check_wheel_position(const nsigii_packet_t* packet);

/* Consensus Calculation */
double nsigii_verifier_count_order_bits(const uint8_t* data, size_t len);
double nsigii_verifier_apply_wheel_correction(double consensus, uint16_t wheel_pos);
bool nsigii_verifier_meets_threshold(double consensus_score);

/* Signature Generation */
void nsigii_verifier_generate_consensus_signature(nsigii_verifier_t* verifier,
                                                   const nsigii_packet_t* packet,
                                                   uint8_t* signature);

void nsigii_verifier_sign_packet(nsigii_verifier_t* verifier,
                                  nsigii_packet_t* packet);

/* Consensus Message Emission */
int nsigii_verifier_emit_consensus_message(nsigii_verifier_t* verifier,
                                            const nsigii_packet_t* verified_packet,
                                            nsigii_consensus_message_t* consensus);

int nsigii_verifier_broadcast_consensus(nsigii_verifier_t* verifier,
                                         const nsigii_consensus_message_t* consensus);

/* Reception Functions */
int nsigii_verifier_listen(nsigii_verifier_t* verifier);
int nsigii_verifier_accept_and_verify(nsigii_verifier_t* verifier);
int nsigii_verifier_receive_packet(nsigii_verifier_t* verifier,
                                    nsigii_packet_t* packet);

/* Queue Management */
int nsigii_verifier_enqueue_verify(nsigii_verifier_t* verifier,
                                    const nsigii_packet_t* packet);

int nsigii_verifier_enqueue_consensus(nsigii_verifier_t* verifier,
                                       const nsigii_consensus_message_t* consensus);

nsigii_packet_t* nsigii_verifier_dequeue_verify(nsigii_verifier_t* verifier);
nsigii_consensus_message_t* nsigii_verifier_dequeue_consensus(nsigii_verifier_t* verifier);

/* Logging */
void nsigii_verifier_log_consensus_event(nsigii_verifier_t* verifier,
                                          const char* event_type,
                                          const nsigii_packet_t* packet);

void nsigii_verifier_log_verification_failure(nsigii_verifier_t* verifier,
                                               const nsigii_verification_result_t* result);

/* State Management */
verifier_state_t nsigii_verifier_get_state(const nsigii_verifier_t* verifier);
const char* nsigii_verifier_state_string(verifier_state_t state);

/* Thread Worker */
void* nsigii_verifier_worker(void* arg);
int nsigii_verifier_start_worker(nsigii_verifier_t* verifier);
int nsigii_verifier_stop_worker(nsigii_verifier_t* verifier);

/* ============================================================================
 * TOMOGRAPHIC PAIR RESOLVING (Verifier Integration)
 * ============================================================================ */

/* Tomographic pair verification */
int nsigii_verifier_resolve_tomographic_pair(nsigii_verifier_t* verifier,
                                               nsigii_tomographic_pair_t* pair);

bool nsigii_verifier_validate_tomographic_correlation(double correlation);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/* Codec ratio */
double nsigii_verifier_get_codec_ratio(void);

/* Listen address */
const char* nsigii_verifier_get_listen_address(void);
uint16_t nsigii_verifier_get_listen_port(void);

/* Valid human rights tags */
const char** nsigii_verifier_get_valid_hr_tags(size_t* count);
bool nsigii_verifier_is_valid_hr_tag(const char* tag);

#ifdef __cplusplus
}
#endif

#endif /* NSIGII_VERIFIER_H */
