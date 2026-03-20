/* ============================================================================
 * NSIGII TRANSMITTER MODULE - Channel 0 (1 * 1/3)
 * First Channel Codec - "Encoding Suffering into Silicon"
 * 
 * The Transmitter is responsible for:
 * - Encoding raw messages into NSIGII packet format
 * - Computing message hashes (SHA-256)
 * - Setting RWX WRITE permissions
 * - Tagging with human rights identifiers
 * - Transmitting to Receiver via loopback
 * ============================================================================ */

#ifndef NSIGII_TRANSMITTER_H
#define NSIGII_TRANSMITTER_H

#include "../nsigii.h"
#include "../serialization.h"
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netinet/in.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * TRANSMITTER STATE
 * ============================================================================ */

typedef enum {
    TRANSMITTER_STATE_IDLE,
    TRANSMITTER_STATE_ENCODING,
    TRANSMITTER_STATE_TRANSMITTING,
    TRANSMITTER_STATE_ERROR
} transmitter_state_t;

typedef struct {
    nsigii_channel_t    base;
    transmitter_state_t state;
    uint32_t            sequence_counter;
    nsigii_queue_t*     message_queue;
    int                 target_socket;
    struct sockaddr_in  target_addr;
} nsigii_transmitter_t;

/* ============================================================================
 * FUNCTION PROTOTYPES
 * ============================================================================ */

/* Initialization and Cleanup */
int nsigii_transmitter_create(nsigii_transmitter_t** transmitter);
void nsigii_transmitter_destroy(nsigii_transmitter_t* transmitter);
int nsigii_transmitter_init(nsigii_transmitter_t* transmitter);
void nsigii_transmitter_cleanup(nsigii_transmitter_t* transmitter);

/* Core Encoding Functions */
int nsigii_transmitter_encode_message(nsigii_transmitter_t* transmitter,
                                       const uint8_t* raw_content,
                                       size_t content_len,
                                       nsigii_packet_t* packet);

int nsigii_transmitter_encode_header(nsigii_transmitter_t* transmitter,
                                      nsigii_header_t* header);

int nsigii_transmitter_encode_payload(const uint8_t* content,
                                       size_t content_len,
                                       nsigii_payload_t* payload);

int nsigii_transmitter_encode_verification(nsigii_verification_t* verification);

int nsigii_transmitter_encode_topology(nsigii_topology_t* topology);

/* Transmission Functions */
int nsigii_transmitter_transmit(nsigii_transmitter_t* transmitter,
                                 const nsigii_packet_t* packet);

int nsigii_transmitter_send_packet(nsigii_transmitter_t* transmitter,
                                    const nsigii_packet_t* packet);

int nsigii_transmitter_send_raw(nsigii_transmitter_t* transmitter,
                                 const uint8_t* data,
                                 size_t len);

/* Queue Management */
int nsigii_transmitter_queue_message(nsigii_transmitter_t* transmitter,
                                      const uint8_t* message,
                                      size_t len);

int nsigii_transmitter_process_queue(nsigii_transmitter_t* transmitter);

/* State Management */
transmitter_state_t nsigii_transmitter_get_state(const nsigii_transmitter_t* transmitter);
const char* nsigii_transmitter_state_string(transmitter_state_t state);

/* Thread Worker */
void* nsigii_transmitter_worker(void* arg);
int nsigii_transmitter_start_worker(nsigii_transmitter_t* transmitter);
int nsigii_transmitter_stop_worker(nsigii_transmitter_t* transmitter);

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

/* Sequence token generation */
uint32_t nsigii_transmitter_next_sequence(nsigii_transmitter_t* transmitter);

/* Codec ratio calculation */
double nsigii_transmitter_get_codec_ratio(void);

/* Target address resolution */
const char* nsigii_transmitter_get_target_address(void);
uint16_t nsigii_transmitter_get_target_port(void);

#ifdef __cplusplus
}
#endif

#endif /* NSIGII_TRANSMITTER_H */
