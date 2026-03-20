/*
 * NSIGII MAGNETIC SYMBOL PROTOCOL — Implementation
 * nsigii_magnetic.c
 *
 * Version: 0.1
 * Author: Nnamdi Okpala (OBINexus Computing)
 * Date: 21 February 2026
 *
 * Build: nlink → polybuild
 * cc -o nsigii_magnetic.o -c nsigii_magnetic.c -lm
 */

#include "nsigii_magnetic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * INTERNAL HELPERS
 * ============================================================================ */

static double current_unix_time(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static void generate_id(char *buf, size_t len) {
    static uint32_t counter = 0;
    snprintf(buf, len, "MSG%05u", ++counter);
}

/* Normalise bearing to [0, 360) */
static double normalise_bearing(double d) {
    while (d >= 360.0) d -= 360.0;
    while (d <   0.0) d += 360.0;
    return d;
}

/* ============================================================================
 * BLOCH SPHERE
 * ============================================================================ */

nsigii_bloch_t nsigii_bloch_from_bearing(nsigii_bearing_t bearing,
                                          double b, double a) {
    nsigii_bloch_t bloch;
    bloch.theta = bearing.degrees;
    bloch.psi   = nsigii_bloch_psi(b, a);
    return bloch;
}

/* ============================================================================
 * MAGNETIC TRIDENT
 * ============================================================================ */

void nsigii_mag_trident_init(nsigii_mag_trident_t *t) {
    if (!t) return;
    t->bearings[POLE_UCHE].degrees = BEARING_UCHE;
    t->bearings[POLE_EZE ].degrees = BEARING_EZE;
    t->bearings[POLE_OBI ].degrees = BEARING_OBI;
    t->center = nsigii_mag_trident_center(t);
}

nsigii_bearing_t nsigii_mag_trident_center(const nsigii_mag_trident_t *t) {
    nsigii_bearing_t c;
    if (!t) { c.degrees = 0.0; return c; }
    c.degrees = normalise_bearing(
        (t->bearings[0].degrees +
         t->bearings[1].degrees +
         t->bearings[2].degrees) / 3.0);
    return c;
}

nsigii_bearing_t nsigii_mag_weighted_drift(double weight,
                                            nsigii_bearing_t observer,
                                            nsigii_bearing_t target) {
    nsigii_bearing_t result;
    result.degrees = normalise_bearing(
        (1.0 - weight) * observer.degrees + weight * target.degrees);
    return result;
}

nsigii_bearing_t nsigii_mag_midpoint(nsigii_bearing_t b1,
                                      nsigii_bearing_t b2,
                                      double weight) {
    nsigii_bearing_t result;
    result.degrees = normalise_bearing(
        (1.0 - weight) * b1.degrees + weight * b2.degrees);
    return result;
}

/* ============================================================================
 * MMUKO ACTUATORS
 * ============================================================================ */

/* UCHE — Knowledge — Send Actuator (Channel 0) */
nsigii_mag_message_t *nsigii_uche_encode(const char *content,
                                          double force,
                                          double stiffness,
                                          nsigii_mag_trident_t *topo) {
    if (!content || !topo) return NULL;

    nsigii_mag_message_t *msg = calloc(1, sizeof(nsigii_mag_message_t));
    if (!msg) return NULL;

    generate_id(msg->id, sizeof(msg->id));
    strncpy(msg->content, content, sizeof(msg->content) - 1);

    msg->spring.force      = force;
    msg->spring.stiffness  = stiffness;
    msg->spring.extension  = 0.0;     /* Not yet extended — encode only */

    msg->source     = POLE_UCHE;
    msg->state      = MAG_STATE_ENCODED;
    msg->encoded_at = current_unix_time();
    msg->is_replay  = false;

    /* Bloch state from UCHE bearing */
    msg->bloch = nsigii_bloch_from_bearing(topo->bearings[POLE_UCHE], 1.0, 1.0);

    /* Advance state to ORIENTED */
    msg->state = MAG_STATE_ORIENTED;

    printf("[UCHE] Encoded message %s | F=%.2f K=%.2f | Bearing=%.1f°\n",
           msg->id, force, stiffness, topo->bearings[POLE_UCHE].degrees);

    return msg;
}

/* EZE — Leadership — Transit Actuator (Channel 1) */
nsigii_mag_consensus_t nsigii_eze_control(nsigii_mag_message_t *msg) {
    if (!msg) return MAG_CONSENSUS_NO;
    if (msg->state < MAG_STATE_ORIENTED) return MAG_CONSENSUS_NO;

    /* Extend the spring — model transit as extension toward full collapse */
    /* Half extension = MAYBE threshold */
    double half_ext  = nsigii_half_extension(&msg->spring);
    double full_ext  = nsigii_full_extension(&msg->spring);

    /* Simulate transit: set extension to full (production: incremental) */
    msg->spring.extension = full_ext;
    msg->state = MAG_STATE_SENDING;

    nsigii_mag_consensus_t consensus = nsigii_spring_consensus(&msg->spring);

    /* Advance to IN_TRANSIT */
    if (consensus != MAG_CONSENSUS_NO) {
        msg->state = MAG_STATE_IN_TRANSIT;
    }

    printf("[EZE]  Transit control | ext=%.3f half=%.3f full=%.3f | Consensus=%s\n",
           msg->spring.extension, half_ext, full_ext,
           consensus == MAG_CONSENSUS_YES   ? "YES"   :
           consensus == MAG_CONSENSUS_MAYBE ? "MAYBE" : "NO");

    return consensus;
}

/* OBI — Heart — Receive Actuator (Channel 2)
 * AXIOM: collapse = received  */
bool nsigii_obi_receive(nsigii_mag_message_t *msg,
                         nsigii_rights_record_t *rights_out) {
    if (!msg || msg->state < MAG_STATE_IN_TRANSIT) return false;

    double ratio = nsigii_collapse_ratio(&msg->spring);
    if (ratio < 1.0) {
        printf("[OBI]  Collapse ratio %.3f < 1.0 — cannot receive\n", ratio);
        return false;
    }

    /* COLLAPSE EVENT */
    msg->state        = MAG_STATE_COLLAPSED;
    msg->collapsed_at = current_unix_time();
    snprintf(msg->collapse_sig, sizeof(msg->collapse_sig),
             "LTE%05u", (uint32_t)(msg->collapsed_at * 1000) % 99999);

    /* Advance to SEALED */
    msg->sealed_at = msg->collapsed_at + 0.001;
    msg->state     = MAG_STATE_SEALED;

    printf("[OBI]  Message %s COLLAPSED at %.3f | Sig: %s\n",
           msg->id, msg->collapsed_at, msg->collapse_sig);

    /* Populate constitutional rights record */
    if (rights_out) {
        strncpy(rights_out->message_id, msg->id, sizeof(rights_out->message_id) - 1);
        rights_out->sender_retrieval_revoked   = true;   /* Sender loses access  */
        rights_out->receiver_ownership_granted = true;   /* Receiver owns it     */
        rights_out->constitutional_breach      = false;
        rights_out->breach_timestamp           = 0.0;
        memset(rights_out->breach_reason, 0, sizeof(rights_out->breach_reason));
        strncpy(rights_out->lte_forensic_sig, msg->collapse_sig,
                sizeof(rights_out->lte_forensic_sig) - 1);
    }

    return true;   /* collapse = received */
}

/* ============================================================================
 * FULL PROTOCOL — 6-STEP TELEPORTATION
 * ============================================================================ */

nsigii_mag_message_t *nsigii_mag_teleport(const char *content,
                                           double force,
                                           double stiffness,
                                           nsigii_mag_trident_t *topo,
                                           nsigii_rights_record_t *rights_out) {
    printf("\n[NSIGII] Beginning 6-step magnetic teleportation...\n");

    /* Step 1-2: ENCODED → ORIENTED via Uche */
    nsigii_mag_message_t *msg = nsigii_uche_encode(content, force, stiffness, topo);
    if (!msg) {
        fprintf(stderr, "[NSIGII] Uche encode failed\n");
        return NULL;
    }

    /* Step 3-4: SENDING → IN_TRANSIT via Eze */
    nsigii_mag_consensus_t consensus = nsigii_eze_control(msg);
    if (consensus == MAG_CONSENSUS_NO) {
        fprintf(stderr, "[NSIGII] Eze transit refused — consensus NO\n");
        free(msg);
        return NULL;
    }
    if (consensus == MAG_CONSENSUS_MAYBE) {
        printf("[NSIGII] WARNING: Eze returned MAYBE — constitutional deadlock risk\n");
    }

    /* Step 5-6: COLLAPSED → SEALED via Obi */
    bool received = nsigii_obi_receive(msg, rights_out);
    if (!received) {
        fprintf(stderr, "[NSIGII] Obi receive failed — collapse incomplete\n");
        free(msg);
        return NULL;
    }

    printf("[NSIGII] Teleportation complete | %s → SEALED\n\n", msg->id);
    return msg;
}

/* ============================================================================
 * FORENSIC REPLAY
 * ============================================================================ */

nsigii_mag_message_t *nsigii_mag_replay_copy(const nsigii_mag_message_t *original) {
    if (!original) return NULL;
    nsigii_mag_message_t *copy = malloc(sizeof(nsigii_mag_message_t));
    if (!copy) return NULL;
    memcpy(copy, original, sizeof(nsigii_mag_message_t));
    copy->is_replay  = true;
    copy->packet_ref = NULL;  /* Replay does not carry live packet reference */
    printf("[REPLAY] Forensic copy of %s created (REPLAY ≠ RE-SEND)\n", original->id);
    return copy;
}

/* ============================================================================
 * FORENSIC RECORD
 * ============================================================================ */

void nsigii_mag_forensic_record(const nsigii_mag_message_t *msg,
                                 const nsigii_rights_record_t *rights,
                                 char *buffer,
                                 size_t len) {
    if (!msg || !buffer) return;
    snprintf(buffer, len,
        "=== NSIGII CONSTITUTIONAL FORENSIC RECORD ===\n"
        "Message ID        : %s\n"
        "State             : %d\n"
        "Encoded At        : %.3f\n"
        "Collapsed At      : %.3f\n"
        "Sealed At         : %.3f\n"
        "LTE Signature     : %s\n"
        "Is Replay         : %s\n"
        "Spring F/K/E      : %.2f / %.2f / %.2f\n"
        "Collapse Ratio    : %.4f\n"
        "Bloch θ/Ψ         : %.2f° / %.4f rad\n"
        "Sender Retrieval  : %s\n"
        "Receiver Owned    : %s\n"
        "Const. Breach     : %s\n"
        "Breach Reason     : %s\n"
        "==============================================\n",
        msg->id,
        (int)msg->state,
        msg->encoded_at,
        msg->collapsed_at,
        msg->sealed_at,
        msg->collapse_sig,
        msg->is_replay ? "YES" : "NO",
        msg->spring.force, msg->spring.stiffness, msg->spring.extension,
        nsigii_collapse_ratio(&msg->spring),
        msg->bloch.theta, msg->bloch.psi,
        rights ? (rights->sender_retrieval_revoked   ? "REVOKED"  : "ACTIVE") : "N/A",
        rights ? (rights->receiver_ownership_granted ? "YES"      : "NO"    ) : "N/A",
        rights ? (rights->constitutional_breach      ? "YES ⚠"   : "NO"    ) : "N/A",
        rights ? rights->breach_reason : "N/A");
}

/* ============================================================================
 * CONSTITUTIONAL DECAY
 * ============================================================================ */

void nsigii_mag_apply_decay(nsigii_mag_message_t *msg,
                             nsigii_rights_record_t *rights,
                             double elapsed_seconds) {
    if (!msg || !rights) return;
    if (msg->state != MAG_STATE_IN_TRANSIT) return;

    nsigii_mag_consensus_t c = nsigii_spring_consensus(&msg->spring);
    if (c != MAG_CONSENSUS_MAYBE) return;

    /* Eze failed to resolve MAYBE within 1/e seconds */
    if (elapsed_seconds > NSIGII_DECAY_CONSTANT) {
        rights->constitutional_breach = true;
        rights->breach_timestamp      = current_unix_time();
        snprintf(rights->breach_reason, sizeof(rights->breach_reason),
                 "Eze failed to resolve MAYBE state within 1/e (%.4f s) threshold",
                 NSIGII_DECAY_CONSTANT);
        printf("[DECAY] Constitutional breach detected for %s after %.3f s\n",
               msg->id, elapsed_seconds);
    }
}

/* ============================================================================
 * BRIDGE FUNCTIONS — network layer ↔ semantic layer
 * ============================================================================ */

nsigii_packet_t *nsigii_mag_to_packet(const nsigii_mag_message_t *msg) {
    if (!msg) return NULL;
    nsigii_packet_t *pkt = nsigii_packet_create();
    if (!pkt) return NULL;

    pkt->header.channel_id    = (uint8_t)msg->source;
    pkt->header.timestamp     = (uint64_t)(msg->encoded_at * 1e9);
    pkt->payload.content_length = (uint32_t)strnlen(msg->content, 1023);
    memcpy(pkt->payload.content, msg->content, pkt->payload.content_length);

    /* Embed LTE forensic sig in human_rights_tag */
    strncpy(pkt->verification.human_rights_tag,
            msg->collapse_sig,
            sizeof(pkt->verification.human_rights_tag) - 1);

    pkt->verification.rwx_flags = RWX_FULL;   /* Full permissions at encode */

    return pkt;
}

nsigii_mag_message_t *nsigii_mag_from_packet(const nsigii_packet_t *pkt,
                                              nsigii_pole_t source_pole) {
    if (!pkt) return NULL;
    nsigii_mag_message_t *msg = calloc(1, sizeof(nsigii_mag_message_t));
    if (!msg) return NULL;

    generate_id(msg->id, sizeof(msg->id));
    memcpy(msg->content, pkt->payload.content,
           pkt->payload.content_length < 1023
           ? pkt->payload.content_length : 1023);

    msg->source     = source_pole;
    msg->state      = MAG_STATE_IN_TRANSIT;   /* Received from network layer */
    msg->encoded_at = (double)pkt->header.timestamp / 1e9;
    msg->is_replay  = false;
    msg->packet_ref = (nsigii_packet_t *)pkt;

    /* Default spring — stiffness from consensus threshold */
    msg->spring.force     = 1.0;
    msg->spring.stiffness = (double)CONSENSUS_THRESHOLD;
    msg->spring.extension = nsigii_full_extension(&msg->spring);

    strncpy(msg->collapse_sig,
            pkt->verification.human_rights_tag,
            sizeof(msg->collapse_sig) - 1);

    return msg;
}
