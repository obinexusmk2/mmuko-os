#ifndef MMUKO_PROCESS_H
#define MMUKO_PROCESS_H

// ============================================================
// MMUKO_PROCESS.H — PCB Lifecycle Interface
// Module: PROCESS_LIFECYCLE (mmuko-scheduler.pse § MODULE 2)
// ============================================================

#include "../mmuko.h"
#include "mmuko_tower.h"

#ifdef __cplusplus
extern "C" {
#endif

mmuko_pcb_t* mmuko_proc_create(uint32_t pid, mmuko_rights_t rights,
                                uint64_t burst_time, uint64_t memory_need);
void         mmuko_proc_destroy(mmuko_pcb_t* proc);
int          mmuko_proc_allocate(mmuko_pcb_t* proc, mmuko_tower_t* tower);

#ifdef __cplusplus
}
#endif

#endif // MMUKO_PROCESS_H
