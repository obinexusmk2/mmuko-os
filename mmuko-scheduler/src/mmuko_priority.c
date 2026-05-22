// ============================================================
// MMUKO_PRIORITY.C — Polar Priority Heap Implementation
// Module:  POLAR_PRIORITY_HEAP (mmuko-scheduler.pse § MODULE 3)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// ============================================================
//
// Separation of Concern: owns only priority computation.
// No process creation, no tower management, no scheduling logic.
//
// Formulas (from OBINexus Dimensional Game Theory):
//   D        = a + b + d + e   (rights dimension sum)
//   need_kb  = token_memory / 1KB
//   n        = floor(log2(need_kb)) + 1  (exponential scaling)
//   priority = D × n × need_kb          (strategic value)
//
//   Bayesian resolver: P(A|B) = (likelihood × prior) / marginal
//   Combined score:    polar_priority × bayesian_confidence
//
// ============================================================

#include "../include/mmuko_priority.h"
#include <inttypes.h>
#include <math.h>

// ─────────────────────────────────────────────────────────────────────────────
// POLAR PRIORITY  (core scheduling weight)
// ─────────────────────────────────────────────────────────────────────────────

uint64_t mmuko_polar_priority(mmuko_pcb_t* p) {
    if (!p) return 0;

    uint64_t D       = (uint64_t)MMUKO_DIM_D(p);
    uint64_t need_kb = p->token.token_memory / MMUKO_KB(1);
    if (need_kb == 0) need_kb = 1;

    // Exponential scaling: H = p^n, base p = D, exponent n = log2(need_kb)+1
    uint64_t n    = 1;
    uint64_t temp = need_kb;
    while (temp > 1) { temp >>= 1; n++; }

    uint64_t priority = D * n * need_kb;
    p->strategic_value = priority;
    return priority;
}

// ─────────────────────────────────────────────────────────────────────────────
// BAYESIAN PRIORITY  (conflict resolution for tower slot contention)
//
// P(A)   = prior  = memory_need / tower_size
// P(B|A) = likelihood = available_capacity / tower_size
// P(B)   = marginal   = 0.5 (uniform prior)
// P(A|B) = posterior  = likelihood × prior / marginal
// ─────────────────────────────────────────────────────────────────────────────

double mmuko_bayesian_priority(mmuko_pcb_t* candidate, mmuko_tower_t* tower) {
    if (!candidate || !tower) return 0.0;

    double prior = (double)candidate->token.token_memory / (double)tower->domain_size;
    if (prior > 1.0) prior = 1.0;

    double available  = (double)(tower->domain_size - tower->used);
    double likelihood = available / (double)tower->domain_size;
    if (likelihood < 0.0) likelihood = 0.0;

    double marginal   = 0.5;
    double posterior  = (likelihood * prior) / marginal;
    if (posterior > 1.0) posterior = 1.0;

    candidate->bayesian_conf = posterior;
    return (double)mmuko_polar_priority(candidate) * posterior;
}

// ─────────────────────────────────────────────────────────────────────────────
// COMPARE PRIORITY  (max-heap ordering)
// Returns: 1 if a > b, -1 if a < b, 0 if equal
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_compare_priority(mmuko_pcb_t* a, mmuko_pcb_t* b) {
    if (!a || !b) return 0;

    uint64_t pa = mmuko_polar_priority(a);
    uint64_t pb = mmuko_polar_priority(b);
    if (pa > pb) return  1;
    if (pa < pb) return -1;

    // Tie-break 1: higher rights dimension wins
    uint64_t da = (uint64_t)MMUKO_DIM_D(a);
    uint64_t db = (uint64_t)MMUKO_DIM_D(b);
    if (da > db) return  1;
    if (da < db) return -1;

    // Tie-break 2: lower PID wins (FIFO fairness)
    return (a->pid < b->pid) ? 1 : -1;
}
