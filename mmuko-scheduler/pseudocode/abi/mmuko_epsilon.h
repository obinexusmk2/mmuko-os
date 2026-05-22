// ============================================================
// MMUKO_EPSILON.H — Epsilon Corruption Lattice Layer
// ABI: _MMUKO32 / _MMUKO64
// Module: EPSILON_LAYER (mmuko-epsilon § MODULE 2)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// Date:    2026-05-23
// Status:  PSEUDOCODE — not yet compilable
// ============================================================
//
// PURPOSE:
//   Brings the Epsilon Corruption Lattice into the MMUKO ABI.
//   The scheduler does not merely allocate CPU — it governs access
//   to resources. When governance channels can be corrupted, the
//   epsilon layer detects structural violations in the lattice.
//
// MAPS FROM PYTHON:
//   EpsilonStateDetector → mmuko_epsilon_detector_t + ABI functions
//
// REFERENCE:
//   "The Epsilon Corruption Lattice" (Okpala, Dec 2025)
//   github.com/obinexus/corruption-lattice
//
// CORRUPTION AWARENESS STATES S (Definition 7):
//   ⊥         BOTTOM      — zero awareness (naivety)
//   --        NEGATIVE    — explicit corruption detection
//   ++        POSITIVE    — perceived legitimacy (surface fair)
//   ++/--     DUAL        — bicultural: surface claim + explicit proof
//   ε         EPSILON     — pure hidden/unknown corruption
//   ++/ε      HIDDEN_POS  — surface compliance + hidden exclusion
//   --/ε      HIDDEN_NEG  — explicit + deeper unknown layers
//   >         TOP         — complete corruption omniscience
//
// THEOREMS EMBEDDED IN ABI:
//   T1 Complement Violation:  no s' exists: s ∧ s' = ⊥ AND s ∨ s' = >
//   T2 Non-Distributivity:    E ∧ (C ∨ X) ≠ (E ∧ C) ∨ (E ∧ X)
//
// INTERACTION WITH OTHER HEADERS:
//   mmuko_connect.h   — calibration vector (ByteState[]) is epsilon input
//   mmuko_collapse.h  — promise state feeds certainty computation
//   mmuko_scheduler.h — governance channels CH_0/CH_1/CH_2 call epsilon
//   mmuko.h           — process rights (MMUKO_RIGHTS_*) map to lattice dimensions
//
// ============================================================

#ifndef MMUKO_EPSILON_H
#define MMUKO_EPSILON_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if !defined(_MMUKO32) && !defined(_MMUKO64)
    #error "mmuko_epsilon.h: define _MMUKO32 or _MMUKO64 before including"
#endif

// Pull ABI macros if not already defined by mmuko_collapse.h
#ifndef MMUKO_ABI
    #if defined(_MMUKO32)
        typedef uint32_t mmuko_ptr_t;
        #define MMUKO_ALIGN  __attribute__((packed, aligned(4)))
        #define MMUKO_ABI    __attribute__((cdecl))
    #else
        typedef uint64_t mmuko_ptr_t;
        #define MMUKO_ALIGN  __attribute__((packed, aligned(8)))
        #define MMUKO_ABI    __attribute__((sysv_abi))
    #endif
#endif

#ifndef MMUKO_EXPORT
    #define MMUKO_EXPORT
#endif

// ─────────────────────────────────────────────────────────────
// SECTION 1: CORRUPTION AWARENESS STATE
//   Ordered lattice S with 8 elements.
//   Partial order: ⊥ < {--, ++, ε} < {++/--, ++/ε, --/ε} < >
// ─────────────────────────────────────────────────────────────

typedef enum mmuko_epsilon_state {
    MMUKO_EPS_BOTTOM        = 0x00,   // ⊥  zero awareness
    MMUKO_EPS_NEGATIVE      = 0x01,   // -- explicit detection
    MMUKO_EPS_POSITIVE      = 0x02,   // ++ perceived legitimacy
    MMUKO_EPS_DUAL          = 0x03,   // ++/-- bicultural
    MMUKO_EPS_EPSILON       = 0x04,   // ε  pure hidden
    MMUKO_EPS_HIDDEN_POS    = 0x05,   // ++/ε UK institutional model
    MMUKO_EPS_HIDDEN_NEG    = 0x06,   // --/ε explicit + deep unknown
    MMUKO_EPS_TOP           = 0x07,   // >  full omniscience
} mmuko_epsilon_state_t;

// Predicate: is ε (hidden layer) present in this state?
// ++/ε, --/ε, and ε itself all carry a hidden component
#define MMUKO_EPS_IS_HIDDEN(s) \
    ((s) == MMUKO_EPS_EPSILON || \
     (s) == MMUKO_EPS_HIDDEN_POS || \
     (s) == MMUKO_EPS_HIDDEN_NEG)

// Predicate: has the system claimed surface legitimacy?
// ++, ++/--, ++/ε all assert surface compliance
#define MMUKO_EPS_CLAIMS_LEGITIMATE(s) \
    ((s) == MMUKO_EPS_POSITIVE || \
     (s) == MMUKO_EPS_DUAL || \
     (s) == MMUKO_EPS_HIDDEN_POS)

