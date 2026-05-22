// ============================================================
// PHASE2_COMPASS_ALIGNMENT.C — PHASE_SAFETY_SCAN
// mmuko-boot.psc § Phase 2
// ============================================================
// For every cubit with UNDEFINED_DIR, resolve its direction
// by majority-vote of its neighbours.
// If direction remains undefined after resolution: BOOT_LOCK_DETECTED.
// ============================================================

#include "phase2_compass_alignment.h"
#include "boot_helpers.h"

// Forward declare contract helper (defined in mmuko-boot.c orchestrator)
void mmuko_contract_mark_outcome_phase2(MMUKO_System* sys, int alert);

BootStatus phase2_compass_alignment(MMUKO_System* sys) {
    sys->current_phase = 2;
    if (sys->contract) sys->contract->membrane_phase = 2;

    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte* byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            Cubit* cubit = &byte->cubit_ring[i];
            if (cubit->direction != UNDEFINED_DIR) continue;

            cubit->direction = resolve_direction_from_neighbors(byte, i);
            if (cubit->direction == UNDEFINED_DIR) {
                // Safety scan failure: compass lock detected
                if (sys->contract) {
                    sys->contract->membrane_outcome = 0xCC; // MMUKO_MEMBRANE_ALERT
                }
                return BOOT_LOCK_DETECTED;
            }
        }
    }

    return BOOT_OK;
}
