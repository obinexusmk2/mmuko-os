#ifndef MMUKO_SCHED_MODES_H
#define MMUKO_SCHED_MODES_H

// ============================================================
// MMUKO_SCHED_MODES.H — Scheduling Modes Interface
// Module: SCHEDULING_MODES (mmuko-scheduler.pse § MODULE 5)
// ============================================================

#include "../mmuko.h"
#include "mmuko_tower.h"

#ifdef __cplusplus
extern "C" {
#endif

// Preemptive: high-priority incoming interrupts running process
int mmuko_schedule_preemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                               mmuko_pcb_t* incoming);

// Non-preemptive: process runs to completion or I/O block
int mmuko_schedule_non_preemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                                   mmuko_pcb_t* incoming);

// Inemptive: memory-preemptive, time-nonpreemptive (staged allocation)
int mmuko_schedule_inemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                              mmuko_inemptive_buffer_t* buffer, mmuko_pcb_t* incoming);

// Resolve a staged inemptive buffer when memory becomes available
int mmuko_inemptive_resolve(mmuko_inemptive_buffer_t* buffer, mmuko_tower_t* running);

#ifdef __cplusplus
}
#endif

#endif // MMUKO_SCHED_MODES_H
