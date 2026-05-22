#ifndef MMUKO_REBALANCER_H
#define MMUKO_REBALANCER_H

// ============================================================
// MMUKO_REBALANCER.H — Dimensional Game Theory Rebalancer
// Module: DIMENSIONAL_REBALANCER (mmuko-scheduler.pse § MODULE 6)
// ============================================================

#include "../mmuko.h"

#ifdef __cplusplus
extern "C" {
#endif

// Detect strategic imbalance: returns count of imbalanced towers
int mmuko_detect_strategic_imbalance(mmuko_tower_t* towers[], uint32_t num_towers);

// Rebalance towers to restore Nash equilibrium
int mmuko_rebalance_dimensions(mmuko_tower_t* towers[], uint32_t num_towers,
                                uint32_t proc_count);

#ifdef __cplusplus
}
#endif

#endif // MMUKO_REBALANCER_H
