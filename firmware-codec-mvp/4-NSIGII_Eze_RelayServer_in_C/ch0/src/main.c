/* ============================================================================
 * NSIGII CHANNEL 0 MAIN - Transmitter Process
 * Rectorial Reasoning Rational Wheel Framework
 * ============================================================================ */

#include "../include/nsigii.h"
#include "../include/transmitter/transmitter.h"
#include "../include/serialization.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

/* ============================================================================
 * GLOBAL STATE
 * ============================================================================ */

static volatile int system_active = 1;
static nsigii_transmitter_t* g_transmitter = NULL;

/* ============================================================================
 * SIGNAL HANDLERS
 * ============================================================================ */

void signal_handler(int sig) {
    (void)sig;
    printf("\n[CH0] Shutting down transmitter...\n");
    system_active = 0;
}

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================ */

void print_banner(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║  NSIGII COMMAND AND CONTROL HUMAN RIGHTS VERIFICATION SYSTEM    ║\n");
    printf("║  Channel 0: TRANSMITTER (1 * 1/3)                               ║\n");
    printf("║  Rectorial Reasoning Rational Wheel Framework                   ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf("Version: %s | Loopback: %s1 | Port: %d\n\n",
           NSIGII_VERSION, LOOPBACK_BASE, PORT_TRANSMITTER);
}

void print_packet_info(const nsigii_packet_t* packet) {
    printf("[CH0] Packet encoded:\n");
    printf("      Channel: %d | Sequence: %u | Wheel: %d°\n",
           packet->header.channel_id,
           packet->header.sequence_token,
           packet->topology.wheel_position);
    printf("      RWX: 0x%02X | Tag: %s\n",
           packet->verification.rwx_flags,
           packet->verification.human_rights_tag);
}

/* ============================================================================
 * INPUT ACQUISITION (from scanning.c equivalent)
 * ============================================================================ */

int acquire_input(uint8_t* buffer, size_t max_len) {
    printf("[CH0] Enter message (or 'quit' to exit): ");
    fflush(stdout);
    
    if (fgets((char*)buffer, (int)max_len, stdin) == NULL) {
        return -1;
    }
    
    /* Remove newline */
    size_t len = strlen((char*)buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }
    
    return (int)len;
}

/* ============================================================================
 * MAIN COMMAND AND CONTROL LOOP
 * ============================================================================ */

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    print_banner();
    
    /* Initialize transmitter */
    printf("[CH0] Initializing transmitter...\n");
    if (nsigii_transmitter_create(&g_transmitter) != 0) {
        fprintf(stderr, "[CH0] Failed to create transmitter\n");
        return 1;
    }
    
    /* Start worker thread */
    printf("[CH0] Starting worker thread...\n");
    if (nsigii_transmitter_start_worker(g_transmitter) != 0) {
        fprintf(stderr, "[CH0] Failed to start worker\n");
        nsigii_transmitter_destroy(g_transmitter);
        return 1;
    }
    
    printf("[CH0] Transmitter ready. Waiting for input...\n\n");
    
    /* Main verification loop - On-the-fly realtime processing */
    while (system_active) {
        uint8_t raw_message[MAX_CONTENT_SIZE];
        
        /* Phase 1: Transmitter generates message */
        int msg_len = acquire_input(raw_message, sizeof(raw_message));
        if (msg_len < 0) break;
        
        /* Check for quit command */
        if (strcmp((char*)raw_message, "quit") == 0) {
            break;
        }
        
        /* Encode and transmit */
        nsigii_packet_t packet;
        memset(&packet, 0, sizeof(packet));
        
        if (nsigii_transmitter_encode_message(g_transmitter, raw_message, 
                                               (size_t)msg_len, &packet) == 0) {
            print_packet_info(&packet);
            
            if (nsigii_transmitter_transmit(g_transmitter, &packet) == 0) {
                printf("[CH0] ✓ Message transmitted to receiver\n\n");
            } else {
                printf("[CH0] ✗ Transmission failed\n\n");
            }
        } else {
            printf("[CH0] ✗ Encoding failed\n\n");
        }
        
        /* Rectorial Reasoning: Update Rational Wheel */
        nsigii_rotate_rational_wheel(1);  /* 1 degree increment */
    }
    
    /* Cleanup */
    printf("\n[CH0] Cleaning up...\n");
    nsigii_transmitter_stop_worker(g_transmitter);
    nsigii_transmitter_destroy(g_transmitter);
    
    printf("[CH0] Shutdown complete.\n");
    return 0;
}
