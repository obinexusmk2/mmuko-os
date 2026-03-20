/* ============================================================================
 * NSIGII RECEIVER MODULE - Channel 1 (2 * 2/3)
 * Triangle Second Receiver - Bipolar Order & Chaos
 * 
 * The Receiver is responsible for:
 * - Receiving packets from Transmitter via loopback
 * - Decoding packets from silicon representation
 * - Verifying hash integrity (SHA-256)
 * - Setting RWX READ permissions
 * - Bipartite order check (Order vs Chaos)
 * - Forwarding to Verifier
 * ============================================================================ */

#ifndef NSIGII_RECEIVER_H
#define NSIGII_RECEIVER_H

#include "../nsigii.h"
#include <stddef.h>
#include <stdint.h>
#include <netinet/in.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * RECEIVER STATE
 * ============================================================================ */

typedef enum {
    RECEIVER_STATE_IDLE,
    RECEIVER_STATE_RECEIVING,
    RECEIVER_STATE_DECODING,
    RECEIVER_STATE_VERIFYING_HASH,
    RECEIVER_STATE_FORWARDING,
    RECEIVER_STATE_ERROR
} receiver_state_t;

typedef struct {
    nsigii_channel_t  base;
    receiver_state_t  state;
    uint8_t           current_bipolar_state;  /* STATE_ORDER or STATE_CHAOS */
    nsigii_queue_t*   receive_queue;
    nsigii_queue_t*   forward_queue;
    int               listen_socket;
    int               forward_socket;
    struct sockaddr_in listen_addr;
    struct sockaddr_in forward_addr;
} nsigii_receiver_t;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================ */

/* Initialization and Cleanup */
int nsigii_receiver_create(nsigii_receiver_t** receiver);
void nsigii_receiver_destroy(nsigii_receiver_t* receiver);
int nsigii_receiver_init(nsigii_receiver_t* receiver);
void nsigii_receiver_cleanup(nsigii_receiver_t* receiver);

/* Core Decoding Functions */
int nsigii_receiver_decode_packet(nsigii_receiver_t* receiver,
                                   const uint8_t* encoded_data,
                                   size_t data_len,
                                   nsigii_packet_t* packet);

int nsigii_receiver_decode_header(const uint8_t* data,
                                   size_t* offset,
                                   nsigii_header_t* header);

int nsigii_receiver_decode_payload(const uint8_t* data,
                                    size_t* offset,
                                    nsigii_payload_t* payload);

int nsigii_receiver_decode_verification(const uint8_t* data,
                                         size_t* offset,
                                         nsigii_verification_t* verification);

int nsigii_receiver_decode_topology(const uint8_t* data,
                                     size_t* offset,
                                     nsigii_topology_t* topology);

/* Hash Verification */
bool nsigii_receiver_verify_hash(const nsigii_payload_t* payload);
int nsigii_receiver_compute_hash(const uint8_t* content,
                                  size_t len,
                                  uint8_t* hash);

/* Bipolar State Management */
uint8_t nsigii_receiver_determine_bipolar_state(uint32_t sequence_token);
void nsigii_receiver_update_bipolar_state(nsigii_receiver_t* receiver,
                                           uint32_t sequence_token);
const char* nsigii_receiver_bipolar_state_string(uint8_t state);

/* Reception Functions */
int nsigii_receiver_receive(nsigii_receiver_t* receiver,
                             nsigii_packet_t* packet);

int nsigii_receiver_listen(nsigii_receiver_t* receiver);
int nsigii_receiver_accept_connection(nsigii_receiver_t* receiver);
int nsigii_receiver_receive_data(nsigii_receiver_t* receiver,
                                  uint8_t* buffer,
                                  size_t* len);

/* Forwarding Functions */
int nsigii_receiver_forward_to_verifier(nsigii_receiver_t* receiver,
                                         const nsigii_packet_t* packet);

int nsigii_receiver_connect_to_verifier(nsigii_receiver_t* receiver);
int nsigii_receiver_forward_packet(nsigii_receiver_t* receiver,
                                    const nsigii_packet_t* packet);

/* Queue Management */
int nsigii_receiver_enqueue_received(nsigii_receiver_t* receiver,
                                      const nsigii_packet_t* packet);

int nsigii_receiver_enqueue_forward(nsigii_receiver_t* receiver,
                                     const nsigii_packet_t* packet);

nsigii_packet_t* nsigii_receiver_dequeue_received(nsigii_receiver_t* receiver);
nsigii_packet_t* nsigii_receiver_dequeue_forward(nsigii_receiver_t* receiver);

/* State Management */
receiver_state_t nsigii_receiver_get_state(const nsigii_receiver_t* receiver);
const char* nsigii_receiver_state_string(receiver_state_t state);

/* Thread Worker */
void* nsigii_receiver_worker(void* arg);
int nsigii_receiver_start_worker(nsigii_receiver_t* receiver);
int nsigii_receiver_stop_worker(nsigii_receiver_t* receiver);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/* Codec ratio calculation */
double nsigii_receiver_get_codec_ratio(void);

/* Listen address */
const char* nsigii_receiver_get_listen_address(void);
uint16_t nsigii_receiver_get_listen_port(void);

/* Forward address */
const char* nsigii_receiver_get_forward_address(void);
uint16_t nsigii_receiver_get_forward_port(void);

#ifdef __cplusplus
}
#endif

#endif /* NSIGII_RECEIVER_H */
