/*
 * tripolar_lexer.c
 * ─────────────────────────────────────────────────────────────────────────────
 * OBINexus NSIGII Complete System
 * Reverse Lexer — CISC Tripolar Signal → RISC Token Stream
 *
 * Author : Nnamdi Okeke / OBINexus Computing
 * Repo   : obinexusmk2/nsigii_ltcodec_showcase
 *
 * ─── MODULE RESPONSIBILITY ───────────────────────────────────────────────────
 *
 *   A CISC signal (tripolar_ptr_t) is a composite — it carries:
 *     - THERE_AND_THEN : provenance, stored authority (Eze)
 *     - HERE_AND_NOW   : active action, present state  (Obi)
 *     - WHEN_AND_WHERE : observer, verification flag   (Uche)
 *
 *   These three things are encoded simultaneously in the CISC unit.
 *   The reverse lexer DECOMPOSES this into three atomic RISC tokens,
 *   one per temporal frame — each processable by a single simple rule.
 *
 *   This is the CISC-to-RISC decomposition layer in the tripolar algebra.
 *
 * ─── WHY REVERSE LEXER ───────────────────────────────────────────────────────
 *
 *   A normal lexer tokenises text into symbols (forward: text → tokens).
 *   This is a REVERSE lexer: it takes a structured composite signal and
 *   strips it back to atomic constituents (signal → tokens).
 *
 *   The direction is reversed because the signal already exists as a
 *   composed unit. We need to find its atomic parts to:
 *     - verify each frame independently
 *     - apply per-frame repair rules
 *     - detect which frame is causing consensus failure
 *
 * ─── ANTI-RELAY / ANTI-REPLAY DEFENCE ───────────────────────────────────────
 *
 *   relay  attack = copy a signal and retransmit it unchanged
 *   replay attack = copy a signal and substitute its content
 *
 *   The lexer uses:
 *     - seq numbers   to detect replay (same seq = duplicate)
 *     - timestamp delta to detect relay (signal too old = stale copy)
 *     - channel_id match to detect frame spoofing
 *
 * ─── CONNECTED SUBSYSTEMS ────────────────────────────────────────────────────
 *
 *   1-NSIGII_Codec                → primary consumer (LTCodec encoding)
 *   4-NSIGII_Eze_RelayServer_in_C → relay detection uses decompose()
 *   5-NSIGII_Probe_System         → WHEN_AND_WHERE token verification
 *
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "tripolar_algebra.h"
#include <stdio.h>
#include <string.h>
#include <time.h>


/* ─────────────────────────────────────────────────────────────────────────────
 * INTERNAL: token weight calculation
 *
 * Weight = amplitude contribution from this frame.
 * A frame with amplitude 0 still contributes — its token is DESTROY,
 * not absent. Absence is not a valid state in a complete system.
 * ─────────────────────────────────────────────────────────────────────────────
 */

static float calculate_token_weight(tripolar_node_t *node) {
    if (!node) return 0.0f;
    /* Weight is amplitude scaled by frequency coherence */
    float w = node->amplitude * node->frequency;
    if (w < 0.0f) w = 0.0f;
    if (w > 1.0f) w = 1.0f;
    return w;
}

static repair_mode_t infer_repair(tripolar_node_t *node) {
    if (!node) return REPAIR_RECREATE;
    if (node->amplitude > TRIPOLAR_AMPLITUDE_MIN) return REPAIR_REBUILD;
    return REPAIR_RECREATE;
}

static uint64_t now_microseconds_lex(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000ULL) + (uint64_t)(ts.tv_nsec / 1000ULL);
}


/* ─────────────────────────────────────────────────────────────────────────────
 * DECOMPOSE — CISC → RISC token stream
 *
 * Takes a full tripolar_ptr_t and extracts three atomic tokens.
 * Runs the trident gate to compute the consensus verdict.
 * Each token is independently processable.
 *
 * This is the primary entry point for all subsystems that need to
 * inspect or verify a composite signal.
 * ─────────────────────────────────────────────────────────────────────────────
 */

tripolar_token_stream_t decompose(tripolar_ptr_t *cisc) {
    tripolar_token_stream_t stream;
    memset(&stream, 0, sizeof(stream));

    if (!cisc) {
        /* Null input → all tokens DESTROY, verdict DESTROY */
        for (int i = 0; i < 3; i++) {
            stream.tokens[i].frame  = (temporal_frame_t)i;
            stream.tokens[i].state  = SIGNAL_DESTROY;
            stream.tokens[i].weight = 0.0f;
            stream.tokens[i].repair = REPAIR_RECREATE;
        }
        stream.count   = 3;
        stream.verdict = SIGNAL_DESTROY;
        return stream;
    }

    tripolar_node_t *nodes[3] = {
        &cisc->there_and_then,
        &cisc->here_and_now,
        &cisc->when_and_where
    };

    /* Extract one token per frame */
    for (int i = 0; i < 3; i++) {
        stream.tokens[i].frame  = nodes[i]->frame;
        stream.tokens[i].state  = nodes[i]->state;
        stream.tokens[i].weight = calculate_token_weight(nodes[i]);
        stream.tokens[i].repair = infer_repair(nodes[i]);
    }

    stream.count   = 3;
    stream.verdict = trident_gate(cisc);

    return stream;
}


