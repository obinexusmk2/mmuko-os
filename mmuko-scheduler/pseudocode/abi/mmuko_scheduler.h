// ============================================================
// MMUKO_SCHEDULER.H — Public Scheduler ABI Header
// ABI: _MMUKO32 / _MMUKO64
// Module: SCHEDULER_LOOP (mmuko-scheduler.pse § MODULE 7)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// Date:    2026-05-23
// Status:  PSEUDOCODE — public ABI facade
// ============================================================
//
// PURPOSE:
//   This is the single public include a user application needs
//   to drive the MMUKO scheduler. It exposes the scheduling
//   surface without requiring knowledge of internal modules
//   (tower, process, priority, governance, sched_modes, rebalancer).
//
//   It acts as the ABI facade over:
//     mmuko_scheduler.c       — SCHEDULER_LOOP
//     src/mmuko_tower.c       — TOWER_LIFECYCLE
//     src/mmuko_process.c     — PROCESS_LIFECYCLE
//     src/mmuko_priority.c    — POLAR_PRIORITY_HEAP
//     src/mmuko_governance.c  — GOVERNANCE_CHANNELS
//     src/mmuko_sched_modes.c — SCHEDULING_MODES
//     src/mmuko_rebalancer.c  — DIMENSIONAL_REBALANCER
//
// INTERACTION WITH OTHER ABI HEADERS:
//   mmuko_connect.h  — connection must be admitted before scheduling
//   mmuko_epsilon.h  — governance channels consult epsilon gate
//   mmuko_collapse.h — inemptive mode uses collapse coherence check
//   mmuko.h          — MMUKO_System, mmuko_pcb_t, MMUKO_Rights
//
// ============================================================

#ifndef MMUKO_SCHEDULER_H
#define MMUKO_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#if !defined(_MMUKO32) && !defined(_MMUKO64)
    #error "mmuko_scheduler.h: define _MMUKO32 or _MMUKO64 before including"
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
// SECTION 1: SCHEDULING MODES
//   Three modes defined by mmuko-scheduler.pse § MODULE 5.
//   INEMPTIVE is the MMUKO-specific third mode:
//     memory-truth preemptive, NOT clock preemptive
// ─────────────────────────────────────────────────────────────

typedef enum mmuko_sched_mode {
    MMUKO_SCHED_PREEMPTIVE       = 0,   // CPU time slice triggers preemption
    MMUKO_SCHED_NON_PREEMPTIVE   = 1,   // process runs until it yields
    MMUKO_SCHED_INEMPTIVE        = 2,   // memory-truth triggers preemption
} mmuko_sched_mode_t;

// ─────────────────────────────────────────────────────────────
// SECTION 2: PROCESS RIGHTS
//   Rights dimensions used in polar priority formula:
//   priority = D × n × need_kb
//   where D = sum of active rights dimensions
// ─────────────────────────────────────────────────────────────

#define MMUKO_RIGHTS_CIVIL       (1u << 0)   // civil rights dimension
#define MMUKO_RIGHTS_HUMAN       (1u << 1)   // human rights dimension
#define MMUKO_RIGHTS_DISABILITY  (1u << 2)   // disability rights dimension
#define MMUKO_RIGHTS_ENVIRON     (1u << 3)   // environmental rights dimension
#define MMUKO_RIGHTS_ALL         (0x0Fu)     // all four dimensions active

// ─────────────────────────────────────────────────────────────
// SECTION 3: PROCESS CONTROL BLOCK (public view)
//   User creates processes via mmuko_proc_create().
//   Internal fields (tower slot, disk index) are opaque.
// ─────────────────────────────────────────────────────────────

typedef enum mmuko_proc_state {
    MMUKO_PROC_READY      = 0,
    MMUKO_PROC_RUNNING    = 1,
    MMUKO_PROC_BLOCKED    = 2,
    MMUKO_PROC_TERMINATED = 3,
    MMUKO_PROC_ZOMBIE     = 4,
} mmuko_proc_state_t;

// Opaque PCB handle — internal layout lives in mmuko.h
// User sees only the fields needed to submit a process
typedef struct mmuko_pcb mmuko_pcb_t;

