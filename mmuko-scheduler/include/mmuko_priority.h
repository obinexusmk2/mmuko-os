#ifndef MMUKO_PRIORITY_H
#define MMUKO_PRIORITY_H

// ============================================================
// MMUKO_PRIORITY.H — Polar Priority Heap Interface
// Module: POLAR_PRIORITY_HEAP (mmuko-scheduler.pse § MODULE 3)
// ============================================================

#include "../mmuko.h"

#ifdef __cplusplus
extern "C" {
#endif

uint64_t mmuko_polar_priority(mmuko_pcb_t* p);
double   mmuko_bayesian_priority(mmuko_pcb_t* candidate, mmuko_tower_t* tower);
int      mmuko_compare_priority(mmuko_pcb_t* a, mmuko_pcb_t* b);

#ifdef __cplusplus
}
#endif

#endif // MMUKO_PRIORITY_H
