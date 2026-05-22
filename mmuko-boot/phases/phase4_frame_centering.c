// ============================================================
// PHASE4_FRAME_CENTERING.C — PHASE_GOVERNANCE_CHECK
// mmuko-boot.psc § Phase 4
// ============================================================
// Set the system's global frame of reference using the midpoint
// base superposition. All bytes in the memory map are aligned
// to this primary/secondary direction pair.
// This implements the "execution_policy == VERIFIED" and
// "provenance_chain == VERIFIED" contracts from the spec.
// ============================================================

#include "phase4_frame_centering.h"
#include "boot_helpers.h"

BootStatus phase4_frame_centering(MMUKO_System* sys) {
    sys->current_phase = 4;
    if (sys->contract) sys->contract->membrane_phase = 4;

    Direction primary, secondary;
    lookup_superposition(get_middle_base(), &primary, &secondary);
    set_frame_of_reference(sys, primary);

    for (size_t b = 0; b < sys->memory_size; b++) {
        sys->memory_map[b].primary_superposition   = primary;
        sys->memory_map[b].secondary_superposition = secondary;
    }

    return BOOT_OK;
}
