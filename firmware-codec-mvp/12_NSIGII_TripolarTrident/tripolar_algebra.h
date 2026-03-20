/*
 * tripolar_algebra.h
 * ─────────────────────────────────────────────────────────────────────────────
 * OBINexus NSIGII Complete System
 * Tripolar Pointer Algebra — Master Type System
 *
 * Author : Nnamdi Okeke / OBINexus Computing
 * Repo   : obinexusmk2/nsigii_ltcodec_showcase
 * License: OBINexus Constitutional Tier 3C
 *
 * ─── ARCHITECTURE OVERVIEW ───────────────────────────────────────────────────
 *
 * This header is the shared type contract for all NSIGII subsystems.
 * Every module (Codec, Relay, Probe, Channel, Memory, Drone) operates
 * on the same three fundamental constructs defined here:
 *
 *   1. SIGNAL STATES     — trinary logic: YES / NO / NIL
 *   2. TEMPORAL FRAMES   — tripolar identity: Eze / Obi / Uche
 *   3. ENZYME OPERATIONS — state transitions: Create / Destroy / Renew
 *
 * ─── TRIPOLAR IDENTITY MAPPING ───────────────────────────────────────────────
 *
 *   THERE_AND_THEN  → Eze  (past authority, stored pattern, governance)
 *   HERE_AND_NOW    → Obi  (present action, active signal, heart)
 *   WHEN_AND_WHERE  → Uche (observer, conditional, void pointer, verifier)
 *
 * ─── SIGNAL PHYSICS MAPPING ──────────────────────────────────────────────────
 *
 *   AM (Amplitude Modulation) → HERE_AND_NOW  → REBUILD path
 *   FM (Frequency Modulation) → THERE_AND_THEN → RECREATE path
 *   RENEW state               → WHEN_AND_WHERE → arbitration
 *
 * ─── LIAR PARADOX GUARD ──────────────────────────────────────────────────────
 *
 *   Any self-referential contradictory signal cannot stabilize as
 *   CREATE or DESTROY. It is forced into RENEW — held in the verification
 *   loop rather than causing undefined behaviour or system fault.
 *   The void * (Uche pointer) is the type-level implementation of this guard.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 */

#ifndef TRIPOLAR_ALGEBRA_H
#define TRIPOLAR_ALGEBRA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


/* ═══════════════════════════════════════════════════════════════════════════
 * SECTION 1: TRINARY SIGNAL STATES
 * ═══════════════════════════════════════════════════════════════════════════
 * Maps directly to the trident gate output: YES / NO / NIL.
 * NIL is not failure — it is the RENEW state, the system thinking.
 */

typedef enum {
    SIGNAL_CREATE  =  1,    /* YES — coherent, stable, verified signal       */
    SIGNAL_DESTROY =  0,    /* NO  — null, invalidated, or rejected signal   */
    SIGNAL_RENEW   = -1     /* NIL — unstable, noisy, or under active repair */
} signal_state_t;

/* Human-readable labels for logging / debug output */
static inline const char *signal_state_str(signal_state_t s) {
    switch (s) {
        case SIGNAL_CREATE:  return "CREATE";
        case SIGNAL_DESTROY: return "DESTROY";
        case SIGNAL_RENEW:   return "RENEW";
        default:             return "UNKNOWN";
    }
}


/* ═══════════════════════════════════════════════════════════════════════════
 * SECTION 2: TEMPORAL FRAMES
 * ═══════════════════════════════════════════════════════════════════════════
 * The three axes of the tripolar algebra.
 * Every signal carries provenance (past), action (present), and
 * observer state (conditional future).
 */

typedef enum {
    FRAME_THERE_AND_THEN = 0,   /* Eze  — stored pattern, past authority     */
    FRAME_HERE_AND_NOW   = 1,   /* Obi  — active signal, present action      */
    FRAME_WHEN_AND_WHERE = 2    /* Uche — observer, verifier, void pointer   */
} temporal_frame_t;

static inline const char *temporal_frame_str(temporal_frame_t f) {
    switch (f) {
        case FRAME_THERE_AND_THEN: return "THERE_AND_THEN (Eze)";
        case FRAME_HERE_AND_NOW:   return "HERE_AND_NOW (Obi)";
        case FRAME_WHEN_AND_WHERE: return "WHEN_AND_WHERE (Uche)";
        default:                   return "UNKNOWN_FRAME";
    }
}


/* ═══════════════════════════════════════════════════════════════════════════
 * SECTION 3: ENZYME / REPAIR OPERATIONS
 * ═══════════════════════════════════════════════════════════════════════════
 * Two recovery paths inside the RENEW state.
 *
 *   REBUILD  — structure survives, use AM (boost amplitude).
 *              Signal is weak but present. Repair from current state.
 *
 *   RECREATE — structure is lost, use FM (shift frequency + load pattern).
 *              Load stored pattern from THERE_AND_THEN (Eze memory).
 */

typedef enum {
    REPAIR_REBUILD  = 0,    /* AM path  — signal degraded but recoverable    */
    REPAIR_RECREATE = 1     /* FM path  — signal lost, restore from memory   */
} repair_mode_t;

static inline const char *repair_mode_str(repair_mode_t r) {
    switch (r) {
        case REPAIR_REBUILD:  return "REBUILD (AM)";
        case REPAIR_RECREATE: return "RECREATE (FM)";
        default:              return "UNKNOWN_REPAIR";
    }
}


