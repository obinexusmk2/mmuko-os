// ============================================================
// MMUKO_COLLAPSE.H — Superposition Decay & BQP Promise Layer
// ABI: _MMUKO32 / _MMUKO64
// Module: COLLAPSE_LAYER (mmuko-collapse § MODULE 1)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// Date:    2026-05-23
// Status:  PSEUDOCODE — not yet compilable
// ============================================================
//
// PURPOSE:
//   Defines the ABI surface for superposition decay and BQP promise
//   resolution. A scheduler process does not simply "run" or "wait"
//   — it exists in a promise state with measurable coherence.
//   When coherence decays below threshold, the promise collapses.
//
// MAPS FROM PYTHON:
//   QuantumSuperpositionDecay v2.0 → C ABI structs + functions
//
// CORE EQUATIONS:
//   SD(t) = exp(-λ * (t - t_open))   coherence decay
//   RWX:   4W + 2R ≡ 1R              modular memory equivalence
//   BQP:   p >= 2/3 → RESOLVED       promise threshold
//          p <= 1/3 → REJECTED
//
// INTERACTION WITH OTHER HEADERS:
//   mmuko.h          — MMUKO_System, mmuko_pcb_t (system state)
//   mmuko_scheduler.h — scheduling modes trigger collapse checks
//   mmuko_epsilon.h  — epsilon layer reads promise state as signal
//   mmuko_connect.h  — calibration fingerprint shared with BQP verifier
//
// ============================================================

#ifndef MMUKO_COLLAPSE_H
#define MMUKO_COLLAPSE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// ─────────────────────────────────────────────────────────────
// ABI WIDTH SELECTION
//   _MMUKO32: 32-bit ABI  (i386, embedded, ringboot runtime)
//   _MMUKO64: 64-bit ABI  (x86_64, desktop, WSL)
// ─────────────────────────────────────────────────────────────

#if defined(_MMUKO32)
    typedef uint32_t mmuko_ptr_t;
    typedef uint32_t mmuko_addr_t;
    #define MMUKO_ALIGN     __attribute__((packed, aligned(4)))
    #define MMUKO_ABI       __attribute__((cdecl))
    #define MMUKO_PTR_SIZE  4u
#elif defined(_MMUKO64)
    typedef uint64_t mmuko_ptr_t;
    typedef uint64_t mmuko_addr_t;
    #define MMUKO_ALIGN     __attribute__((packed, aligned(8)))
    #define MMUKO_ABI       __attribute__((sysv_abi))
    #define MMUKO_PTR_SIZE  8u
#else
    #error "mmuko_collapse.h: define _MMUKO32 or _MMUKO64 before including"
#endif

// ─────────────────────────────────────────────────────────────
// SYMBOL VISIBILITY
// ─────────────────────────────────────────────────────────────

#ifdef MMUKO_BUILD_SHARED
    #define MMUKO_EXPORT __attribute__((visibility("default")))
    #define MMUKO_IMPORT __attribute__((visibility("default")))
#else
    #define MMUKO_EXPORT
    #define MMUKO_IMPORT
#endif

// ─────────────────────────────────────────────────────────────
// SECTION 1: PROMISE STATE
//   BQP classification of a scheduler obligation
//   Maps: PromiseState(Python) → mmuko_promise_state_t(C)
// ─────────────────────────────────────────────────────────────

typedef enum mmuko_promise_state {
    MMUKO_PROMISE_RESOLVED  = 0,   // p >= 2/3 : obligation fulfilled
    MMUKO_PROMISE_REJECTED  = 1,   // p <= 1/3 : obligation cannot be met
    MMUKO_PROMISE_PENDING   = 2,   // 1/3 < p < 2/3 : still in coherence window
    MMUKO_PROMISE_COLLAPSED = 3,   // coherence < min_coherence : decay complete
} mmuko_promise_state_t;

// ─────────────────────────────────────────────────────────────
// SECTION 2: COMPLEXITY CLASS
//   Runtime class of the promise problem.
//   Scheduler inemptive mode targets BQP — memory truth,
//   not clock time, drives the transition.
// ─────────────────────────────────────────────────────────────

typedef enum mmuko_complexity_class {
    MMUKO_CLASS_P      = 0,   // Deterministic polynomial
    MMUKO_CLASS_BPP    = 1,   // Randomized polynomial
    MMUKO_CLASS_BQP    = 2,   // Quantum polynomial (inemptive target)
    MMUKO_CLASS_PP     = 3,   // Probabilistic polynomial
    MMUKO_CLASS_PSPACE = 4,   // Polynomial space (multi-window chains)
} mmuko_complexity_class_t;

