/* ============================================================================
 * NSIGII RECEIVER MODULE IMPLEMENTATION - Channel 1 (2 * 2/3)
 * Triangle Second Receiver - Bipolar Order & Chaos
 * ============================================================================ */

#include "receiver/receiver.h"
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

/* ============================================================================
 * STATIC VARIABLES
 * ============================================================================ */

static pthread_t worker_thread;
static int worker_running = 0;

/* ============================================================================
 * INITIALIZATION AND CLEANUP
 * ============================================================================ */

int nsigii_receiver_create(nsigii_receiver_t** receiver) {
    if (!receiver) return -1;
    
    *receiver = (nsigii_receiver_t*)malloc(sizeof(nsigii_receiver_t));
    if (!*receiver) return -1;
    
    memset(*receiver, 0, sizeof(nsigii_receiver_t));
    return nsigii_receiver_init(*receiver);
}

void nsigii_receiver_destroy(nsigii_receiver_t* receiver) {
    if (!receiver) return;
    
    nsigii_receiver_cleanup(receiver);
    free(receiver);
}

int nsigii_receiver_init(nsigii_receiver_t* receiver) {
    if (!receiver) return -1;
    
    /* Initialize base channel */
    receiver->base.channel_id = CHANNEL_RECEIVER;
    strncpy(receiver->base.loopback_addr, LOOPBACK_BASE "2", 15);
    receiver->base.loopback_addr[15] = '\0';
    snprintf(receiver->base.dns_name, sizeof(receiver->base.dns_name),
             "receiver.%s", DNS_NAMESPACE);
    receiver->base.codec_ratio = 2.0 / 3.0;
    receiver->base.state = STATE_CHAOS;  /* Bipolar: Chaos state */
    receiver->base.port = PORT_RECEIVER;
    
    /* Initialize receiver-specific fields */
    receiver->state = RECEIVER_STATE_IDLE;
    receiver->current_bipolar_state = STATE_CHAOS;
    receiver->receive_queue = nsigii_queue_create();
    receiver->forward_queue = nsigii_queue_create();
    
    if (!receiver->receive_queue || !receiver->forward_queue) {
        return -1;
    }
    
    /* Create listen socket */
    receiver->listen_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiver->listen_socket < 0) {
        perror("socket");
        return -1;
    }

    /* Allow socket reuse to prevent "Address already in use" */
    int reuse = 1;
    setsockopt(receiver->listen_socket, SOL_SOCKET, SO_REUSEADDR,
               (const char*)&reuse, sizeof(reuse));

    /* Set up listen address */
    memset(&receiver->listen_addr, 0, sizeof(receiver->listen_addr));
    receiver->listen_addr.sin_family = AF_INET;
    receiver->listen_addr.sin_port = htons(PORT_RECEIVER);
    inet_pton(AF_INET, LOOPBACK_BASE "2", &receiver->listen_addr.sin_addr);
    
    /* Bind socket */
    if (bind(receiver->listen_socket, (struct sockaddr*)&receiver->listen_addr,
             sizeof(receiver->listen_addr)) < 0) {
        perror("bind");
        close(receiver->listen_socket);
        return -1;
    }
    
    /* Set up forward address (Verifier) */
    memset(&receiver->forward_addr, 0, sizeof(receiver->forward_addr));
    receiver->forward_addr.sin_family = AF_INET;
    receiver->forward_addr.sin_port = htons(PORT_VERIFIER);
    inet_pton(AF_INET, LOOPBACK_BASE "3", &receiver->forward_addr.sin_addr);
    
    /* Create forward socket */
    receiver->forward_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiver->forward_socket < 0) {
        perror("forward socket");
        close(receiver->listen_socket);
        return -1;
    }
    
    return 0;
}

void nsigii_receiver_cleanup(nsigii_receiver_t* receiver) {
    if (!receiver) return;
    
    if (receiver->listen_socket >= 0) {
        close(receiver->listen_socket);
        receiver->listen_socket = -1;
    }
    
    if (receiver->forward_socket >= 0) {
        close(receiver->forward_socket);
        receiver->forward_socket = -1;
    }
    
    if (receiver->receive_queue) {
        nsigii_queue_destroy(receiver->receive_queue);
        receiver->receive_queue = NULL;
    }
    
    if (receiver->forward_queue) {
        nsigii_queue_destroy(receiver->forward_queue);
        receiver->forward_queue = NULL;
    }
}

/* ============================================================================
 * CORE DECODING FUNCTIONS
 * ============================================================================ */

