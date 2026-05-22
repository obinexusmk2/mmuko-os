// ============================================================
// MMUKO_CONNECT.H — NSIGII Calibration Tripartite Stream
// ABI: _MMUKO32 / _MMUKO64
// Module: CONNECT_LAYER (mmuko-connect § MODULE 3)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// Date:    2026-05-23
// Status:  PSEUDOCODE — not yet compilable
// ============================================================
//
// PURPOSE:
//   The connection layer is the entry gate of MMUKO.
//   No process communicates without first passing through the
//   tripartite calibration stream: Transmitter emits, Receiver
//   classifies, Verifier signs. Only when SIGNAL is dominant
//   does the calibration fingerprint become the shared proof
//   that both sides are speaking the same language.
//
// MAPS FROM PYTHON:
//   CalibrationSession + Transmitter/Receiver/Verifier → C ABI
//
// CORE CONCEPTS:
//   Calibration tuple: C = (NOISE, NONOISE, SIGNAL, NOSIGNAL)
//   Tripartite stream: S = (TRANSMITTER, RECEIVER, VERIFIER)
//   Planar elimination: 2D (entropy × structure) → 1D ByteState
//   Preamble: 0xAA 0x55 (reuses NSIGII boot signature)
//   Fingerprint: SHA-256[:16] of serialized ByteState vector
//
// INTERACTION WITH OTHER HEADERS:
//   mmuko_epsilon.h  — calibration vector is the epsilon input
//   mmuko_collapse.h — fingerprint cross-checks BQP verifier
//   mmuko_scheduler.h — session result gates process admission
//   mmuko.h          — system boot_complete must be true before connect
//
// ============================================================

#ifndef MMUKO_CONNECT_H
#define MMUKO_CONNECT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if !defined(_MMUKO32) && !defined(_MMUKO64)
    #error "mmuko_connect.h: define _MMUKO32 or _MMUKO64 before including"
#endif

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
// SECTION 1: BYTE STATE
//   Calibration tuple C = (NOISE, NONOISE, SIGNAL, NOSIGNAL)
//   Every byte window in a stream is classified into exactly one.
//   Maps: ByteState(Python IntEnum) → mmuko_byte_state_t(C enum)
// ─────────────────────────────────────────────────────────────

typedef enum mmuko_byte_state {
    MMUKO_BYTE_NOISE    = 0,   // high entropy — cannot be structured
    MMUKO_BYTE_NONOISE  = 1,   // entropy excluded — clean but silent
    MMUKO_BYTE_SIGNAL   = 2,   // intentional pattern — distinguishable structure
    MMUKO_BYTE_NOSIGNAL = 3,   // null/silence — no transmission event
} mmuko_byte_state_t;

// Number of distinct states
#define MMUKO_BYTE_STATE_COUNT  4u

// ─────────────────────────────────────────────────────────────
// SECTION 2: CALIBRATION TUPLE
//   Planar elimination thresholds.
//   Maps: CalibrationTuple dataclass → mmuko_calibration_tuple_t
// ─────────────────────────────────────────────────────────────

typedef struct MMUKO_ALIGN mmuko_calibration_tuple {
    float    noise_threshold;    // P(entropy) >= this → NOISE    (default 0.70)
    float    signal_threshold;   // structure  >= this → SIGNAL   (default 0.60)
    uint32_t silence_window;     // consecutive nulls → NOSIGNAL  (default 8)
    uint32_t window_size;        // bytes per classification window (default 16)
} mmuko_calibration_tuple_t;

// Default calibration tuple values
#define MMUKO_CAL_DEFAULT_NOISE_THRESH    0.70f
#define MMUKO_CAL_DEFAULT_SIGNAL_THRESH   0.60f
#define MMUKO_CAL_DEFAULT_SILENCE_WINDOW  8u
#define MMUKO_CAL_DEFAULT_WINDOW_SIZE     16u

// Preamble bytes: 0xAA 0x55 (boosts structure score +0.30)
#define MMUKO_CONNECT_PREAMBLE_0   0xAAu
#define MMUKO_CONNECT_PREAMBLE_1   0x55u
#define MMUKO_CONNECT_PREAMBLE_LEN 2u

// ─────────────────────────────────────────────────────────────
// SECTION 3: CALIBRATION EVENT
//   Event-agnostic message passed through the tripartite stream.
//   Maps: CalibrationEvent dataclass → mmuko_calibration_event_t
//
//   ABI NOTE: payload buffer is fixed-size at MMUKO_EVENT_PAYLOAD_MAX.
//   On _MMUKO32: struct is 4-byte aligned, total ~560 bytes.
//   On _MMUKO64: struct is 8-byte aligned, total ~568 bytes.
// ─────────────────────────────────────────────────────────────