// ─────────────────────────────────────────────────────────────
// SECTION 3: SUPERPOSITION DECAY WINDOW
//   SD(t) = exp(-λ * (t - t_open))
//   Process stays schedulable within [t_open, t_decay].
//   Maps: SuperpositionDecay(Python) → mmuko_superposition_decay_t(C)
// ─────────────────────────────────────────────────────────────

typedef struct MMUKO_ALIGN mmuko_superposition_decay {
    double   psi_real;          // Re(ψ) — real amplitude component
    double   psi_imag;          // Im(ψ) — imaginary amplitude component
    double   t_open;            // window opens  (scheduler tick time)
    double   t_decay;           // window closes (hard deadline)
    double   decay_lambda;      // λ — decay rate (higher = faster collapse)
    double   coherence_min;     // minimum coherence before COLLAPSED
    uint32_t window_id;         // unique window ID (ties to PID)
    uint32_t _reserved;         // pad to 8-byte boundary on both ABI widths
} mmuko_superposition_decay_t;

// Coherence measurement result
typedef struct {
    double               coherence;    // [0.0, 1.0] remaining coherence
    mmuko_promise_state_t state;       // what the coherence implies
    float                confidence;   // probability weight at measurement time
    uint8_t              _pad[4];
} mmuko_coherence_result_t;

// ─────────────────────────────────────────────────────────────
// SECTION 4: RWX MEMORY — modular equivalence table
//   4W + 2R ≡ 1R : after threshold, coherent unified read
//   Maps: RWXMemory(Python) → mmuko_rwx_memory_t(C)
//
//   ABI NOTE: addr table width is ABI-dependent (mmuko_addr_t)
//   On _MMUKO32: each addr is 4 bytes → 256-byte table
//   On _MMUKO64: each addr is 8 bytes → 512-byte table
// ─────────────────────────────────────────────────────────────

#define MMUKO_RWX_MAX_ADDRS  64u

typedef enum { MMUKO_RWX_R = 'R', MMUKO_RWX_W = 'W', MMUKO_RWX_X = 'X' } mmuko_rwx_op_t;

typedef struct MMUKO_ALIGN mmuko_rwx_memory {
    mmuko_addr_t    addrs[MMUKO_RWX_MAX_ADDRS];   // address table (ABI width)
    uint8_t         ops[MMUKO_RWX_MAX_ADDRS];      // last op per address slot
    uint32_t        write_count;                   // accumulated W operations
    uint32_t        read_count;                    // accumulated R operations
    uint32_t        addr_count;                    // active address slots
    bool            equivalence_triggered;         // 4W+2R satisfied → coherent
    uint8_t         _pad[3];
} mmuko_rwx_memory_t;

// Equivalence thresholds (4W + 2R → 1R)
#define MMUKO_RWX_WRITE_THRESHOLD  4u
#define MMUKO_RWX_READ_THRESHOLD   2u

// ─────────────────────────────────────────────────────────────
// SECTION 5: BQP CIRCUIT
//   Q_n: n-qubit circuit → 1-bit output
//   H^⊗n creates full superposition; oracle inverts marked states
//   Maps: BQPCircuit(Python) → mmuko_bqp_circuit_t(C)
// ─────────────────────────────────────────────────────────────

#define MMUKO_BQP_MAX_QUBITS  6u
#define MMUKO_BQP_MAX_DIM     64u   // 2^6

typedef struct MMUKO_ALIGN mmuko_bqp_circuit {
    uint32_t   n_qubits;
    uint32_t   dim;                            // 2^n_qubits
    double     state_real[MMUKO_BQP_MAX_DIM];  // Re(amplitude[i])
    double     state_imag[MMUKO_BQP_MAX_DIM];  // Im(amplitude[i])
    double     threshold_yes;                  // a(n): accept threshold (default 2/3)
    double     threshold_no;                   // b(n): reject threshold (default 1/3)
    bool       superposed;                     // true after hadamard applied
    uint8_t    _pad[7];
} mmuko_bqp_circuit_t;

// ─────────────────────────────────────────────────────────────
// SECTION 6: QUANTUM BURST PROTOCOL (QBP)
//   User-facing aggregate: decay windows + BQP circuits + RWX
//   Maps: QuantumBurstProtocol(Python) → mmuko_quantum_burst_t(C)
// ─────────────────────────────────────────────────────────────

#define MMUKO_QBP_MAX_WINDOWS   16u
#define MMUKO_QBP_MAX_CIRCUITS  16u

typedef struct MMUKO_ALIGN mmuko_quantum_burst {
    mmuko_complexity_class_t    target_class;
    uint32_t                    window_count;
    uint32_t                    circuit_count;
    mmuko_rwx_memory_t          rwx;
    mmuko_superposition_decay_t windows[MMUKO_QBP_MAX_WINDOWS];
    mmuko_bqp_circuit_t         circuits[MMUKO_QBP_MAX_CIRCUITS];
    mmuko_ptr_t                 dag_root;      // head of probabilistic DAG (ABI-width ptr)
    uint32_t                    node_count;    // DAG node count
    uint32_t                    _reserved;
} mmuko_quantum_burst_t;