/* ═══════════════════════════════════════════════════════════════════════════
 * SECTION 4: TRIPOLAR NODE
 * ═══════════════════════════════════════════════════════════════════════════
 * The atomic unit. One frame carrier.
 * The void *payload is the Uche pointer — typeless observer.
 * If payload == (void*)node itself → liar paradox guard triggers.
 */

typedef struct tripolar_node {
    temporal_frame_t    frame;          /* which temporal axis this node sits on */
    signal_state_t      state;          /* current CREATE / DESTROY / RENEW      */
    void               *payload;        /* Uche void pointer — typeless observer */
    float               amplitude;      /* AM component — signal strength [0,1]  */
    float               frequency;      /* FM component — signal structure [0,1] */
    uint64_t            timestamp;      /* last verification time (microseconds) */
    uint32_t            channel_id;     /* which A/B channel this node is on     */
    bool                self_ref_guard; /* true if liar paradox was detected     */
} tripolar_node_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * SECTION 5: TRIPOLAR POINTER (full algebra unit — CISC composite)
 * ═══════════════════════════════════════════════════════════════════════════
 * The composite signal type. Carries all three frames simultaneously.
 * The reverse lexer decomposes this into three atomic RISC tokens.
 * The trident gate runs majority consensus on the three frame states.
 */

typedef struct tripolar_ptr {
    tripolar_node_t     there_and_then;     /* Eze  frame — past pattern     */
    tripolar_node_t     here_and_now;       /* Obi  frame — present action   */
    tripolar_node_t     when_and_where;     /* Uche frame — observer/verify  */
    signal_state_t      consensus;          /* trident gate result           */
    uint32_t            seq;                /* sequence number (anti-replay) */
    bool                is_broadcast;       /* true = broadcast to all nodes */
} tripolar_ptr_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * SECTION 6: TRIPOLAR TOKEN (atomic RISC output of reverse lexer)
 * ═══════════════════════════════════════════════════════════════════════════
 */

typedef struct {
    temporal_frame_t    frame;          /* which frame this token came from  */
    signal_state_t      state;          /* the frame's local signal state    */
    float               weight;         /* amplitude contribution [0,1]      */
    repair_mode_t       repair;         /* which repair path if RENEW        */
} tripolar_token_t;

typedef struct {
    tripolar_token_t    tokens[3];      /* one per temporal frame            */
    size_t              count;          /* always 3 for a valid decomposition*/
    signal_state_t      verdict;        /* trident gate consensus result     */
} tripolar_token_stream_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * SECTION 7: CHANNEL DESCRIPTOR
 * ═══════════════════════════════════════════════════════════════════════════
 * Maps to the A0–A9 / B0–B9 channel algebra from the radio spec.
 *
 *   CHANNEL_TRANSMIT   → send only    (A-prefix, channel 0 reserved)
 *   CHANNEL_RECEIVE    → listen only  (B-prefix)
 *   CHANNEL_BROADCAST  → all nodes    (B* wildcard)
 *   CHANNEL_VERIFY     → probe / hold (USCI-001 stateless probe)
 */

typedef enum {
    CHANNEL_TRANSMIT  = 0,
    CHANNEL_RECEIVE   = 1,
    CHANNEL_BROADCAST = 2,
    CHANNEL_VERIFY    = 3
} channel_role_t;

typedef struct {
    uint32_t        id;             /* channel number 0–9                   */
    char            prefix;         /* 'A' or 'B'                           */
    channel_role_t  role;
    bool            is_blocked;     /* true if jammed / relay attack active */
    uint32_t        fallback_id;    /* channel to route to if blocked       */
} channel_desc_t;


/* ═══════════════════════════════════════════════════════════════════════════
 * SECTION 8: SYSTEM THRESHOLDS (tunable constants)
 * ═══════════════════════════════════════════════════════════════════════════
 */

#define TRIPOLAR_AMPLITUDE_MIN      0.3f    /* below = REBUILD triggers      */
#define TRIPOLAR_FREQUENCY_MIN      0.2f    /* below = RECREATE triggers     */
#define TRIPOLAR_NOISE_THRESHOLD    0.7f    /* above = noise countermeasure  */
#define TRIPOLAR_DISTORTION_MAX     0.6f    /* above = FM shift triggers     */
#define TRIPOLAR_HOLD_TIME_MS       12      /* push-to-talk verification ms  */
#define TRIPOLAR_CHANNEL_MAX        10      /* A0–A9 or B0–B9                */


/* ═══════════════════════════════════════════════════════════════════════════
 * SECTION 9: FUNCTION DECLARATIONS
 * (implementations in tripolar_engine.c and tripolar_lexer.c)
 * ═══════════════════════════════════════════════════════════════════════════
 */

/* Engine — state transitions and control */
signal_state_t  trident_gate(tripolar_ptr_t *ptr);
repair_mode_t   determine_repair_mode(tripolar_node_t *node);
signal_state_t  control_signal(tripolar_node_t *node,
                                float noise_level,
                                float distortion);
signal_state_t  enzyme_cycle(tripolar_ptr_t *ptr,
                              float noise_level,
                              float distortion);
void            tripolar_node_init(tripolar_node_t *node,
                                   temporal_frame_t frame,
                                   uint32_t channel_id);
void            tripolar_ptr_init(tripolar_ptr_t *ptr);

/* Lexer — CISC decomposition to RISC token stream */
tripolar_token_stream_t decompose(tripolar_ptr_t *cisc);
tripolar_ptr_t          recompose(tripolar_token_stream_t *stream);
bool                    is_self_referential(tripolar_node_t *node);
void                    print_token_stream(const tripolar_token_stream_t *stream);


#ifdef __cplusplus
}
#endif

#endif /* TRIPOLAR_ALGEBRA_H */