#define MMUKO_EVENT_PAYLOAD_MAX  512u
#define MMUKO_NODE_ID_LEN        16u

typedef enum mmuko_event_kind {
    MMUKO_EVENT_CONNECT     = 0,   // open channel
    MMUKO_EVENT_DISCONNECT  = 1,   // close channel
    MMUKO_EVENT_DATA        = 2,   // framed payload
    MMUKO_EVENT_RECALIBRATE = 3,   // force re-classification
} mmuko_event_kind_t;

typedef struct MMUKO_ALIGN mmuko_calibration_event {
    mmuko_event_kind_t   kind;
    uint32_t             payload_len;                        // valid bytes in payload[]
    uint8_t              payload[MMUKO_EVENT_PAYLOAD_MAX];   // framed byte stream
    char                 node_id[MMUKO_NODE_ID_LEN];         // originating node
    double               timestamp;                          // seconds since epoch
} mmuko_calibration_event_t;

// ─────────────────────────────────────────────────────────────
// SECTION 4: TRANSMITTER
//   Emits preamble-framed byte streams.
//   Preamble 0xAA 0x55 prepended to every DATA event payload.
//   Maps: Transmitter dataclass → mmuko_transmitter_t(C)
// ─────────────────────────────────────────────────────────────

typedef struct MMUKO_ALIGN mmuko_transmitter {
    char     node_id[MMUKO_NODE_ID_LEN];   // e.g. "TX-001"
    uint32_t emit_count;                   // DATA events emitted
    bool     connected;                    // channel open?
    uint8_t  _pad[3];
} mmuko_transmitter_t;

// ─────────────────────────────────────────────────────────────
// SECTION 5: RECEIVER
//   Receives events and classifies each payload via the
//   CalibrationTuple. Maintains a rolling classification vector.
//   Maps: Receiver dataclass → mmuko_receiver_t(C)
//
//   The calibration_vector[] is the 1D projection of the stream —
//   one ByteState per window_size bytes.
//
//   ABI NOTE: vector is fixed at MMUKO_VECTOR_MAX entries.
//   On _MMUKO32 with 4 bytes/entry: 16KB for vector.
//   On _MMUKO64: same (ByteState is 4 bytes either way).
// ─────────────────────────────────────────────────────────────

#define MMUKO_VECTOR_MAX  4096u

typedef struct MMUKO_ALIGN mmuko_receiver {
    char                     node_id[MMUKO_NODE_ID_LEN];
    mmuko_calibration_tuple_t calibrator;
    mmuko_byte_state_t        vector[MMUKO_VECTOR_MAX];   // rolling classification
    uint32_t                  vector_len;                  // entries used
    bool                      connected;
    uint8_t                   _pad[3];
    uint32_t                  dominant_cache;             // cached dominant ByteState
    bool                      dominant_valid;             // cache is current?
    uint8_t                   _pad2[3];
} mmuko_receiver_t;

// ─────────────────────────────────────────────────────────────
// SECTION 6: VERIFIER
//   Validates that SIGNAL is the dominant state in the vector.
//   Produces SHA-256[:16] fingerprint as the shared calibration proof.
//   Both TX and RX must agree on this fingerprint for connection
//   to be considered constitutionally sound.
//
//   Maps: Verifier dataclass → mmuko_verifier_t(C)
// ─────────────────────────────────────────────────────────────

#define MMUKO_CONNECT_FINGERPRINT_LEN  17u   // 16 hex chars + NUL

typedef struct MMUKO_ALIGN mmuko_verifier {
    char    node_id[MMUKO_NODE_ID_LEN];
    char    last_fingerprint[MMUKO_CONNECT_FINGERPRINT_LEN];
    bool    last_valid;                     // last verify() result
    uint8_t _pad[6];
    uint32_t verify_count;
} mmuko_verifier_t;

// ─────────────────────────────────────────────────────────────
// SECTION 7: CALIBRATION SESSION
//   Orchestrates TX → RX → VRF in sequence.
//   Maps: CalibrationSession class → mmuko_connect_session_t(C)
//
//   ABI NOTE: session embeds all three nodes by value.
//   Total size on _MMUKO32: ~(TX~24 + RX~16520 + VRF~48) ≈ 16KB
//   Total size on _MMUKO64: same structure, 8-byte aligned.
//   If size is a concern in embedded contexts, pointers may be used.
// ─────────────────────────────────────────────────────────────

