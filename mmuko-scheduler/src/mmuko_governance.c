// ============================================================
// MMUKO_GOVERNANCE.C — CH_0/CH_1/CH_2 Governance Channels
// Module:  GOVERNANCE_CHANNELS (mmuko-scheduler.pse § MODULE 4)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// ============================================================
//
// Separation of Concern: owns only RiftLang gating decisions.
// CH_0 = YES (observe/execute), CH_1 = MAYBE (defer+retry),
// CH_2 = NO (collapse/abort). No scheduling logic here.
//
// ============================================================

#include "../include/mmuko_governance.h"
#include <stdio.h>

// Forward declaration — Hanoi check from mmuko_tower module
int mmuko_can_stack(mmuko_tower_t* tower, mmuko_pcb_t* proc);

// ─────────────────────────────────────────────────────────────────────────────
// GOVERNANCE CHECK  — determine gating outcome for proc on target tower
// ─────────────────────────────────────────────────────────────────────────────

mmuko_channel_t mmuko_governance_check(mmuko_pcb_t* proc, mmuko_tower_t* target) {
    if (!proc || !target) return CH_2_COLLAPSE;

    // CH_0: YES — tower has space and Hanoi constraint satisfied
    uint64_t available = target->domain_size - target->used;
    if (available >= proc->token.token_memory && mmuko_can_stack(target, proc)) {
        return CH_0_OBSERVE;
    }

    // CH_1: MAYBE — tower near capacity but not exhausted, process has dimensions
    double capacity_ratio = (double)target->used / (double)target->domain_size;
    if (capacity_ratio < 0.9 && proc->dimension_count > 0) {
        return CH_1_DEFER;
    }

    // CH_2: NO — tower full or rights/Hanoi mismatch
    return CH_2_COLLAPSE;
}

// ─────────────────────────────────────────────────────────────────────────────
// GOVERNANCE ENFORCE  — apply the gating decision to the process
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_governance_enforce(mmuko_pcb_t* proc, mmuko_channel_t ch) {
    switch (ch) {
        case CH_0_OBSERVE:
            proc->state = MMUKO_PROC_RUNNING;
            printf("[GOV] CH_0 OBSERVE: pid=%u executes immediately\n", proc->pid);
            return 0;

        case CH_1_DEFER:
            proc->state = MMUKO_PROC_BLOCKED;
            printf("[GOV] CH_1 DEFER: pid=%u blocked for retry\n", proc->pid);
            // In real kernel: set 60s retry timer
            return 1;

        case CH_2_COLLAPSE:
            proc->state = MMUKO_PROC_ZOMBIE;
            printf("[GOV] CH_2 COLLAPSE: pid=%u aborted (measurement forced)\n", proc->pid);
            return -1;
    }
    return -1;
}
