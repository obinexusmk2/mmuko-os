/* ============================================================================
 * NSIGII TRANSMITTER MODULE IMPLEMENTATION - Channel 0 (1 * 1/3)
 * First Channel Codec - "Encoding Suffering into Silicon"
 * ============================================================================ */

#include "transmitter/transmitter.h"
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
#include <pthread.h>
#include <openssl/sha.h>
#include <time.h>

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static pthread_mutex_t sequence_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint32_t global_sequence = 0;
static pthread_t worker_thread;
static int worker_running = 0;

/* ============================================================================
 * INITIALIZATION AND CLEANUP
 * ============================================================================ */

int nsigii_transmitter_create(nsigii_transmitter_t** transmitter) {
    if (!transmitter) return -1;
    
    *transmitter = (nsigii_transmitter_t*)malloc(sizeof(nsigii_transmitter_t));
    if (!*transmitter) return -1;
    
    memset(*transmitter, 0, sizeof(nsigii_transmitter_t));
    return nsigii_transmitter_init(*transmitter);
}

void nsigii_transmitter_destroy(nsigii_transmitter_t* transmitter) {
    if (!transmitter) return;
    
    nsigii_transmitter_cleanup(transmitter);
    free(transmitter);
}

int nsigii_transmitter_init(nsigii_transmitter_t* transmitter) {
    if (!transmitter) return -1;
    
    /* Initialize base channel */
    transmitter->base.channel_id = CHANNEL_TRANSMITTER;
    strncpy(transmitter->base.loopback_addr, LOOPBACK_BASE "1", 15);
    transmitter->base.loopback_addr[15] = '\0';
    snprintf(transmitter->base.dns_name, sizeof(transmitter->base.dns_name),
             "transmitter.%s", DNS_NAMESPACE);
    transmitter->base.codec_ratio = 1.0 / 3.0;
    transmitter->base.state = STATE_ORDER;
    transmitter->base.port = PORT_TRANSMITTER;
    
    /* Initialize transmitter-specific fields */
    transmitter->state = TRANSMITTER_STATE_IDLE;
    transmitter->sequence_counter = 0;
    transmitter->message_queue = nsigii_queue_create();
    if (!transmitter->message_queue) return -1;
    
    /* Create socket */
    transmitter->target_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (transmitter->target_socket < 0) {
        perror("socket");
        return -1;
    }
    
    /* Set up target address (Receiver) */
    memset(&transmitter->target_addr, 0, sizeof(transmitter->target_addr));
    transmitter->target_addr.sin_family = AF_INET;
    transmitter->target_addr.sin_port = htons(PORT_RECEIVER);
    inet_pton(AF_INET, LOOPBACK_BASE "2", &transmitter->target_addr.sin_addr);
    
    return 0;
}

void nsigii_transmitter_cleanup(nsigii_transmitter_t* transmitter) {
    if (!transmitter) return;
    
    if (transmitter->target_socket >= 0) {
        close(transmitter->target_socket);
        transmitter->target_socket = -1;
    }
    
    if (transmitter->message_queue) {
        nsigii_queue_destroy(transmitter->message_queue);
        transmitter->message_queue = NULL;
    }
}

/* ============================================================================
 * CORE ENCODING FUNCTIONS
 * ============================================================================ */

int nsigii_transmitter_encode_message(nsigii_transmitter_t* transmitter,
                                       const uint8_t* raw_content,
                                       size_t content_len,
                                       nsigii_packet_t* packet) {
    if (!transmitter || !raw_content || !packet || content_len > MAX_CONTENT_SIZE) {
        return -1;
    }
    
    transmitter->state = TRANSMITTER_STATE_ENCODING;
    
    /* Encode header */
    nsigii_transmitter_encode_header(transmitter, &packet->header);
    
    /* Encode payload */
    nsigii_transmitter_encode_payload(raw_content, content_len, &packet->payload);
    
    /* Encode verification */
    nsigii_transmitter_encode_verification(&packet->verification);
    
    /* Encode topology */
    nsigii_transmitter_encode_topology(&packet->topology);
    
    transmitter->state = TRANSMITTER_STATE_IDLE;
    return 0;
}

