// ============================================================
// MMUKO_SCHED_MODES.C — Preemptive / Non-Preemptive / Inemptive
// Module:  SCHEDULING_MODES (mmuko-scheduler.pse § MODULE 5)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// ============================================================
//
// Separation of Concern: owns only the three scheduling mode
// implementations and inemptive buffer resolution.
// Priority, governance, and tower ops are delegated to their modules.
//
// ============================================================

#include "../include/mmuko_sched_modes.h"
#include "../include/mmuko_governance.h"
#include "../include/mmuko_priority.h"
#include "../include/mmuko_process.h"
#include <stdio.h>
#include <inttypes.h>

extern uint32_t mmuko_next_pid;

// ─────────────────────────────────────────────────────────────────────────────
// PREEMPTIVE scheduling
// If incoming has higher polar priority than the running process, preempt.
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_schedule_preemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                               mmuko_pcb_t* incoming) {
    if (!ready || !running || !incoming) return -1;

    printf("\n[SCHED] PREEMPTIVE: incoming pid=%u (priority=%" PRIu64 ")\n",
           incoming->pid, mmuko_polar_priority(incoming));

    mmuko_channel_t ch = mmuko_governance_check(incoming, running);
    if (ch == CH_2_COLLAPSE) {
        mmuko_governance_enforce(incoming, ch);
        return -1;
    }

    if (running->count > 0) {
        mmuko_pcb_t* current = running->disks[running->count - 1];
        if (mmuko_compare_priority(incoming, current) > 0) {
            // Incoming outranks running — PREEMPT
            printf("[SCHED] PREEMPT: pid=%u (pri=%" PRIu64
                   ") > pid=%u (pri=%" PRIu64 ")\n",
                   incoming->pid, mmuko_polar_priority(incoming),
                   current->pid,  mmuko_polar_priority(current));

            int r = mmuko_migrate(running, ready, current);
            if (r < 0) {
                printf("[SCHED] Preempt failed: cannot push current to ready\n");
                return -1;
            }
            current->state = MMUKO_PROC_READY;

            r = mmuko_migrate(ready, running, incoming);
            if (r < 0) {
                printf("[SCHED] Preempt failed: cannot load incoming\n");
                return -1;
            }
            incoming->state = MMUKO_PROC_RUNNING;
            mmuko_governance_enforce(incoming, CH_0_OBSERVE);
            return 1; // preemption occurred
        }
    }

    // No preemption — queue incoming in ready
    int r = mmuko_migrate(NULL, ready, incoming);
    if (r < 0) r = mmuko_proc_allocate(incoming, ready);
    incoming->state = MMUKO_PROC_READY;
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// NON-PREEMPTIVE scheduling
// Schedule only if CPU is idle. Otherwise queue in ready.
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_schedule_non_preemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                                   mmuko_pcb_t* incoming) {
    if (!ready || !running || !incoming) return -1;

    printf("\n[SCHED] NON-PREEMPTIVE: incoming pid=%u\n", incoming->pid);

    mmuko_channel_t ch = mmuko_governance_check(incoming, running);
    if (ch == CH_2_COLLAPSE) {
        mmuko_governance_enforce(incoming, ch);
        return -1;
    }

    if (running->count == 0) {
        int r = mmuko_migrate(ready, running, incoming);
        if (r < 0) r = mmuko_proc_allocate(incoming, running);
        incoming->state = MMUKO_PROC_RUNNING;
        mmuko_governance_enforce(incoming, CH_0_OBSERVE);
        printf("[SCHED] NON-PREEMPTIVE: pid=%u scheduled (CPU was idle)\n",
               incoming->pid);
        return 1;
    } else {
        int r = mmuko_proc_allocate(incoming, ready);
        if (r < 0) {
            printf("[SCHED] NON-PREEMPTIVE: pid=%u cannot fit in ready queue\n",
                   incoming->pid);
            return -1;
        }
        incoming->state = MMUKO_PROC_READY;
        printf("[SCHED] NON-PREEMPTIVE: pid=%u queued in ready (CPU busy)\n",
               incoming->pid);
        return 0;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// INEMPTIVE scheduling  (memory-preemptive, time-nonpreemptive)
// Allocates memory to high-priority process via a buffer/staging PID
// without performing a full context switch on the running process.
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_schedule_inemptive(mmuko_tower_t* ready, mmuko_tower_t* running,
                              mmuko_inemptive_buffer_t* buffer, mmuko_pcb_t* incoming) {
    if (!ready || !running || !buffer || !incoming) return -1;

    printf("\n[SCHED] INEMPTIVE: incoming pid=%u (mem=%" PRIu64 ")\n",
           incoming->pid, incoming->token.token_memory);

    uint64_t available = running->domain_size - running->used;

    if (incoming->token.token_memory <= available && mmuko_can_stack(running, incoming)) {
        // Direct allocation — no staging needed
        int r = mmuko_migrate(ready, running, incoming);
        if (r < 0) r = mmuko_proc_allocate(incoming, running);
        if (r == 0) {
            incoming->state = MMUKO_PROC_RUNNING;
            printf("[SCHED] INEMPTIVE: pid=%u directly allocated to running\n",
                   incoming->pid);
            return 1;
        }
    }

    // Staged inemptive path — create buffer process for memory reservation
    buffer->target_proc  = incoming;
    buffer->overlap_size = incoming->token.token_memory;
    buffer->buffer_pid   = mmuko_next_pid++;
    buffer->resolved     = false;

    // Stage in heap/ready tower
    if (mmuko_can_stack(ready, incoming)) {
        int r = mmuko_proc_allocate(incoming, ready);
        if (r == 0) {
            incoming->state       = MMUKO_PROC_BLOCKED;
            buffer->staging_tower = ready;
            buffer->resolved      = false;
            printf("[SCHED] INEMPTIVE: pid=%u staged (buffer_pid=%u overlap=%" PRIu64 ")\n",
                   incoming->pid, buffer->buffer_pid, buffer->overlap_size);
            return 0; // buffered — awaiting resolution
        }
    }

    mmuko_governance_enforce(incoming, CH_2_COLLAPSE);
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// INEMPTIVE RESOLVE — called when running process releases memory
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_inemptive_resolve(mmuko_inemptive_buffer_t* buffer, mmuko_tower_t* running) {
    if (!buffer || !buffer->target_proc || !running) return -1;
    if (buffer->resolved) return 0;

    mmuko_pcb_t* proc     = buffer->target_proc;
    uint64_t     available = running->domain_size - running->used;

    if (available >= buffer->overlap_size && mmuko_can_stack(running, proc)) {
        int r = mmuko_migrate(buffer->staging_tower, running, proc);
        if (r == 1 || r == 0) {
            proc->state     = MMUKO_PROC_RUNNING;
            buffer->resolved = true;
            printf("[INEMPTIVE] Resolved: pid=%u migrated to running (buffer_pid=%u retired)\n",
                   proc->pid, buffer->buffer_pid);
            return 1;
        }
    }

    printf("[INEMPTIVE] Cannot resolve yet: pid=%u still staged\n", proc->pid);
    return 0;
}
