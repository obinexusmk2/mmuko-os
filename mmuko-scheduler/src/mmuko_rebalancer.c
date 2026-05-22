// ============================================================
// MMUKO_REBALANCER.C — Dimensional Game Theory Rebalancer
// Module:  DIMENSIONAL_REBALANCER (mmuko-scheduler.pse § MODULE 6)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// ============================================================
//
// Separation of Concern: owns only strategic imbalance detection
// and Nash equilibrium restoration across memory towers.
// No process creation, no scheduling decisions.
//
// From OBINexus PDF: "Perfect games result in deterministic outcomes;
// deviations indicate strategic imbalances that can be algorithmically
// detected."
//
// ============================================================

#include "../include/mmuko_rebalancer.h"
#include "../include/mmuko_priority.h"
#include <stdio.h>
#include <math.h>

// ─────────────────────────────────────────────────────────────────────────────
// DETECT STRATEGIC IMBALANCE
// Any tower whose mean priority deviates > 30% from global mean
// is considered strategically imbalanced (minimax deviation).
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_detect_strategic_imbalance(mmuko_tower_t* towers[], uint32_t num_towers) {
    if (!towers || num_towers == 0) return -1;

    uint64_t total_priority = 0;
    uint32_t total_procs    = 0;

    for (uint32_t i = 0; i < num_towers; i++) {
        if (!towers[i]) continue;
        for (uint32_t j = 0; j < towers[i]->count; j++) {
            total_priority += mmuko_polar_priority(towers[i]->disks[j]);
            total_procs++;
        }
    }

    if (total_procs == 0) return 0; // empty — no imbalance

    double mean_priority   = (double)total_priority / (double)total_procs;
    int    imbalance_count = 0;

    for (uint32_t i = 0; i < num_towers; i++) {
        if (!towers[i] || towers[i]->count == 0) continue;
        uint64_t tower_sum = 0;
        for (uint32_t j = 0; j < towers[i]->count; j++) {
            tower_sum += mmuko_polar_priority(towers[i]->disks[j]);
        }
        double tower_mean = (double)tower_sum / (double)towers[i]->count;
        double deviation  = fabs(tower_mean - mean_priority) / mean_priority;

        if (deviation > 0.30) {
            printf("[REBALANCE] Imbalance in %s: mean=%.1f global=%.1f dev=%.2f%%\n",
                   towers[i]->name, tower_mean, mean_priority, deviation * 100.0);
            imbalance_count++;
        }
    }

    return imbalance_count;
}

// ─────────────────────────────────────────────────────────────────────────────
// REBALANCE DIMENSIONS
// Migrate highest-priority process from overloaded tower to
// underloaded tower, restoring Nash equilibrium.
// Respects Hanoi constraint at all times.
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_rebalance_dimensions(mmuko_tower_t* towers[], uint32_t num_towers,
                                uint32_t proc_count) {
    (void)proc_count;
    if (!towers || num_towers < 2) return -1;

    printf("\n[REBALANCE] Dimensional rebalancing started...\n");

    int imbalances = mmuko_detect_strategic_imbalance(towers, num_towers);
    if (imbalances == 0) {
        printf("[REBALANCE] System at Nash equilibrium — no rebalancing needed.\n");
        return 0;
    }

    // Find max-mean and min-mean towers
    int    max_idx = -1, min_idx = -1;
    double max_mean = 0.0, min_mean = 1e18;

    for (uint32_t i = 0; i < num_towers; i++) {
        if (!towers[i] || towers[i]->count == 0) continue;
        uint64_t tower_sum = 0;
        for (uint32_t j = 0; j < towers[i]->count; j++) {
            tower_sum += mmuko_polar_priority(towers[i]->disks[j]);
        }
        double tower_mean = (double)tower_sum / (double)towers[i]->count;
        if (tower_mean > max_mean) { max_mean = tower_mean; max_idx = (int)i; }
        if (tower_mean < min_mean) { min_mean = tower_mean; min_idx = (int)i; }
    }

    if (max_idx >= 0 && min_idx >= 0 && max_idx != min_idx) {
        mmuko_tower_t* src = towers[max_idx];
        mmuko_tower_t* dst = towers[min_idx];
        if (src->count > 0) {
            mmuko_pcb_t* proc = src->disks[src->count - 1]; // top disk
            int r = mmuko_migrate(src, dst, proc);
            if (r == 1) {
                printf("[REBALANCE] Migrated pid=%u %s -> %s\n",
                       proc->pid, src->name, dst->name);
            } else {
                printf("[REBALANCE] Migration blocked for pid=%u (Hanoi violation)\n",
                       proc->pid);
            }
        }
    }

    printf("[REBALANCE] Dimensional rebalancing complete.\n");
    return imbalances;
}