/* ─────────────────────────────────────────────────────────────────────────────
 * RECOMPOSE — RISC token stream → CISC tripolar_ptr
 *
 * Inverse of decompose(). Takes a token stream and reconstructs a
 * tripolar_ptr_t with each frame's state restored from the tokens.
 *
 * Used by the RECREATE path after tripolar_memory.c has loaded
 * a stored pattern into the THERE_AND_THEN frame.
 * ─────────────────────────────────────────────────────────────────────────────
 */

tripolar_ptr_t recompose(tripolar_token_stream_t *stream) {
    tripolar_ptr_t ptr;
    tripolar_ptr_init(&ptr);

    if (!stream || stream->count < 3) {
        /* Incomplete stream → leave all frames in RENEW */
        return ptr;
    }

    tripolar_node_t *nodes[3] = {
        &ptr.there_and_then,
        &ptr.here_and_now,
        &ptr.when_and_where
    };

    for (size_t i = 0; i < 3 && i < stream->count; i++) {
        tripolar_token_t *tok = &stream->tokens[i];
        /* Find the matching node by frame */
        for (int j = 0; j < 3; j++) {
            if (nodes[j]->frame == tok->frame) {
                nodes[j]->state     = tok->state;
                nodes[j]->amplitude = tok->weight;
                nodes[j]->timestamp = now_microseconds_lex();
                break;
            }
        }
    }

    ptr.consensus = stream->verdict;
    return ptr;
}


/* ─────────────────────────────────────────────────────────────────────────────
 * RELAY/REPLAY DETECTION
 *
 * relay  = same signal copied and retransmitted (channel attack)
 * replay = copied signal with content swapped   (content attack)
 *
 * Returns true if the signal should be rejected.
 * ─────────────────────────────────────────────────────────────────────────────
 */

#define RELAY_STALE_THRESHOLD_US  50000ULL   /* 50ms — signal too old */
#define SEQ_WINDOW                128        /* sequence window size  */

bool detect_relay(tripolar_ptr_t *incoming,
                  tripolar_ptr_t *reference,
                  uint32_t last_seen_seq) {
    if (!incoming || !reference) return true;

    /* Replay check: seq number already seen */
    if (incoming->seq <= last_seen_seq) {
        fprintf(stderr,
            "[TRIPOLAR LEXER] REPLAY ATTACK detected: "
            "seq %u <= last %u\n",
            incoming->seq, last_seen_seq);
        return true;
    }

    /* Relay check: timestamp too old relative to reference */
    uint64_t now  = now_microseconds_lex();
    uint64_t sent = incoming->here_and_now.timestamp;
    if (sent > 0 && (now - sent) > RELAY_STALE_THRESHOLD_US) {
        fprintf(stderr,
            "[TRIPOLAR LEXER] RELAY ATTACK detected: "
            "signal age %llu us exceeds threshold\n",
            (unsigned long long)(now - sent));
        return true;
    }

    /* Channel ID mismatch — frame spoofing */
    if (incoming->there_and_then.channel_id !=
        reference->there_and_then.channel_id) {
        fprintf(stderr,
            "[TRIPOLAR LEXER] FRAME SPOOF detected: "
            "channel mismatch %u vs %u\n",
            incoming->there_and_then.channel_id,
            reference->there_and_then.channel_id);
        return true;
    }

    return false;
}


/* ─────────────────────────────────────────────────────────────────────────────
 * PRINT TOKEN STREAM (debug / logging)
 * ─────────────────────────────────────────────────────────────────────────────
 */

void print_token_stream(const tripolar_token_stream_t *stream) {
    if (!stream) return;
    printf("─── Tripolar Token Stream ───────────────────────────\n");
    for (size_t i = 0; i < stream->count; i++) {
        const tripolar_token_t *t = &stream->tokens[i];
        printf("  Token[%zu]  frame=%-24s  state=%-8s  weight=%.3f  repair=%s\n",
               i,
               temporal_frame_str(t->frame),
               signal_state_str(t->state),
               t->weight,
               repair_mode_str(t->repair));
    }
    printf("  Verdict: %s\n", signal_state_str(stream->verdict));
    printf("─────────────────────────────────────────────────────\n");
}
