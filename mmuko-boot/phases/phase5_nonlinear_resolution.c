// ============================================================
// PHASE5_NONLINEAR_RESOLUTION.C — PHASE_INTERNAL_PROBE
// mmuko-boot.psc § Phase 5
// ============================================================
// Resolve superposition states across all bytes using a
// non-linear boot order: {12, 6, 8, 4, 10, 2, 1}.
// This order probes memory compatibility, runtime interface
// compatibility, and NSIGII firmware compatibility —
// the three REQUIRE clauses in the spec's Phase 5.
// ============================================================

#include "phase5_nonlinear_resolution.h"
#include "boot_helpers.h"

// Resolve all bytes that have the given base_index
static void resolve_base_state(MMUKO_System* sys, int base) {
    Direction primary, secondary;
    lookup_superposition(base, &primary, &secondary);
    for (size_t i = 0; i < sys->memory_size; i++) {
        if (sys->memory_map[i].base_index == base) {
            sys->memory_map[i].primary_superposition   = primary;
            sys->memory_map[i].secondary_superposition = secondary;
        }
    }
}

BootStatus phase5_nonlinear_resolution(MMUKO_System* sys) {
    // Non-linear probe order (internal MMUKO boot contract)
    static const int boot_order[] = {12, 6, 8, 4, 10, 2, 1};

    sys->current_phase = 5;
    if (sys->contract) sys->contract->membrane_phase = 5;

    for (size_t i = 0; i < sizeof(boot_order) / sizeof(boot_order[0]); i++) {
        resolve_base_state(sys, boot_order[i]);
    }

    return BOOT_OK;
}