// ─────────────────────────────────────────────────────────────
// SECTION 2: ENTRAPMENT KIND
//   Lattice operators (Definitions 12–15).
//   Stored as bitmask — multiple traps may be active simultaneously.
//   CIVIL_COLLAPSE = EXHAUSTION ∧ SILENCE ∧ ASSERTION
// ─────────────────────────────────────────────────────────────

typedef enum mmuko_entrapment_kind {
    MMUKO_TRAP_NONE           = 0x00,
    MMUKO_TRAP_IMPROBABILITY  = 0x01,   // hidden barriers despite stated eligibility
    MMUKO_TRAP_EXHAUSTION     = 0x02,   // temporal: victim resources degrade over time
    MMUKO_TRAP_LOOPBACK       = 0x04,   // circular referrals — no resolution path
    MMUKO_TRAP_SILENCE        = 0x08,   // communication denial (NOSIGNAL dominant)
    MMUKO_TRAP_ASSERTION      = 0x10,   // false claims of legitimacy (SIGNAL but excluded)
    MMUKO_TRAP_CIVIL_COLLAPSE = 0x20,   // triple: EXHAUSTION ∧ SILENCE ∧ ASSERTION
} mmuko_entrapment_kind_t;

// Bitmask of zero or more active entrapment algorithms
typedef uint8_t mmuko_entrapment_mask_t;

// Convenience test
#define MMUKO_TRAP_IS_CIVIL_COLLAPSE(mask) \
    (((mask) & MMUKO_TRAP_EXHAUSTION) && \
     ((mask) & MMUKO_TRAP_SILENCE)    && \
     ((mask) & MMUKO_TRAP_ASSERTION))

// ─────────────────────────────────────────────────────────────
// SECTION 3: CALIBRATION INPUT REFERENCE
//   The epsilon layer receives its input as a classified vector
//   from mmuko_connect.h (Receiver::calibration_vector).
//   We mirror the ByteState enum here to avoid circular includes.
// ─────────────────────────────────────────────────────────────

typedef enum mmuko_eps_cal_state {
    MMUKO_EPS_CAL_NOISE    = 0,   // matches MMUKO_BYTE_NOISE
    MMUKO_EPS_CAL_NONOISE  = 1,   // matches MMUKO_BYTE_NONOISE
    MMUKO_EPS_CAL_SIGNAL   = 2,   // matches MMUKO_BYTE_SIGNAL
    MMUKO_EPS_CAL_NOSIGNAL = 3,   // matches MMUKO_BYTE_NOSIGNAL
} mmuko_eps_cal_state_t;

// ─────────────────────────────────────────────────────────────
// SECTION 4: TIMELINE ENTRY
//   Temporal sequence of (event, outcome) for exhaustion/loopback.
//   Maps: list[tuple[str, bool]] → mmuko_timeline_entry_t[]
// ─────────────────────────────────────────────────────────────

#define MMUKO_EPS_EVENT_LEN    32u
#define MMUKO_EPS_TIMELINE_MAX 64u

typedef struct {
    char    event[MMUKO_EPS_EVENT_LEN];   // e.g. "housing_application"
    bool    outcome;                       // true = success, false = rejection
    uint8_t _pad[3];
    double  timestamp;                     // when this event occurred
} mmuko_timeline_entry_t;

// ─────────────────────────────────────────────────────────────
// SECTION 5: CORRUPTION VECTOR — detection result
//   Maps: CorruptionVector dataclass → C struct
//   The fingerprint is SHA-256[:16] of the awareness state + ratios.
//   It can be verified against the calibration fingerprint from
//   mmuko_connect_verify() — if they match, same evidence base.
// ─────────────────────────────────────────────────────────────

#define MMUKO_EPS_FINGERPRINT_LEN  17u   // 16 hex chars + NUL
#define MMUKO_EPS_DESC_LEN         256u

typedef struct MMUKO_ALIGN mmuko_corruption_vector {
    mmuko_epsilon_state_t     awareness_state;   // detected lattice position
    bool                      is_hidden;          // ε present in state
    float                     certainty;          // [0.0, 1.0] confidence
    mmuko_entrapment_mask_t   entrapment;         // bitmask of active traps
    uint8_t                   _pad1[2];
    char                      fingerprint[MMUKO_EPS_FINGERPRINT_LEN];
    uint8_t                   _pad2[3];
    char                      description[MMUKO_EPS_DESC_LEN];
    bool                      complement_violated;    // Theorem 1 outcome
    bool                      distributivity_violated; // Theorem 2 outcome
    uint8_t                   _pad3[6];
} mmuko_corruption_vector_t;

// ─────────────────────────────────────────────────────────────
// SECTION 6: EPSILON DETECTOR — stateful analysis node
//   Maps: EpsilonStateDetector dataclass → mmuko_epsilon_detector_t
// ─────────────────────────────────────────────────────────────

#define MMUKO_EPS_NODE_ID_LEN  16u

