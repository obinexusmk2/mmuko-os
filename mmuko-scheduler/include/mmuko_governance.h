#ifndef MMUKO_GOVERNANCE_H
#define MMUKO_GOVERNANCE_H

// ============================================================
// MMUKO_GOVERNANCE.H — CH_0/CH_1/CH_2 Governance Interface
// Module: GOVERNANCE_CHANNELS (mmuko-scheduler.pse § MODULE 4)
// ============================================================

#include "../mmuko.h"
#include "mmuko_tower.h"

#ifdef __cplusplus
extern "C" {
#endif

mmuko_channel_t mmuko_governance_check(mmuko_pcb_t* proc, mmuko_tower_t* target);
int             mmuko_governance_enforce(mmuko_pcb_t* proc, mmuko_channel_t ch);

#ifdef __cplusplus
}
#endif

#endif // MMUKO_GOVERNANCE_H
