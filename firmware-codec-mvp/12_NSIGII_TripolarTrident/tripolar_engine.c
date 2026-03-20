/*
 * tripolar_engine.c
 * ─────────────────────────────────────────────────────────────────────────────
 * OBINexus NSIGII Complete System
 * Enzyme State Machine + Trident Gate + AM/FM Signal Control
 *
 * Author : Nnamdi Okeke / OBINexus Computing
 * Repo   : obinexusmk2/nsigii_ltcodec_showcase
 *
 * ─── MODULE RESPONSIBILITY ───────────────────────────────────────────────────
 *
 *   This module is the runtime engine of the tripolar algebra.
 *   It governs three things:
 *
 *   1. TRIDENT GATE     — majority consensus across three temporal frames.
 *                         YES/YES/? → CREATE. NO/NO/? → DESTROY.
 *                         No majority → NIL → RENEW. NIL never crashes.
 *
 *   2. ENZYME TRANSITIONS — create / destroy / renew state machine.
 *                           RENEW has two internal paths:
 *                           REBUILD  (AM: structure survives)
 *                           RECREATE (FM: load from THERE_AND_THEN memory)
 *
 *   3. AM/FM CONTROL RULE — maps signal physics to repair decisions.
 *                           Noise    → boost amplitude (AM, HERE_AND_NOW)
 *                           Distortion → shift frequency (FM, THERE_AND_THEN)
 *                           Both degraded → full RENEW (WHEN_AND_WHERE)
 *
 * ─── CONNECTED SUBSYSTEMS ────────────────────────────────────────────────────
 *
 *   4-NSIGII_Eze_RelayServer_in_C  → calls trident_gate()
 *   6-NSIGII_Not_A_Toy             → calls control_signal()
 *   9-NSIGII_MagneticElectroRuntimeDrone → calls determine_repair_mode()
 *   11-NSIGII-MmukoFluid           → calls full enzyme cycle
 *
 * ─────────────────────────────────────────────────────────────────────────────
 */

#include "tripolar_algebra.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ─────────────────────────────────────────────────────────────────────────────
 * CROSS-PLATFORM TIMESTAMP
 *
 * Windows (MinGW/MSVC) does not expose CLOCK_MONOTONIC even with
 * _POSIX_C_SOURCE. We use QueryPerformanceCounter on Windows and
 * clock_gettime(CLOCK_MONOTONIC) on POSIX (Linux, macOS).
 * ─────────────────────────────────────────────────────────────────────────────
 */

