// ============================================================
// MMUKO_SCHEDULER.C — Scheduler Orchestrator (main loop only)
// Module:  SCHEDULER_LOOP (mmuko-scheduler.pse § MODULE 7)
// Project: github.com/obinexus/mmuko-os
// Author:  Nnamdi Michael Okpala
// Date:    2026-05-22
// Version: 0.2-polar-scheduler
// ============================================================
//
// Separation of Concern: this file owns ONLY the scheduler loop.
//   mmuko_scheduler_init   — set scheduling mode
//   mmuko_scheduler_tick   — one scheduling quantum
//   mmuko_scheduler_run    — full run until completion
//   mmuko_next_pid         — global PID counter (shared via extern)
//
// System lifecycle (create/destroy/boot/print) lives in mmuko.c.
// Individual module implementations live in src/:
//   src/mmuko_tower.c       — TOWER_LIFECYCLE
//   src/mmuko_process.c     — PROCESS_LIFECYCLE
//   src/mmuko_priority.c    — POLAR_PRIORITY_HEAP
//   src/mmuko_governance.c  — GOVERNANCE_CHANNELS
//   src/mmuko_sched_modes.c — SCHEDULING_MODES
//   src/mmuko_rebalancer.c  — DIMENSIONAL_REBALANCER
//
// ============================================================

#include "mmuko.h"
#include "include/mmuko_priority.h"
#include "include/mmuko_rebalancer.h"

#include <stdio.h>
#include <inttypes.h>

// ─────────────────────────────────────────────────────────────────────────────
// GLOBAL PID COUNTER (shared via extern in process/sched_modes modules)
// ─────────────────────────────────────────────────────────────────────────────

uint32_t mmuko_next_pid = 1000;

