// ============================================================
// MMUKO_PROCESS.C — PCB Lifecycle Implementation
// Module:  PROCESS_LIFECYCLE (mmuko-scheduler.pse § MODULE 2)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// ============================================================
//
// Separation of Concern: owns only PCB allocation, initialisation,
// and tower placement. Priority computation is in mmuko_priority.c.
//
// ============================================================

#include "../include/mmuko_process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

// Shared PID counter (extern declared here; defined in mmuko_scheduler.c)
extern uint32_t mmuko_next_pid;

// Forward declaration — priority computed by mmuko_priority module
uint64_t mmuko_polar_priority(mmuko_pcb_t* p);

// ─────────────────────────────────────────────────────────────────────────────
// PROCESS CREATE / DESTROY
// ─────────────────────────────────────────────────────────────────────────────

mmuko_pcb_t* mmuko_proc_create(uint32_t pid, mmuko_rights_t rights,
                                uint64_t burst_time, uint64_t memory_need) {
    mmuko_pcb_t* proc = (mmuko_pcb_t*)calloc(1, sizeof(mmuko_pcb_t));
    if (!proc) return NULL;

    proc->pid            = (pid == 0) ? mmuko_next_pid++ : pid;
    proc->rights         = rights;
    proc->burst_time     = burst_time;
    proc->remaining_time = burst_time;
    proc->arrival_time   = mmuko_now_ms();
    proc->state          = MMUKO_PROC_NEW;

    proc->token.token_type   = TOKEN_CLASSICAL_FIXED;
    proc->token.token_value  = 0;
    proc->token.token_memory = memory_need;

    proc->dimension_count  = MMUKO_DIM_N(proc);
    proc->strategic_value  = mmuko_polar_priority(proc);
    proc->bayesian_conf    = 0.5;   // uniform prior
    proc->calibrated       = false;
    proc->next             = NULL;
    proc->prev             = NULL;

    printf("[PROC] Created pid=%u rights=0x%02X burst=%" PRIu64
           "ms mem=%" PRIu64 " D=%" PRIu64 " N=%u priority=%" PRIu64 "\n",
           proc->pid, proc->rights, proc->burst_time,
           proc->token.token_memory,
           (uint64_t)MMUKO_DIM_D(proc),
           proc->dimension_count,
           proc->strategic_value);
    return proc;
}

void mmuko_proc_destroy(mmuko_pcb_t* proc) {
    free(proc);
}

// ─────────────────────────────────────────────────────────────────────────────
// ALLOCATE: place process onto a tower (initial placement)
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_proc_allocate(mmuko_pcb_t* proc, mmuko_tower_t* tower) {
    if (!proc || !tower) return -1;
    if (!mmuko_can_stack(tower, proc)) return -1;

    tower->disks[tower->count] = proc;
    tower->count++;
    tower->used += proc->token.token_memory;
    proc->token.token_type = (uint8_t)tower->segment;
    proc->state = MMUKO_PROC_READY;

    printf("[ALLOC] pid=%u -> %s (mem=%" PRIu64
           " tower_used=%" PRIu64 "/%" PRIu64 ")\n",
           proc->pid, tower->name,
           proc->token.token_memory,
           tower->used, tower->domain_size);
    return 0;
}
