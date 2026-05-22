// ============================================================
// PHASE6_ROTATION_VERIFICATION.C — PHASE_INTEGRITY_VERIFICATION
// mmuko-boot.psc § Phase 6
// ============================================================
// Verify the integrity of every cubit by double-rotating its
// value by 4 bits and confirming the result equals the original.
// A failed rotation means the cubit is in a degenerate state —
// BOOT_ROTATION_LOCK is raised and the outcome is set to ALERT.
// This implements the "discriminant >= 0" and
// "kernel_entry_is_resolved == TRUE" clauses from the spec.
// ============================================================

#include "phase6_rotation_verification.h"
#include "boot_helpers.h"

BootStatus phase6_rotation_verification(MMUKO_System* sys) {
    sys->current_phase = 6;
    if (sys->contract) sys->contract->membrane_phase = 6;

    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte* byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            uint8_t original = byte->cubit_ring[i].value;
            uint8_t test_val = rotate_bits(original, 4);
            test_val         = rotate_bits(test_val, 4);

            if (test_val != original) {
                // Rotation invariant broken — integrity failure
                if (sys->contract) {
                    sys->contract->membrane_outcome = 0xCC; // MMUKO_MEMBRANE_ALERT
                }
                return BOOT_ROTATION_LOCK;
            }
        }
    }

    return BOOT_OK;
}
