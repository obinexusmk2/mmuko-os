#ifndef MMUKO_TOWER_H
#define MMUKO_TOWER_H

// ============================================================
// MMUKO_TOWER.H — Tower of Hanoi Memory Domain Interface
// Module: TOWER_LIFECYCLE (mmuko-scheduler.pse § MODULE 1)
// ============================================================

#include "../mmuko.h"

#ifdef __cplusplus
extern "C" {
#endif

// Tower lifecycle
mmuko_tower_t* mmuko_tower_create(mmuko_segment_t seg, uint32_t capacity,
                                   uint64_t domain_size, const char* name);
void           mmuko_tower_destroy(mmuko_tower_t* tower);

// Hanoi constraint check: can proc be placed on tower?
int            mmuko_can_stack(mmuko_tower_t* tower, mmuko_pcb_t* proc);

// Migrate proc from src tower to dst tower (respects Hanoi constraint)
int            mmuko_migrate(mmuko_tower_t* src, mmuko_tower_t* dst, mmuko_pcb_t* proc);

#ifdef __cplusplus
}
#endif

#endif // MMUKO_TOWER_H