typedef struct MMUKO_ALIGN mmuko_connect_session {
    mmuko_transmitter_t  tx;
    mmuko_receiver_t     rx;
    mmuko_verifier_t     vrf;
    uint32_t             run_count;        // times session.run() called
    bool                 last_connected;
    uint8_t              _pad[3];
} mmuko_connect_session_t;

// Result of mmuko_connect_run()
typedef struct {
    bool                  connected;
    char                  fingerprint[MMUKO_CONNECT_FINGERPRINT_LEN];
    mmuko_byte_state_t    dominant;
    uint32_t              vector_len;
    uint8_t               _pad[4];
} mmuko_connect_result_t;

// ─────────────────────────────────────────────────────────────
// SECTION 8: ABI FUNCTION SIGNATURES
// ─────────────────────────────────────────────────────────────

// --- Session lifecycle ---

// Allocates session on heap and initializes TX/RX/VRF with given IDs
MMUKO_EXPORT mmuko_connect_session_t* MMUKO_ABI
mmuko_connect_create(const char* tx_id, const char* rx_id, const char* vrf_id);

MMUKO_EXPORT void MMUKO_ABI
mmuko_connect_destroy(mmuko_connect_session_t* session);

// --- Transmitter operations ---

// Emit framed DATA event: prepends 0xAA 0x55 to payload
MMUKO_EXPORT mmuko_calibration_event_t MMUKO_ABI
mmuko_transmit(mmuko_transmitter_t* tx,
               const uint8_t* payload, uint32_t payload_len);

// Emit CONNECT event (channel open)
MMUKO_EXPORT mmuko_calibration_event_t MMUKO_ABI
mmuko_transmit_connect(mmuko_transmitter_t* tx);

// Emit DISCONNECT event (channel close)
MMUKO_EXPORT mmuko_calibration_event_t MMUKO_ABI
mmuko_transmit_disconnect(mmuko_transmitter_t* tx);

// --- Receiver operations ---

// Receive event, classify payload, append to rolling vector
// Returns number of new ByteState entries written to out_states[]
MMUKO_EXPORT uint32_t MMUKO_ABI
mmuko_receive(mmuko_receiver_t* rx,
              const mmuko_calibration_event_t* event,
              mmuko_byte_state_t* out_states, uint32_t out_max);

// Return dominant ByteState (most frequent in vector)
MMUKO_EXPORT mmuko_byte_state_t MMUKO_ABI
mmuko_receive_dominant(mmuko_receiver_t* rx);

// --- Verifier operations ---

// Validate calibration vector; write fingerprint to out_fp (len >= 17)
MMUKO_EXPORT bool MMUKO_ABI
mmuko_verify(mmuko_verifier_t* vrf, const mmuko_receiver_t* rx,
             char* out_fp, size_t fp_len);

// --- Full session run ---

// Runs: connect → emit each payload → verify → disconnect
// payloads[i] has length payload_lens[i]
MMUKO_EXPORT mmuko_connect_result_t MMUKO_ABI
mmuko_connect_run(mmuko_connect_session_t* session,
                  const uint8_t* const* payloads,
                  const uint32_t*       payload_lens,
                  uint32_t              payload_count);

// --- Utilities ---

MMUKO_EXPORT const char* MMUKO_ABI
mmuko_byte_state_name(mmuko_byte_state_t state);

// Planar elimination — classify a single window (exposed for unit testing)
MMUKO_EXPORT mmuko_byte_state_t MMUKO_ABI
mmuko_calibrate_window(const mmuko_calibration_tuple_t* cal,
                       const uint8_t* window, uint32_t window_len);

// ─────────────────────────────────────────────────────────────
// SECTION 9: SCHEDULER ADMISSION GATE
//   A process may not be admitted to the scheduler until its
//   connection session has verified to SIGNAL-dominant.
//   This ties the calibration fingerprint to the PID.
// ─────────────────────────────────────────────────────────────

// Bind a verified session fingerprint to a process ID.
// Returns false if session is not connected (dominant != SIGNAL).
MMUKO_EXPORT bool MMUKO_ABI
mmuko_connect_admit_process(const mmuko_connect_result_t* result,
                            uint32_t pid,
                            char* out_bound_fp, size_t fp_len);

// Check whether a PID's bound fingerprint is still valid
// (used by scheduler on each tick to detect stale connections)
MMUKO_EXPORT bool MMUKO_ABI
mmuko_connect_check_pid(uint32_t pid, const char* expected_fp);

#endif // MMUKO_CONNECT_H