int nsigii_receiver_decode_packet(nsigii_receiver_t* receiver,
                                   const uint8_t* encoded_data,
                                   size_t data_len,
                                   nsigii_packet_t* packet) {
    if (!receiver || !encoded_data || !packet || data_len < 100) {
        return -1;
    }
    
    receiver->state = RECEIVER_STATE_DECODING;
    
    /* Deserialize binary packet */
    if (nsigii_deserialize_binary(encoded_data, data_len, packet) != 0) {
        receiver->state = RECEIVER_STATE_ERROR;
        return -1;
    }
    
    /* Decode from silicon representation */
    uint8_t decoded[MAX_CONTENT_SIZE];
    size_t decoded_len = MAX_CONTENT_SIZE;
    
    if (nsigii_silicon_to_suffering_decode(packet->payload.content,
                                            packet->payload.content_length,
                                            decoded, &decoded_len) == 0) {
        memcpy(packet->payload.content, decoded, decoded_len);
        packet->payload.content_length = (uint32_t)decoded_len;
    }
    
    /* Verify hash integrity */
    if (!nsigii_receiver_verify_hash(&packet->payload)) {
        fprintf(stderr, "Hash mismatch in receiver\n");
        receiver->state = RECEIVER_STATE_ERROR;
        return -1;
    }
    
    /* Set RWX: Receiver has READ permission */
    packet->verification.rwx_flags = RWX_READ;
    
    /* Update topology links */
    packet->topology.prev_channel = CHANNEL_TRANSMITTER;
    packet->topology.next_channel = CHANNEL_VERIFIER;
    packet->topology.wheel_position = 120;  /* 120 degrees on wheel */
    
    /* Bipartite order check */
    nsigii_receiver_update_bipolar_state(receiver, packet->header.sequence_token);
    
    receiver->state = RECEIVER_STATE_IDLE;
    return 0;
}

/* ============================================================================
 * HASH VERIFICATION
 * ============================================================================ */

bool nsigii_receiver_verify_hash(const nsigii_payload_t* payload) {
    if (!payload) return false;
    
    uint8_t computed_hash[HASH_SIZE];
    nsigii_sha256(payload->content, payload->content_length, computed_hash);
    
    return (memcmp(computed_hash, payload->message_hash, HASH_SIZE) == 0);
}

int nsigii_receiver_compute_hash(const uint8_t* content,
                                  size_t len,
                                  uint8_t* hash) {
    if (!content || !hash) return -1;
    
    nsigii_sha256(content, len, hash);
    return 0;
}

/* ============================================================================
 * BIPOLAR STATE MANAGEMENT
 * ============================================================================ */

uint8_t nsigii_receiver_determine_bipolar_state(uint32_t sequence_token) {
    /* Bipartite order check: Order vs Chaos */
    return (sequence_token % 2 == 0) ? STATE_ORDER : STATE_CHAOS;
}

void nsigii_receiver_update_bipolar_state(nsigii_receiver_t* receiver,
                                           uint32_t sequence_token) {
    if (!receiver) return;
    
    receiver->current_bipolar_state = nsigii_receiver_determine_bipolar_state(sequence_token);
    receiver->base.state = receiver->current_bipolar_state;
}

const char* nsigii_receiver_bipolar_state_string(uint8_t state) {
    switch (state) {
        case STATE_ORDER:   return "ORDER";
        case STATE_CHAOS:   return "CHAOS";
        case STATE_VERIFIED: return "VERIFIED";
        default:            return "UNKNOWN";
    }
}

/* ============================================================================
 * RECEPTION FUNCTIONS
 * ============================================================================ */

int nsigii_receiver_receive(nsigii_receiver_t* receiver,
                             nsigii_packet_t* packet) {
    if (!receiver || !packet) return -1;
    
    receiver->state = RECEIVER_STATE_RECEIVING;
    
    uint8_t buffer[MAX_CONTENT_SIZE + 512];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    ssize_t received = recvfrom(receiver->listen_socket, buffer, sizeof(buffer), 0,
                                (struct sockaddr*)&from_addr, &from_len);
    
    if (received < 0) {
        perror("recvfrom");
        receiver->state = RECEIVER_STATE_ERROR;
        return -1;
    }
    
    /* Decode packet */
    if (nsigii_receiver_decode_packet(receiver, buffer, (size_t)received, packet) != 0) {
        receiver->state = RECEIVER_STATE_ERROR;
        return -1;
    }
    
    receiver->state = RECEIVER_STATE_IDLE;
    return 0;
}

int nsigii_receiver_listen(nsigii_receiver_t* receiver) {
    if (!receiver) return -1;
    
    /* Already bound in init */
    return 0;
}

int nsigii_receiver_receive_data(nsigii_receiver_t* receiver,
                                  uint8_t* buffer,
                                  size_t* len) {
    if (!receiver || !buffer || !len) return -1;
    
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    
    ssize_t received = recvfrom(receiver->listen_socket, buffer, *len, 0,
                                (struct sockaddr*)&from_addr, &from_len);
    
    if (received < 0) {
        perror("recvfrom");
        return -1;
    }
    
    *len = (size_t)received;
    return 0;
}

/* ============================================================================
 * FORWARDING FUNCTIONS
 * ============================================================================ */

