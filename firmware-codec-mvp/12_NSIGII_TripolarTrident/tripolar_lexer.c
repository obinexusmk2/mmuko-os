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
 *   The reverse lexer DECOMPOSES this into three atomic RISC tokens,
 *   one per temporal frame — each processable by a single simple rule.
 *
 * ─── ANTI-RELAY / ANTI-REPLAY DEFENCE ───────────────────────────────────────
 *
 *   relay  attack = copy a signal and retransmit unchanged
 *   replay attack = copy a signal and substitute content
 *
 *   Defence: seq numbers + timestamp delta + channel_id match
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
 * CROSS-PLATFORM TIMESTAMP
 * ─────────────────────────────────────────────────────────────────────────────
 */

static uint64_t now_microseconds_lex(void) {
    return (uint64_t)time(NULL) * 1000000ULL;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * INTERNAL: token weight + repair inference
 * ─────────────────────────────────────────────────────────────────────────────
 */

static float calculate_token_weight(tripolar_node_t *node) {
    if (!node) return 0.0f;
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


/* ─────────────────────────────────────────────────────────────────────────────
 * DECOMPOSE — CISC → RISC token stream
 * ─────────────────────────────────────────────────────────────────────────────
 */

tripolar_token_stream_t decompose(tripolar_ptr_t *cisc) {
    tripolar_token_stream_t stream;
    memset(&stream, 0, sizeof(stream));

    if (!cisc) {
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
 * ─────────────────────────────────────────────────────────────────────────────
 */

tripolar_ptr_t recompose(tripolar_token_stream_t *stream) {
    tripolar_ptr_t ptr;
    tripolar_ptr_init(&ptr);

    if (!stream || stream->count < 3) {
        return ptr;
    }

    tripolar_node_t *nodes[3] = {
        &ptr.there_and_then,
        &ptr.here_and_now,
        &ptr.when_and_where
    };

    for (size_t i = 0; i < 3 && i < stream->count; i++) {
        tripolar_token_t *tok = &stream->tokens[i];
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
 * ─────────────────────────────────────────────────────────────────────────────
 */

#define RELAY_STALE_THRESHOLD_US  50000ULL   /* 50ms — signal too old */

bool detect_relay(tripolar_ptr_t *incoming,
                  tripolar_ptr_t *reference,
                  uint32_t last_seen_seq) {
    if (!incoming || !reference) return true;

    if (incoming->seq <= last_seen_seq) {
        fprintf(stderr,
            "[TRIPOLAR LEXER] REPLAY ATTACK: seq %u <= last %u\n",
            incoming->seq, last_seen_seq);
        return true;
    }

    uint64_t now  = now_microseconds_lex();
    uint64_t sent = incoming->here_and_now.timestamp;
    if (sent > 0 && (now - sent) > RELAY_STALE_THRESHOLD_US) {
        fprintf(stderr,
            "[TRIPOLAR LEXER] RELAY ATTACK: signal age %llu us exceeds threshold\n",
            (unsigned long long)(now - sent));
        return true;
    }

    if (incoming->there_and_then.channel_id !=
        reference->there_and_then.channel_id) {
        fprintf(stderr,
            "[TRIPOLAR LEXER] FRAME SPOOF: channel mismatch %u vs %u\n",
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
