// ============================================================
// PHASE1_CUBIT_INIT.C — PHASE_NEED_STATE_INIT
// mmuko-boot.psc § Phase 1
// ============================================================
// Initialise the cubit ring for every byte in the memory map.
// Each byte gets its 8 cubits seeded from raw_value, with
// compass directions, spin values, entanglement, and
// superposition directions resolved from the lookup table.
// ============================================================

#include "phase1_cubit_init.h"
#include "boot_helpers.h"

BootStatus phase1_cubit_init(MMUKO_System* sys) {
    sys->current_phase = 1;
    if (sys->contract) sys->contract->membrane_phase = 1;

    for (size_t i = 0; i < sys->memory_size; i++) {
        uint8_t value = sys->memory_map[i].raw_value;
        sys->memory_map[i].base_index = (value % 12) + 1;
        init_cubit_ring(&sys->memory_map[i]);
        lookup_superposition(sys->memory_map[i].base_index,
                             &sys->memory_map[i].primary_superposition,
                             &sys->memory_map[i].secondary_superposition);
    }

    return BOOT_OK;
}