int nsigii_transmitter_encode_header(nsigii_transmitter_t* transmitter,
                                      nsigii_header_t* header) {
    if (!transmitter || !header) return -1;
    
    header->channel_id = CHANNEL_TRANSMITTER;
    header->sequence_token = nsigii_transmitter_next_sequence(transmitter);
    header->timestamp = nsigii_now_ns();
    header->codec_version = (uint8_t)(1 * 1);  /* 1 * 1/3 scaled */
    
    return 0;
}

int nsigii_transmitter_encode_payload(const uint8_t* content,
                                       size_t content_len,
                                       nsigii_payload_t* payload) {
    if (!content || !payload || content_len > MAX_CONTENT_SIZE) return -1;
    
    /* Apply NSIGII encoding: "Encoding Suffering into Silicon" */
    uint8_t encoded[MAX_CONTENT_SIZE];
    size_t encoded_len = MAX_CONTENT_SIZE;
    
    if (nsigii_suffering_to_silicon_encode(content, content_len, encoded, &encoded_len) != 0) {
        /* Fallback: direct copy */
        memcpy(payload->content, content, content_len);
        payload->content_length = (uint32_t)content_len;
    } else {
        memcpy(payload->content, encoded, encoded_len);
        payload->content_length = (uint32_t)encoded_len;
    }
    
    /* Compute SHA-256 hash */
    nsigii_sha256(payload->content, payload->content_length, payload->message_hash);
    
    return 0;
}

int nsigii_transmitter_encode_verification(nsigii_verification_t* verification) {
    if (!verification) return -1;
    
    /* Set RWX: Transmitter has WRITE permission */
    verification->rwx_flags = RWX_WRITE;
    
    /* Clear signature (will be set by verifier) */
    memset(verification->consensus_sig, 0, SIGNATURE_SIZE);
    
    /* Tag with human rights identifier */
    strncpy(verification->human_rights_tag, "NSIGII_HR_TRANSMIT", MAX_TAG_SIZE - 1);
    verification->human_rights_tag[MAX_TAG_SIZE - 1] = '\0';
    
    return 0;
}

int nsigii_transmitter_encode_topology(nsigii_topology_t* topology) {
    if (!topology) return -1;
    
    /* Position on Rational Wheel - start of wheel */
    topology->wheel_position = 0;
    topology->prev_channel = CHANNEL_VERIFIER;  /* Circular: loop back */
    topology->next_channel = CHANNEL_RECEIVER;  /* Forward to receiver */
    
    return 0;
}

/* ============================================================================
 * TRANSMISSION FUNCTIONS
 * ============================================================================ */

int nsigii_transmitter_transmit(nsigii_transmitter_t* transmitter,
                                 const nsigii_packet_t* packet) {
    if (!transmitter || !packet) return -1;
    
    transmitter->state = TRANSMITTER_STATE_TRANSMITTING;
    
    /* Serialize packet */
    uint8_t buffer[MAX_CONTENT_SIZE + 512];
    size_t len = 0;
    
    if (nsigii_serialize_binary(packet, buffer, &len) != 0) {
        transmitter->state = TRANSMITTER_STATE_ERROR;
        return -1;
    }
    
    /* Send packet */
    int result = nsigii_transmitter_send_raw(transmitter, buffer, len);
    
    /* Log consensus event */
    if (result == 0) {
        nsigii_log_consensus_event("TRANSMIT", packet);
    }
    
    transmitter->state = (result == 0) ? TRANSMITTER_STATE_IDLE : TRANSMITTER_STATE_ERROR;
    return result;
}

int nsigii_transmitter_send_packet(nsigii_transmitter_t* transmitter,
                                    const nsigii_packet_t* packet) {
    return nsigii_transmitter_transmit(transmitter, packet);
}

int nsigii_transmitter_send_raw(nsigii_transmitter_t* transmitter,
                                 const uint8_t* data,
                                 size_t len) {
    if (!transmitter || !data || len == 0) return -1;
    
    ssize_t sent = sendto(transmitter->target_socket, data, len, 0,
                          (struct sockaddr*)&transmitter->target_addr,
                          sizeof(transmitter->target_addr));
    
    if (sent < 0) {
        perror("sendto");
        return -1;
    }
    
    return 0;
}