int nsigii_receiver_forward_to_verifier(nsigii_receiver_t* receiver,
                                         const nsigii_packet_t* packet) {
    if (!receiver || !packet) return -1;
    
    receiver->state = RECEIVER_STATE_FORWARDING;
    
    /* Serialize packet */
    uint8_t buffer[MAX_CONTENT_SIZE + 512];
    size_t len = 0;
    
    if (nsigii_serialize_binary(packet, buffer, &len) != 0) {
        receiver->state = RECEIVER_STATE_ERROR;
        return -1;
    }
    
    /* Send to verifier */
    ssize_t sent = sendto(receiver->forward_socket, buffer, len, 0,
                          (struct sockaddr*)&receiver->forward_addr,
                          sizeof(receiver->forward_addr));
    
    if (sent < 0) {
        perror("sendto verifier");
        receiver->state = RECEIVER_STATE_ERROR;
        return -1;
    }
    
    /* Log consensus event */
    nsigii_log_consensus_event("RECEIVE_FORWARD", packet);
    
    receiver->state = RECEIVER_STATE_IDLE;
    return 0;
}

int nsigii_receiver_connect_to_verifier(nsigii_receiver_t* receiver) {
    if (!receiver) return -1;
    
    /* UDP doesn't require explicit connection */
    return 0;
}

int nsigii_receiver_forward_packet(nsigii_receiver_t* receiver,
                                    const nsigii_packet_t* packet) {
    return nsigii_receiver_forward_to_verifier(receiver, packet);
}

/* ============================================================================
 * QUEUE MANAGEMENT
 * ============================================================================ */

int nsigii_receiver_enqueue_received(nsigii_receiver_t* receiver,
                                      const nsigii_packet_t* packet) {
    if (!receiver || !packet) return -1;
    
    nsigii_packet_t* copy = (nsigii_packet_t*)malloc(sizeof(nsigii_packet_t));
    if (!copy) return -1;
    
    memcpy(copy, packet, sizeof(nsigii_packet_t));
    return nsigii_queue_enqueue(receiver->receive_queue, copy);
}

int nsigii_receiver_enqueue_forward(nsigii_receiver_t* receiver,
                                     const nsigii_packet_t* packet) {
    if (!receiver || !packet) return -1;
    
    nsigii_packet_t* copy = (nsigii_packet_t*)malloc(sizeof(nsigii_packet_t));
    if (!copy) return -1;
    
    memcpy(copy, packet, sizeof(nsigii_packet_t));
    return nsigii_queue_enqueue(receiver->forward_queue, copy);
}

nsigii_packet_t* nsigii_receiver_dequeue_received(nsigii_receiver_t* receiver) {
    if (!receiver) return NULL;
    return (nsigii_packet_t*)nsigii_queue_dequeue(receiver->receive_queue);
}

nsigii_packet_t* nsigii_receiver_dequeue_forward(nsigii_receiver_t* receiver) {
    if (!receiver) return NULL;
    return (nsigii_packet_t*)nsigii_queue_dequeue(receiver->forward_queue);
}

/* ============================================================================
 * STATE MANAGEMENT
 * ============================================================================ */

receiver_state_t nsigii_receiver_get_state(const nsigii_receiver_t* receiver) {
    return receiver ? receiver->state : RECEIVER_STATE_ERROR;
}

const char* nsigii_receiver_state_string(receiver_state_t state) {
    switch (state) {
        case RECEIVER_STATE_IDLE:           return "IDLE";
        case RECEIVER_STATE_RECEIVING:      return "RECEIVING";
        case RECEIVER_STATE_DECODING:       return "DECODING";
        case RECEIVER_STATE_VERIFYING_HASH: return "VERIFYING_HASH";
        case RECEIVER_STATE_FORWARDING:     return "FORWARDING";
        case RECEIVER_STATE_ERROR:          return "ERROR";
        default:                            return "UNKNOWN";
    }
}

/* ============================================================================
 * THREAD WORKER
 * ============================================================================ */

void* nsigii_receiver_worker(void* arg) {
    nsigii_receiver_t* receiver = (nsigii_receiver_t*)arg;
    if (!receiver) return NULL;
    
    while (worker_running) {
        nsigii_packet_t packet;
        memset(&packet, 0, sizeof(packet));
        
        /* Receive packet */
        if (nsigii_receiver_receive(receiver, &packet) == 0) {
            /* Forward to verifier */
            nsigii_receiver_forward_to_verifier(receiver, &packet);
        }
        
        usleep(1000);  /* 1ms sleep */
    }
    
    return NULL;
}

int nsigii_receiver_start_worker(nsigii_receiver_t* receiver) {
    if (!receiver || worker_running) return -1;
    
    worker_running = 1;
    if (pthread_create(&worker_thread, NULL, nsigii_receiver_worker, receiver) != 0) {
        worker_running = 0;
        return -1;
    }
    
    return 0;
}

int nsigii_receiver_stop_worker(nsigii_receiver_t* receiver) {
    (void)receiver;
    
    if (!worker_running) return -1;
    
    worker_running = 0;
    pthread_join(worker_thread, NULL);
    return 0;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

double nsigii_receiver_get_codec_ratio(void) {
    return 2.0 / 3.0;
}

const char* nsigii_receiver_get_listen_address(void) {
    return LOOPBACK_BASE "2";
}

uint16_t nsigii_receiver_get_listen_port(void) {
    return PORT_RECEIVER;
}

const char* nsigii_receiver_get_forward_address(void) {
    return LOOPBACK_BASE "3";
}

uint16_t nsigii_receiver_get_forward_port(void) {
    return PORT_VERIFIER;
}