static uint64_t now_microseconds(void) {
    return (uint64_t)time(NULL) * 1000000ULL;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * INITIALISATION
 * ─────────────────────────────────────────────────────────────────────────────
 */

void tripolar_node_init(tripolar_node_t *node,
                        temporal_frame_t frame,
                        uint32_t channel_id) {
    if (!node) return;
    memset(node, 0, sizeof(tripolar_node_t));
    node->frame      = frame;
    node->state      = SIGNAL_RENEW;    /* start in RENEW — not yet verified */
    node->payload    = NULL;
    node->amplitude  = 1.0f;
    node->frequency  = 1.0f;
    node->timestamp  = now_microseconds();
    node->channel_id = channel_id;
    node->self_ref_guard = false;
}

void tripolar_ptr_init(tripolar_ptr_t *ptr) {
    if (!ptr) return;
    tripolar_node_init(&ptr->there_and_then, FRAME_THERE_AND_THEN, 0);
    tripolar_node_init(&ptr->here_and_now,   FRAME_HERE_AND_NOW,   1);
    tripolar_node_init(&ptr->when_and_where, FRAME_WHEN_AND_WHERE, 2);
    ptr->consensus    = SIGNAL_RENEW;
    ptr->seq          = 0;
    ptr->is_broadcast = false;
}


/* ─────────────────────────────────────────────────────────────────────────────
 * LIAR PARADOX GUARD
 *
 * If the Uche void pointer points back at its own node,
 * the signal is self-referential and cannot stabilise.
 * Classification: RENEW. No crash, no undefined behaviour.
 * ─────────────────────────────────────────────────────────────────────────────
 */

bool is_self_referential(tripolar_node_t *node) {
    if (!node) return false;
    if (node->payload == (void *)node) {
        node->self_ref_guard = true;
        node->state = SIGNAL_RENEW;
        return true;
    }
    return false;
}


/* ─────────────────────────────────────────────────────────────────────────────
 * TRIDENT GATE
 *
 * Runs majority consensus across all three temporal frames.
 *
 *   2+ CREATE  → SIGNAL_CREATE
 *   2+ DESTROY → SIGNAL_DESTROY
 *   No majority → SIGNAL_RENEW  (NIL gate — system must repair)
 *
 * This is the YES / NO / NIL gate from polybuild consensus.
 * NIL is structurally identical to the HOLD state in RIFT trinary logic.
 * ─────────────────────────────────────────────────────────────────────────────
 */

signal_state_t trident_gate(tripolar_ptr_t *ptr) {
    if (!ptr) return SIGNAL_RENEW;

    int yes = 0, no = 0, nil = 0;

    signal_state_t frames[3] = {
        ptr->there_and_then.state,
        ptr->here_and_now.state,
        ptr->when_and_where.state
    };

    for (int i = 0; i < 3; i++) {
        switch (frames[i]) {
            case SIGNAL_CREATE:  yes++; break;
            case SIGNAL_DESTROY: no++;  break;
            case SIGNAL_RENEW:   nil++; break;
        }
    }

    if (yes >= 2) {
        ptr->consensus = SIGNAL_CREATE;
    } else if (no >= 2) {
        ptr->consensus = SIGNAL_DESTROY;
    } else {
        ptr->consensus = SIGNAL_RENEW;     /* NIL wins — enter repair cycle */
    }

    return ptr->consensus;
}


/* ─────────────────────────────────────────────────────────────────────────────
 * REPAIR MODE SELECTOR
 *
 * Called when a node is in RENEW state.
 * Determines which repair path to take:
 *
 *   REBUILD  → amplitude still above threshold → AM path (HERE_AND_NOW)
 *              Structure survives. Boost signal, repair in place.
 *
 *   RECREATE → amplitude below threshold → FM path (THERE_AND_THEN)
 *              Structure lost. Shift frequency, load stored pattern.
 * ─────────────────────────────────────────────────────────────────────────────
 */

repair_mode_t determine_repair_mode(tripolar_node_t *node) {
    if (!node) return REPAIR_RECREATE;

    if (node->amplitude > TRIPOLAR_AMPLITUDE_MIN) {
        return REPAIR_REBUILD;
    }
    return REPAIR_RECREATE;
}


/* ─────────────────────────────────────────────────────────────────────────────
 * AM/FM CONTROL RULE
 *
 * Maps signal physics (noise, distortion) to enzyme state transitions.
 *
 *   noise_level  : 0.0 (clean) → 1.0 (severe external interference)
 *   distortion   : 0.0 (clean) → 1.0 (severe internal instability)
 *
 * ─── DECISION TABLE ──────────────────────────────────────────────────────────
 *
 *   Self-referential             → RENEW   (liar paradox guard)
 *   Noise ↑, signal coherent    → CREATE  (AM boost: push through noise)
 *   Distortion ↑ OR coherence ↓ → RENEW   (FM shift: find clean channel)
 *   Both degraded                → RENEW   (full enzyme repair cycle)
 *   All stable                   → CREATE  (no intervention needed)
 *
 * ─────────────────────────────────────────────────────────────────────────────
 */

signal_state_t control_signal(tripolar_node_t *node,
                               float noise_level,
                               float distortion) {
    if (!node) return SIGNAL_RENEW;

    /* Guard 1: liar paradox — self-referential payload */
    if (is_self_referential(node)) {
        fprintf(stderr,
            "[TRIPOLAR] LIAR GUARD triggered on channel %u frame %s\n",
            node->channel_id,
            temporal_frame_str(node->frame));
        return SIGNAL_RENEW;
    }

    /* Guard 2: completely dead signal */
    if (node->amplitude <= 0.0f && node->frequency <= 0.0f) {
        node->state = SIGNAL_DESTROY;
        return SIGNAL_DESTROY;
    }

    /* Case 1: external noise but signal still coherent → BOOST AMPLITUDE (AM) */
    if (noise_level > TRIPOLAR_NOISE_THRESHOLD &&
        node->amplitude > TRIPOLAR_AMPLITUDE_MIN) {
        node->amplitude = node->amplitude * 1.5f;
        if (node->amplitude > 1.0f) node->amplitude = 1.0f;  /* clamp */
        node->state     = SIGNAL_CREATE;
        node->timestamp = now_microseconds();
        return SIGNAL_CREATE;
    }

    /* Case 2: internal distortion or coherence loss → SHIFT FREQUENCY (FM) */
    if (distortion    > TRIPOLAR_DISTORTION_MAX ||
        node->frequency < TRIPOLAR_FREQUENCY_MIN) {
        node->frequency += 0.1f;
        if (node->frequency > 1.0f) node->frequency = 0.1f;  /* wrap channel */
        node->state     = SIGNAL_RENEW;
        node->timestamp = now_microseconds();
        return SIGNAL_RENEW;
    }

    /* Case 3: both noise AND distortion — full RENEW cycle */
    if (noise_level > TRIPOLAR_NOISE_THRESHOLD &&
        distortion  > TRIPOLAR_DISTORTION_MAX) {
        node->state     = SIGNAL_RENEW;
        node->timestamp = now_microseconds();
        return SIGNAL_RENEW;
    }

    /* Default: stable */
    node->state     = SIGNAL_CREATE;
    node->timestamp = now_microseconds();
    return SIGNAL_CREATE;
}


/* ─────────────────────────────────────────────────────────────────────────────
 * ENZYME CYCLE RUNNER
 *
 * Full create → [damage] → renew → [repair] → create loop.
 * Connects determine_repair_mode() to control_signal() in sequence.
 *
 * Returns final signal state after one repair attempt.
 * ─────────────────────────────────────────────────────────────────────────────
 */

signal_state_t enzyme_cycle(tripolar_ptr_t *ptr,
                             float noise_level,
                             float distortion) {
    if (!ptr) return SIGNAL_RENEW;

    /* Run control rule on each frame independently */
    control_signal(&ptr->there_and_then, noise_level, distortion);
    control_signal(&ptr->here_and_now,   noise_level, distortion);
    control_signal(&ptr->when_and_where, noise_level, distortion);

    /* Determine repair mode for each frame in RENEW */
    tripolar_node_t *frames[3] = {
        &ptr->there_and_then,
        &ptr->here_and_now,
        &ptr->when_and_where
    };

    for (int i = 0; i < 3; i++) {
        if (frames[i]->state == SIGNAL_RENEW) {
            repair_mode_t mode = determine_repair_mode(frames[i]);
            /*
             * REBUILD  → amplitude recovery in place (HERE_AND_NOW pattern)
             * RECREATE → frequency shift, caller must load THERE_AND_THEN
             *            (handled by tripolar_memory.c)
             */
            if (mode == REPAIR_REBUILD) {
                frames[i]->amplitude += 0.2f;
                if (frames[i]->amplitude > 1.0f) frames[i]->amplitude = 1.0f;
                if (frames[i]->amplitude > TRIPOLAR_AMPLITUDE_MIN) {
                    frames[i]->state = SIGNAL_CREATE;
                }
            }
            /* RECREATE: leave in RENEW — tripolar_memory.c will handle */
        }
    }

    /* Re-run trident gate after repair attempt */
    return trident_gate(ptr);
}