typedef struct MMUKO_ALIGN mmuko_epsilon_detector {
    char         node_id[MMUKO_EPS_NODE_ID_LEN];   // e.g. "ESD-001"
    float        min_signal_ratio;                  // default 0.50
    uint32_t     analysis_count;                    // lifetime analyze() calls
    mmuko_ptr_t  _internal;                         // implementation pointer (ABI-width)
} mmuko_epsilon_detector_t;

// ─────────────────────────────────────────────────────────────
// SECTION 7: ANALYZE INPUT
//   Full parameter set passed to mmuko_epsilon_analyze().
//   Maps: detector.analyze() kwargs → single struct arg (C ABI).
// ─────────────────────────────────────────────────────────────

#define MMUKO_EPS_VECTOR_MAX  512u

typedef struct {
    // Calibration vector (from mmuko_connect Receiver)
    const uint8_t*               calibration_vector;  // array of mmuko_eps_cal_state_t
    uint32_t                     vector_len;

    // Context flags
    bool  applicant_awareness;        // can applicant detect the pattern?
    bool  system_claims_legitimacy;   // does system assert fair process?
    bool  eligibility;                // applicant meets formal eligibility?
    bool  criteria_met;               // formal criteria satisfied?
    bool  insider_advantage;          // connected applicant present?
    uint8_t _pad[3];

    // Timeline (may be NULL — exhaustion/loopback require it)
    const mmuko_timeline_entry_t*  timeline;
    uint32_t                       timeline_len;
    uint32_t                       _reserved;
} mmuko_epsilon_input_t;

// ─────────────────────────────────────────────────────────────
// SECTION 8: ABI FUNCTION SIGNATURES
// ─────────────────────────────────────────────────────────────

// --- Detector lifecycle ---

MMUKO_EXPORT mmuko_epsilon_detector_t* MMUKO_ABI
mmuko_epsilon_create(const char* node_id, float min_signal_ratio);

MMUKO_EXPORT void MMUKO_ABI
mmuko_epsilon_destroy(mmuko_epsilon_detector_t* det);

// --- Core analysis ---

// Run complete corruption detection. Returns by value (struct copy).
// On _MMUKO32: caller allocates stack space; struct returned via hidden pointer.
// On _MMUKO64: struct small enough for register pair or SRET per SysV ABI.
MMUKO_EXPORT mmuko_corruption_vector_t MMUKO_ABI
mmuko_epsilon_analyze(mmuko_epsilon_detector_t* det,
                      const mmuko_epsilon_input_t* input);

// --- Individual theorem tests (exposed for unit verification) ---

// Test T1: Complement Violation
// Returns true if violation detected; writes human-readable reason
MMUKO_EXPORT bool MMUKO_ABI
mmuko_epsilon_test_complement(mmuko_epsilon_state_t applicant,
                              mmuko_epsilon_state_t system,
                              char* out_reason, size_t reason_len);

// Test T2: Distributivity Failure
// E ∧ (C ∨ X) ≠ (E ∧ C) ∨ (E ∧ X) when criteria met but outcome negative
MMUKO_EXPORT bool MMUKO_ABI
mmuko_epsilon_test_distributivity(bool eligibility, bool criteria_met,
                                  bool insider_advantage,
                                  char* out_reason, size_t reason_len);

// --- State & entrapment query helpers ---

MMUKO_EXPORT const char* MMUKO_ABI
mmuko_epsilon_state_name(mmuko_epsilon_state_t state);

MMUKO_EXPORT const char* MMUKO_ABI
mmuko_epsilon_trap_name(mmuko_entrapment_kind_t kind);

// Serialize entrapment mask to comma-separated string
MMUKO_EXPORT void MMUKO_ABI
mmuko_epsilon_trap_describe(mmuko_entrapment_mask_t mask,
                            char* out_buf, size_t buf_len);

// ─────────────────────────────────────────────────────────────
// SECTION 9: SCHEDULER GOVERNANCE HOOK
//   The epsilon layer intercepts governance channel decisions.
//   When a process's context carries a corruption vector, the
//   scheduler consults mmuko_epsilon_gate() before emitting
//   CH_0 (run), CH_1 (defer), or CH_2 (terminate).
//
//   Corruption present + CIVIL_COLLAPSE active → force CH_2
//   Corruption hidden (ε) + improbability → force CH_1 with audit
//   Surface only (++) → CH_0 permitted but flagged
// ─────────────────────────────────────────────────────────────

typedef enum mmuko_governance_channel {
    MMUKO_CH_0_OBSERVE  = 0,   // YES  — run, obligation observable
    MMUKO_CH_1_DEFER    = 1,   // MAYBE — defer, retry later
    MMUKO_CH_2_COLLAPSE = 2,   // NO   — terminate, obligation incoherent
} mmuko_governance_channel_t;

// Called by scheduler before promoting a process.
// Returns the governance channel the scheduler must use.
MMUKO_EXPORT mmuko_governance_channel_t MMUKO_ABI
mmuko_epsilon_gate(const mmuko_corruption_vector_t* cv,
                   mmuko_governance_channel_t proposed);

#endif // MMUKO_EPSILON_H
