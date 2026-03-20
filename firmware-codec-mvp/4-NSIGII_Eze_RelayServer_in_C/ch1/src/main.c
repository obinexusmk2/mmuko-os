/* ============================================================================
 * NSIGII CHANNEL 1 MAIN - Receiver + Verifier Process
 * Rectorial Reasoning Rational Wheel Framework
 * Tomographic Pair Resolving
 * ============================================================================
 * 
 * This process combines Receiver (Channel 1) and Verifier (Channel 2)
 * for tomographic pair resolving and consensus verification.
 */

#include "nsigii.h"
#include "receiver/receiver.h"
#include "verifier/verifier.h"
#include "serialization.h"
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
static nsigii_receiver_t* g_receiver = NULL;
static nsigii_verifier_t* g_verifier = NULL;
static uint16_t g_wheel_position = 0;

/* ============================================================================
 * SIGNAL HANDLERS
 * ============================================================================ */

void signal_handler(int sig) {
    (void)sig;
    printf("\n[CH1] Shutting down receiver/verifier...\n");
    system_active = 0;
}

/* ============================================================================
 * BANNER
 * ============================================================================ */

void print_banner(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║  NSIGII COMMAND AND CONTROL HUMAN RIGHTS VERIFICATION SYSTEM    ║\n");
    printf("║  Channel 1: RECEIVER (2 * 2/3) + Channel 2: VERIFIER (3 * 3/3)  ║\n");
    printf("║  Tomographic Pair Resolving - Rectorial Reasoning Rational Wheel║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n");
    printf("Version: %s | Loopback: %s2/3 | Ports: %d/%d\n\n",
           NSIGII_VERSION, LOOPBACK_BASE, PORT_RECEIVER, PORT_VERIFIER);
}

/* ============================================================================
 * TOMOGRAPHIC PAIR RESOLVING
 * ============================================================================ */

void print_tomographic_result(const nsigii_tomographic_pair_t* pair) {
    printf("[CH1] Tomographic Pair Analysis:\n");
    printf("      Correlation: %.4f | Resolved: %s\n",
           pair->correlation, pair->resolved ? "YES" : "NO");
    
    for (int i = 0; i < 2; i++) {
        if (pair->pair[i]) {
            printf("      Packet[%d]: Seq=%u Wheel=%d°\n",
                   i, pair->pair[i]->header.sequence_token,
                   pair->pair[i]->topology.wheel_position);
        }
    }
}

int process_tomographic_pair(nsigii_receiver_t* receiver,
                              nsigii_verifier_t* verifier,
                              nsigii_packet_t* packet1,
                              nsigii_packet_t* packet2) {
    if (!receiver || !verifier || !packet1 || !packet2) return -1;
    
    /* Create tomographic pair */
    nsigii_tomographic_pair_t pair;
    memset(&pair, 0, sizeof(pair));
    pair.pair[0] = packet1;
    pair.pair[1] = packet2;
    
    /* Compute correlation */
    pair.correlation = nsigii_tomographic_compute_correlation(packet1, packet2);
    
    /* Validate pair */
    pair.resolved = nsigii_tomographic_validate_pair(&pair);
    
    print_tomographic_result(&pair);
    
    if (pair.resolved) {
        /* Process first packet through verifier */
        nsigii_verification_result_t result;
        memset(&result, 0, sizeof(result));
        
        if (nsigii_verifier_verify_packet(verifier, packet1, &result) == 0) {
            printf("[CH1] ✓ Packet verified - Consensus: %.3f\n", result.consensus_score);
            
            /* Emit consensus message */
            nsigii_consensus_message_t consensus;
            memset(&consensus, 0, sizeof(consensus));
            nsigii_verifier_emit_consensus_message(verifier, result.verified_packet, &consensus);
            
            printf("[CH1] ✓ Consensus emitted: %s\n", consensus.status);
            
            if (result.verified_packet) {
                free(result.verified_packet);
            }
        } else {
            printf("[CH1] ✗ Verification failed: %s\n", result.status);
        }
    } else {
        printf("[CH1] ✗ Tomographic pair resolution failed\n");
    }
    
    return pair.resolved ? 0 : -1;
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
    
    /* Initialize receiver */
    printf("[CH1] Initializing receiver...\n");
    if (nsigii_receiver_create(&g_receiver) != 0) {
        fprintf(stderr, "[CH1] Failed to create receiver\n");
        return 1;
    }
    
    /* Initialize verifier */
    printf("[CH1] Initializing verifier...\n");
    if (nsigii_verifier_create(&g_verifier) != 0) {
        fprintf(stderr, "[CH1] Failed to create verifier\n");
        nsigii_receiver_destroy(g_receiver);
        return 1;
    }
    
    /* Start receiver worker */
    printf("[CH1] Starting receiver worker...\n");
    if (nsigii_receiver_start_worker(g_receiver) != 0) {
        fprintf(stderr, "[CH1] Failed to start receiver worker\n");
        nsigii_verifier_destroy(g_verifier);
        nsigii_receiver_destroy(g_receiver);
        return 1;
    }
    
    /* Start verifier worker */
    printf("[CH1] Starting verifier worker...\n");
    if (nsigii_verifier_start_worker(g_verifier) != 0) {
        fprintf(stderr, "[CH1] Failed to start verifier worker\n");
        nsigii_receiver_stop_worker(g_receiver);
        nsigii_verifier_destroy(g_verifier);
        nsigii_receiver_destroy(g_receiver);
        return 1;
    }
    
    printf("[CH1] Receiver/Verifier ready. Processing packets...\n\n");
    
    /* Main verification loop with tomographic pair resolving */
    nsigii_packet_t packet_buffer[2];
    int packet_count = 0;
    
    while (system_active) {
        /* Receive packet */
        nsigii_packet_t packet;
        memset(&packet, 0, sizeof(packet));
        
        if (nsigii_receiver_receive(g_receiver, &packet) == 0) {
            printf("[CH1] Packet received from transmitter:\n");
            printf("      Channel: %d | Sequence: %u | Bipolar: %s\n",
                   packet.header.channel_id,
                   packet.header.sequence_token,
                   nsigii_receiver_bipolar_state_string(
                       nsigii_receiver_determine_bipolar_state(packet.header.sequence_token)));
            
            /* Store packet for tomographic pairing */
            memcpy(&packet_buffer[packet_count], &packet, sizeof(nsigii_packet_t));
            packet_count++;
            
            /* Process pair when we have 2 packets */
            if (packet_count >= 2) {
                process_tomographic_pair(g_receiver, g_verifier,
                                         &packet_buffer[0], &packet_buffer[1]);
                packet_count = 0;
                printf("\n");
            }
            
            /* Forward to verifier (also done in worker) */
            nsigii_receiver_forward_to_verifier(g_receiver, &packet);
            
            /* Rectorial Reasoning: Update Rational Wheel */
            g_wheel_position = (g_wheel_position + 1) % 360;
            nsigii_rotate_rational_wheel(1);
        }
        
        usleep(1000);  /* 1ms sleep to prevent CPU spinning */
    }
    
    /* Cleanup */
    printf("\n[CH1] Cleaning up...\n");
    nsigii_receiver_stop_worker(g_receiver);
    nsigii_verifier_stop_worker(g_verifier);
    nsigii_verifier_destroy(g_verifier);
    nsigii_receiver_destroy(g_receiver);
    
    printf("[CH1] Shutdown complete.\n");
    return 0;
}