// ─────────────────────────────────────────────────────────────────────────────
// SCHEDULER INIT
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_scheduler_init(MMUKO_System* sys, mmuko_sched_mode_t mode) {
    if (!sys) return -1;
    sys->sched_mode = mode;
    printf("\n[SCHEDULER] Initialized mode=%d\n", mode);
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// SCHEDULER TICK  — one scheduling quantum
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_scheduler_tick(MMUKO_System* sys) {
    if (!sys || !sys->boot_complete) return -1;

    mmuko_tower_t* ready   = sys->towers[MMUKO_SEG_STACK];
    mmuko_tower_t* running = sys->towers[MMUKO_SEG_DATA];
    if (!ready || !running) return -1;

    // Decrement remaining time for the running process
    if (running->count > 0) {
        mmuko_pcb_t* current = running->disks[running->count - 1];
        if (current->remaining_time > 0) current->remaining_time--;

        if (current->remaining_time == 0) {
            current->state           = MMUKO_PROC_TERMINATED;
            current->completion_time = mmuko_now_ms();
            current->turnaround_time = current->completion_time - current->arrival_time;
            printf("[TICK] pid=%u COMPLETED (turnaround=%" PRIu64 "ms)\n",
                   current->pid, current->turnaround_time);
            running->count--;
            running->disks[running->count] = NULL;
            running->used -= current->token.token_memory;
        }
    }

    // Schedule highest-priority ready process if CPU is idle
    if (running->count == 0 && ready->count > 0) {
        int      best_idx = -1;
        uint64_t best_pri = 0;
        for (uint32_t i = 0; i < ready->count; i++) {
            uint64_t pri = mmuko_polar_priority(ready->disks[i]);
            if (best_idx < 0 || pri > best_pri) { best_pri = pri; best_idx = (int)i; }
        }
        if (best_idx >= 0) {
            mmuko_pcb_t* next = ready->disks[best_idx];
            for (uint32_t i = (uint32_t)best_idx; i < ready->count - 1; i++) {
                ready->disks[i] = ready->disks[i + 1];
            }
            ready->count--;
            ready->used -= next->token.token_memory;

            running->disks[running->count] = next;
            running->count++;
            running->used += next->token.token_memory;
            next->state = MMUKO_PROC_RUNNING;
            printf("[TICK] pid=%u scheduled RUNNING (priority=%" PRIu64 ")\n",
                   next->pid, best_pri);
        }
    }

    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// SCHEDULER RUN  — full simulation until all processes terminate
// ─────────────────────────────────────────────────────────────────────────────

int mmuko_scheduler_run(MMUKO_System* sys, uint64_t max_ticks) {
    if (!sys) return -1;

    printf("\n========================================\n");
    printf("  MMUKO POLAR SCHEDULER START\n");
    printf("  Mode: %s\n",
           sys->sched_mode == MMUKO_SCHED_PREEMPTIVE     ? "PREEMPTIVE"     :
           sys->sched_mode == MMUKO_SCHED_NON_PREEMPTIVE ? "NON-PREEMPTIVE" :
                                                            "INEMPTIVE");
    printf("  Max ticks: %" PRIu64 "\n", max_ticks);
    printf("========================================\n");

    uint64_t tick   = 0;
    int      active = 1;

    while (active && tick < max_ticks) {
        mmuko_scheduler_tick(sys);

        // Periodic dimensional rebalancing every 10 ticks
        if (tick % 10 == 0 && tick > 0) {
            mmuko_tower_t* t[3] = {
                sys->towers[MMUKO_SEG_STACK],
                sys->towers[MMUKO_SEG_DATA],
                sys->towers[MMUKO_SEG_HEAP]
            };
            mmuko_rebalance_dimensions(t, 3, sys->proc_count);
        }

        // Termination check
        active = 0;
        for (uint32_t i = 0; i < sys->proc_count; i++) {
            if (sys->proc_table[i].state != MMUKO_PROC_TERMINATED &&
                sys->proc_table[i].state != MMUKO_PROC_ZOMBIE) {
                active = 1;
                break;
            }
        }
        tick++;
    }

    printf("\n========================================\n");
    printf("  MMUKO POLAR SCHEDULER END\n");
    printf("  Ticks executed: %" PRIu64 "\n", tick);
    printf("========================================\n");
    return 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// TEST ENTRY POINT
// ─────────────────────────────────────────────────────────────────────────────

#ifdef MMUKO_SCHEDULER_TEST

int main(int argc, char** argv) {
    (void)argc; (void)argv;

    printf("MMUKO OS Polar Priority Heap Scheduler Test\n");
    printf("OBINexus — \"Don't just schedule processes. Schedule truthful ones.\"\n\n");

    MMUKO_System* sys = mmuko_system_create(64, 16);
    if (!sys) { fprintf(stderr, "Failed to create MMUKO system\n"); return 1; }

    // Boot the system
    BootStatus status = mmuko_boot(sys);
    if (status != BOOT_OK) {
        printf("BOOT FAILED: %d\n", status);
        mmuko_system_destroy(sys);
        return 1;
    }

    mmuko_scheduler_init(sys, MMUKO_SCHED_PREEMPTIVE);

    // Create sample processes
    mmuko_pcb_t* p0 = mmuko_proc_create(0, MMUKO_RIGHTS_CIVIL | MMUKO_RIGHTS_HUMAN, 5, MMUKO_KB(4));
    mmuko_pcb_t* p1 = mmuko_proc_create(0, MMUKO_RIGHTS_CIVIL | MMUKO_RIGHTS_HUMAN | MMUKO_RIGHTS_DISABILITY, 8, MMUKO_KB(8));
    mmuko_pcb_t* p2 = mmuko_proc_create(0, MMUKO_RIGHTS_ALL, 12, MMUKO_KB(16));
    mmuko_pcb_t* p3 = mmuko_proc_create(0, MMUKO_RIGHTS_HUMAN, 3, MMUKO_KB(2));

    mmuko_proc_allocate(p0, sys->towers[MMUKO_SEG_STACK]);
    mmuko_proc_allocate(p1, sys->towers[MMUKO_SEG_STACK]);
    mmuko_proc_allocate(p2, sys->towers[MMUKO_SEG_STACK]);
    mmuko_proc_allocate(p3, sys->towers[MMUKO_SEG_STACK]);

    sys->proc_table[sys->proc_count++] = *p0;
    sys->proc_table[sys->proc_count++] = *p1;
    sys->proc_table[sys->proc_count++] = *p2;
    sys->proc_table[sys->proc_count++] = *p3;

    printf("\n--- Initial Tower States ---\n");
    mmuko_print_tower(sys->towers[MMUKO_SEG_STACK]);
    mmuko_print_tower(sys->towers[MMUKO_SEG_DATA]);

    mmuko_scheduler_run(sys, 100);
    mmuko_print_system_status(sys);

    mmuko_system_destroy(sys);
    free(p0); free(p1); free(p2); free(p3);
    printf("\nTest complete.\n");
    return 0;
}

#endif // MMUKO_SCHEDULER_TEST