// ─────────────────────────────────────────────────────────────
// SECTION 7: ABI FUNCTION SIGNATURES
// ─────────────────────────────────────────────────────────────

// --- Superposition decay ----

// Compute coherence at time t
MMUKO_EXPORT mmuko_coherence_result_t MMUKO_ABI
mmuko_collapse_coherence(const mmuko_superposition_decay_t* sd, double t);

// Measure promise state at time t
MMUKO_EXPORT mmuko_promise_state_t MMUKO_ABI
mmuko_collapse_measure(mmuko_superposition_decay_t* sd, double t,
                       float* out_confidence);

// Initialize a decay window for a scheduler process (pid binds to window_id)
MMUKO_EXPORT void MMUKO_ABI
mmuko_collapse_window_init(mmuko_superposition_decay_t* sd,
                           uint32_t pid,
                           double t_now,
                           double window_duration,
                           double decay_lambda);

// --- RWX memory ---

// W operation — state preparation
MMUKO_EXPORT void MMUKO_ABI
mmuko_rwx_write(mmuko_rwx_memory_t* mem, mmuko_addr_t addr);

// R operation — measurement with equivalence check
MMUKO_EXPORT mmuko_promise_state_t MMUKO_ABI
mmuko_rwx_read(mmuko_rwx_memory_t* mem, mmuko_addr_t addr);

// X operation — trigger decay window execution
MMUKO_EXPORT void MMUKO_ABI
mmuko_rwx_execute(mmuko_rwx_memory_t* mem, mmuko_addr_t addr);

// --- BQP circuit ---

// Apply H^⊗n — put all 2^n states in equal superposition
MMUKO_EXPORT void MMUKO_ABI
mmuko_bqp_hadamard(mmuko_bqp_circuit_t* circuit);

// Apply oracle: U_f|x⟩ = (-1)^f(x)|x⟩
// f_marks: bitmask of states to invert
MMUKO_EXPORT void MMUKO_ABI
mmuko_bqp_apply_oracle(mmuko_bqp_circuit_t* circuit, uint64_t f_marks);

// Measure against BQP promise thresholds
MMUKO_EXPORT mmuko_promise_state_t MMUKO_ABI
mmuko_bqp_measure(mmuko_bqp_circuit_t* circuit, float* out_prob, uint8_t* out_bit);

// --- Quantum burst protocol ---

// Create a QBP for a given complexity class
MMUKO_EXPORT mmuko_quantum_burst_t* MMUKO_ABI
mmuko_qbp_create(mmuko_complexity_class_t target);

MMUKO_EXPORT void MMUKO_ABI
mmuko_qbp_destroy(mmuko_quantum_burst_t* qbp);

// Map linear equations → probabilistic DAG, write addresses to RWX
MMUKO_EXPORT uint32_t MMUKO_ABI
mmuko_qbp_map_linear(mmuko_quantum_burst_t* qbp,
                     const char* const* equations, uint32_t eq_count);

// Execute all DAG nodes in parallel BQP circuits
// Returns overall promise state (worst-case across nodes)
MMUKO_EXPORT mmuko_promise_state_t MMUKO_ABI
mmuko_qbp_execute(mmuko_quantum_burst_t* qbp, double t_now);

// Query a specific circuit's result
MMUKO_EXPORT mmuko_promise_state_t MMUKO_ABI
mmuko_qbp_query_node(const mmuko_quantum_burst_t* qbp,
                     uint32_t circuit_idx, float* out_coherence);

// ─────────────────────────────────────────────────────────────
// SECTION 8: SCHEDULER INTEGRATION HOOKS
//   These are called by mmuko_scheduler_tick() when
//   MMUKO_SCHED_INEMPTIVE mode is active.
//   The collapse layer gates process promotion and preemption.
// ─────────────────────────────────────────────────────────────

// Returns true if the process's promise is still coherent
// (i.e., scheduler should not preempt on memory-truth grounds)
MMUKO_EXPORT bool MMUKO_ABI
mmuko_collapse_is_coherent(const mmuko_quantum_burst_t* qbp,
                           uint32_t pid, double t_now);

// Called when a process is promoted from READY → RUNNING
// Opens a new decay window for the process
MMUKO_EXPORT void MMUKO_ABI
mmuko_collapse_on_schedule(mmuko_quantum_burst_t* qbp,
                           uint32_t pid, double t_now);

// Called when a process is preempted or terminates
// Collapses its decay window
MMUKO_EXPORT void MMUKO_ABI
mmuko_collapse_on_terminate(mmuko_quantum_burst_t* qbp, uint32_t pid);

#endif // MMUKO_COLLAPSE_H