/* ============================================================================
 * QUEUE MANAGEMENT
 * ============================================================================ */

int nsigii_transmitter_queue_message(nsigii_transmitter_t* transmitter,
                                      const uint8_t* message,
                                      size_t len) {
    if (!transmitter || !message || len == 0) return -1;
    
    /* Allocate message copy */
    uint8_t* msg_copy = (uint8_t*)malloc(len + sizeof(size_t));
    if (!msg_copy) return -1;
    
    *(size_t*)msg_copy = len;
    memcpy(msg_copy + sizeof(size_t), message, len);
    
    return nsigii_queue_enqueue(transmitter->message_queue, msg_copy);
}

int nsigii_transmitter_process_queue(nsigii_transmitter_t* transmitter) {
    if (!transmitter) return -1;
    
    int processed = 0;
    
    while (!nsigii_queue_is_empty(transmitter->message_queue)) {
        uint8_t* msg = (uint8_t*)nsigii_queue_dequeue(transmitter->message_queue);
        if (!msg) continue;
        
        size_t len = *(size_t*)msg;
        uint8_t* content = msg + sizeof(size_t);
        
        nsigii_packet_t packet;
        memset(&packet, 0, sizeof(packet));
        
        if (nsigii_transmitter_encode_message(transmitter, content, len, &packet) == 0) {
            nsigii_transmitter_transmit(transmitter, &packet);
            processed++;
        }
        
        free(msg);
    }
    
    return processed;
}

/* ============================================================================
 * STATE MANAGEMENT
 * ============================================================================ */

transmitter_state_t nsigii_transmitter_get_state(const nsigii_transmitter_t* transmitter) {
    return transmitter ? transmitter->state : TRANSMITTER_STATE_ERROR;
}

const char* nsigii_transmitter_state_string(transmitter_state_t state) {
    switch (state) {
        case TRANSMITTER_STATE_IDLE:        return "IDLE";
        case TRANSMITTER_STATE_ENCODING:    return "ENCODING";
        case TRANSMITTER_STATE_TRANSMITTING: return "TRANSMITTING";
        case TRANSMITTER_STATE_ERROR:       return "ERROR";
        default:                            return "UNKNOWN";
    }
}

/* ============================================================================
 * THREAD WORKER
 * ============================================================================ */

void* nsigii_transmitter_worker(void* arg) {
    nsigii_transmitter_t* transmitter = (nsigii_transmitter_t*)arg;
    if (!transmitter) return NULL;
    
    while (worker_running) {
        nsigii_transmitter_process_queue(transmitter);
        usleep(1000);  /* 1ms sleep */
    }
    
    return NULL;
}

int nsigii_transmitter_start_worker(nsigii_transmitter_t* transmitter) {
    if (!transmitter || worker_running) return -1;
    
    worker_running = 1;
    if (pthread_create(&worker_thread, NULL, nsigii_transmitter_worker, transmitter) != 0) {
        worker_running = 0;
        return -1;
    }
    
    return 0;
}

int nsigii_transmitter_stop_worker(nsigii_transmitter_t* transmitter) {
    if (!worker_running) return -1;
    
    worker_running = 0;
    pthread_join(worker_thread, NULL);
    return 0;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

uint32_t nsigii_transmitter_next_sequence(nsigii_transmitter_t* transmitter) {
    (void)transmitter;  /* Unused - using global sequence */
    
    pthread_mutex_lock(&sequence_mutex);
    uint32_t seq = ++global_sequence;
    pthread_mutex_unlock(&sequence_mutex);
    
    return seq;
}

double nsigii_transmitter_get_codec_ratio(void) {
    return 1.0 / 3.0;
}

const char* nsigii_transmitter_get_target_address(void) {
    return LOOPBACK_BASE "2";
}

uint16_t nsigii_transmitter_get_target_port(void) {
    return PORT_RECEIVER;
}