// ─────────────────────────────────────────────────────────────
// SECTION 4: SYSTEM HANDLE
//   MMUKO_System is defined in mmuko.h.
//   Declared here as an opaque pointer for applications that
//   include mmuko_scheduler.h without including mmuko.h.
// ─────────────────────────────────────────────────────────────

typedef struct MMUKO_System MMUKO_System;

// ─────────────────────────────────────────────────────────────
// SECTION 5: SCHEDULER TICK RESULT
//   Returned by mmuko_scheduler_tick() so the user can inspect
//   what happened in this quantum without reading internal state.
// ─────────────────────────────────────────────────────────────

typedef struct {
    uint32_t pid_scheduled;      // PID promoted to RUNNING (0 = none)
    uint32_t pid_terminated;     // PID completed this tick (0 = none)
    bool     rebalance_occurred; // dimensional rebalancer fired
    bool     epsilon_gated;      // epsilon layer altered a CH decision
    bool     collapse_checked;   // inemptive coherence check ran
    uint8_t  _pad[5];
    uint64_t turnaround_ms;      // turnaround of terminated process (0 = none)
} mmuko_tick_result_t;

// ─────────────────────────────────────────────────────────────
// SECTION 6: RUN RESULT
//   Summary returned by mmuko_scheduler_run()
// ─────────────────────────────────────────────────────────────

typedef struct {
    uint64_t ticks_executed;
    uint32_t processes_completed;
    uint32_t processes_zombie;
    bool     nash_equilibrium_held;  // true if no rebalance was needed
    uint8_t  _pad[3];
} mmuko_run_result_t;

// ─────────────────────────────────────────────────────────────
// SECTION 7: ABI FUNCTION SIGNATURES
// ─────────────────────────────────────────────────────────────

// --- System lifecycle ---

MMUKO_EXPORT MMUKO_System* MMUKO_ABI
mmuko_system_create(uint32_t memory_bytes, uint32_t max_processes);

MMUKO_EXPORT void MMUKO_ABI
mmuko_system_destroy(MMUKO_System* sys);

// --- Process lifecycle ---

// arrival_time=0 means "now"
// need_kb  — how many KB this process requires (need dimension)
// rights   — bitmask of MMUKO_RIGHTS_* (dimension D)
MMUKO_EXPORT mmuko_pcb_t* MMUKO_ABI
mmuko_proc_create(uint64_t arrival_time, uint32_t rights,
                  uint32_t burst_time, uint32_t need_kb);

MMUKO_EXPORT void MMUKO_ABI
mmuko_proc_destroy(mmuko_pcb_t* pcb);

// Admit process into the READY tower (MMUKO_SEG_STACK)
// Returns false if connect fingerprint is not valid
MMUKO_EXPORT bool MMUKO_ABI
mmuko_proc_admit(MMUKO_System* sys, mmuko_pcb_t* pcb,
                 const char* connect_fingerprint);

// Register process in the system process table
MMUKO_EXPORT bool MMUKO_ABI
mmuko_proc_register(MMUKO_System* sys, mmuko_pcb_t* pcb);

// --- Scheduler control ---

MMUKO_EXPORT int MMUKO_ABI
mmuko_scheduler_init(MMUKO_System* sys, mmuko_sched_mode_t mode);

MMUKO_EXPORT mmuko_tick_result_t MMUKO_ABI
mmuko_scheduler_tick(MMUKO_System* sys);

MMUKO_EXPORT mmuko_run_result_t MMUKO_ABI
mmuko_scheduler_run(MMUKO_System* sys, uint64_t max_ticks);

// --- Priority query ---

// Compute polar priority for a process
// priority = D × n × need_kb  (n = log2(need_kb)+1)
MMUKO_EXPORT uint64_t MMUKO_ABI
mmuko_priority_of(const mmuko_pcb_t* pcb);

// --- Governance channel query ---

// Ask what channel the scheduler would assign to this process right now
MMUKO_EXPORT int MMUKO_ABI
mmuko_governance_query(MMUKO_System* sys, const mmuko_pcb_t* pcb);

// --- Status output ---

MMUKO_EXPORT void MMUKO_ABI
mmuko_print_system_status(const MMUKO_System* sys);

MMUKO_EXPORT void MMUKO_ABI
mmuko_print_pcb(const mmuko_pcb_t* pcb);

#endif // MMUKO_SCHEDULER_H
