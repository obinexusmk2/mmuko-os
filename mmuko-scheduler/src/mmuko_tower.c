// ============================================================
// MMUKO_TOWER.C — Tower of Hanoi Memory Domain Implementation
// Module:  TOWER_LIFECYCLE (mmuko-scheduler.pse § MODULE 1)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// ============================================================
//
// Separation of Concern: owns only tower create/destroy,
// stack constraint enforcement, and cross-tower migration.
// No process creation, no priority logic, no scheduling modes.
//
// ============================================================

#include "../include/mmuko_tower.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// ─────────────────────────────────────────────────────────────────────────────
// TOWER CREATE / DESTROY
// ─────────────────────────────────────────────────────────────────────────────

mmuko_tower_t* mmuko_tower_create(mmuko_segment_t seg, uint32_t capacity,
                                   uint64_t domain_size, const char* name) {
    mmuko_tower_t* tower = (mmuko_tower_t*)calloc(1, sizeof(mmuko_tower_t));
    if (!tower) return NULL;

    tower->disks = (mmuko_pcb_t**)calloc(capacity, sizeof(mmuko_pcb_t*));
    if (!tower->disks) {
        free(tower);
        return NULL;
    }

    tower->capacity    = capacity;
    tower->domain_size = domain_size;
    tower->used        = 0;
    tower->count       = 0;
    tower->segment     = seg;
    strncpy(tower->name, name, 15);
    tower->name[15] = '\0';

    printf("[TOWER] Created %s: cap=%u, size=%" PRIu64 " bytes\n",
           name, capacity, domain_size);
    return tower;
}

void mmuko_tower_destroy(mmuko_tower_t* tower) {
    if (!tower) return;
    // Note: PCB objects are owned by the system proc_table — do NOT free them here.
    if (tower->disks) free(tower->disks);
    free(tower);
}

// ─────────────────────────────────────────────────────────────────────────────
// HANOI CONSTRAINT: larger disk cannot be placed on smaller disk
// In MMUKO: protects memory hierarchy — large allocations cannot
// overlay smaller ones in stack domains (FILO integrity).
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_can_stack(mmuko_tower_t* tower, mmuko_pcb_t* proc) {
    if (!tower || !proc) return 0;
    if (tower->count == 0) return 1;                  // empty tower always accepts
    if (tower->count >= tower->capacity) return 0;    // full

    mmuko_pcb_t* top = tower->disks[tower->count - 1];
    if (!top) return 1;

    // Smaller or equal memory can sit on top of larger memory
    return proc->token.token_memory <= top->token.token_memory;
}

// ─────────────────────────────────────────────────────────────────────────────
// MIGRATE: move proc from src tower to dst tower
// Returns:  1 = success, -1 = Hanoi violation, 0 = error
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_migrate(mmuko_tower_t* src, mmuko_tower_t* dst, mmuko_pcb_t* proc) {
    if (!dst || !proc) return 0;

    if (!mmuko_can_stack(dst, proc)) {
        printf("[MIGRATE] VIOLATION: pid=%u (mem=%" PRIu64
               ") cannot stack on %s top (mem=%" PRIu64 ")\n",
               proc->pid, proc->token.token_memory, dst->name,
               dst->count > 0 ? dst->disks[dst->count - 1]->token.token_memory : 0ULL);
        return -1;
    }

    // Remove from source (if provided)
    if (src) {
        int found = -1;
        for (uint32_t i = 0; i < src->count; i++) {
            if (src->disks[i] == proc) { found = (int)i; break; }
        }
        if (found >= 0) {
            for (uint32_t i = (uint32_t)found; i < src->count - 1; i++) {
                src->disks[i] = src->disks[i + 1];
            }
            src->disks[src->count - 1] = NULL;
            src->count--;
            if (proc->token.token_memory <= src->used)
                src->used -= proc->token.token_memory;
        }
    }

    // Push to destination
    dst->disks[dst->count] = proc;
    dst->count++;
    dst->used += proc->token.token_memory;
    proc->token.token_type = (uint8_t)dst->segment;

    printf("[MIGRATE] pid=%u %s -> %s (mem=%" PRIu64
           ", used=%" PRIu64 "/%" PRIu64 ")\n",
           proc->pid,
           src ? src->name : "NONE", dst->name,
           proc->token.token_memory,
           dst->used, dst->domain_size);
    return 1;
}
