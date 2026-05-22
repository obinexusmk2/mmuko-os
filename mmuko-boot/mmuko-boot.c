// ============================================================
// MMUKO-BOOT.C — Boot Orchestrator (phases extracted)
// Project: OBINexus / OBIELF R&D
// Author:  Nnamdi Michael Okpala
// Version: 0.2-portable
// ============================================================
//
// Separation of Concern: this file is now the ORCHESTRATOR only.
// It owns:
//   - mmuko_system_init / mmuko_boot_bind_contract
//   - mmuko_seed_memory_map
//   - mmuko_boot() — sequential phase 1–6 dispatch
//   - Contract marking helpers
//   - Desktop sim main()
//
// All phase logic has been separated into:
//   phases/boot_helpers.c                  — shared utilities
//   phases/phase1_cubit_init.c             — PHASE_NEED_STATE_INIT
//   phases/phase2_compass_alignment.c      — PHASE_SAFETY_SCAN
//   phases/phase3_superposition_entanglement.c — PHASE_IDENTITY_CALIBRATION
//   phases/phase4_frame_centering.c        — PHASE_GOVERNANCE_CHECK
//   phases/phase5_nonlinear_resolution.c   — PHASE_INTERNAL_PROBE
//   phases/phase6_rotation_verification.c  — PHASE_INTEGRITY_VERIFICATION
//
// ============================================================

#include "boot_contract.h"

// Phase headers
#include "phases/phase1_cubit_init.h"
#include "phases/phase2_compass_alignment.h"
#include "phases/phase3_superposition_entanglement.h"
#include "phases/phase4_frame_centering.h"
#include "phases/phase5_nonlinear_resolution.h"
#include "phases/phase6_rotation_verification.h"
#include "phases/boot_helpers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef MMUKO_BOOT_DESKTOP_SIM
#include <stdio.h>
#endif

// ─────────────────────────────────────────────────────────────────────────────
// CONTRACT HELPERS  (used by phase files via forward declarations)
// ─────────────────────────────────────────────────────────────────────────────

static void mmuko_contract_mark_phase(MMUKO_System* sys, uint8_t phase) {
    sys->current_phase = phase;
    if (sys->contract) sys->contract->membrane_phase = phase;
}

static void mmuko_contract_mark_outcome(MMUKO_System* sys,
                                        mmuko_membrane_outcome_t outcome,
                                        mmuko_transfer_state_t transfer) {
    if (!sys->contract) return;
    sys->contract->membrane_outcome = (uint8_t)outcome;
    sys->contract->transfer_state   = (uint8_t)transfer;
    sys->contract->boot_flags      |= MMUKO_BOOT_FLAG_NSIGII_READY;
}

// ─────────────────────────────────────────────────────────────────────────────
// MEMORY SEEDING
// ─────────────────────────────────────────────────────────────────────────────

static uint8_t seed_raw_value_from_contract(const mmuko_boot_contract_t* contract, size_t index) {
    uint8_t seeded = (uint8_t)(index * 17u + 42u);
    if (!contract) return seeded;
    seeded ^= (uint8_t)(contract->boot_flags & 0xFFu);
    seeded ^= contract->membrane_outcome;
    if (contract->keyboard.length > 0) {
        seeded ^= (uint8_t)contract->keyboard.bytes[index % contract->keyboard.length];
    }
    return bit_shift_semantic(seeded, ROTATE, (int)(index % 8u));
}

static void mmuko_seed_memory_map(MMUKO_System* sys) {
    for (size_t i = 0; i < sys->memory_size; i++) {
        sys->memory_map[i].raw_value = seed_raw_value_from_contract(sys->contract, i);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// CONTRACT BIND & SYSTEM INIT
// ─────────────────────────────────────────────────────────────────────────────

static void mmuko_boot_bind_contract(MMUKO_System* sys,
                                     mmuko_boot_contract_t* contract,
                                     mmuko_transfer_state_t mode) {
    sys->contract      = contract;
    sys->current_phase = 0;
    if (!contract) return;

    if (contract->magic != MMUKO_BOOT_CONTRACT_MAGIC ||
        contract->total_size != sizeof(mmuko_boot_contract_t)) {
        mmuko_boot_contract_reset(contract);
    }

    contract->transfer_state  = (uint8_t)mode;
    contract->boot_flags     |= MMUKO_BOOT_FLAG_NATIVE_C_READY;
    contract->membrane_outcome = MMUKO_MEMBRANE_HOLD;
}

static void mmuko_system_init(MMUKO_System* sys,
                              MMUKO_Byte* storage,
                              size_t memory_size,
                              mmuko_boot_contract_t* contract,
                              mmuko_transfer_state_t mode) {
    memset(sys, 0, sizeof(*sys));
    sys->memory_map  = storage;
    sys->memory_size = memory_size;
    sys->frame_of_reference = N;
    sys->medium.gravity = G_VACUUM;
    mmuko_boot_bind_contract(sys, contract, mode);
    mmuko_seed_memory_map(sys);
}

// ─────────────────────────────────────────────────────────────────────────────
// MMUKO_BOOT — orchestrates phases 1–6 in sequence
// ─────────────────────────────────────────────────────────────────────────────

static BootStatus mmuko_boot(MMUKO_System* sys) {
    BootStatus status;

    mmuko_contract_mark_phase(sys, 0);

    status = phase1_cubit_init(sys);
    if (status != BOOT_OK) {
        mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_ALERT, MMUKO_TRANSFER_NATIVE_C_ENTRY);
        return status;
    }

    status = phase2_compass_alignment(sys);
    if (status != BOOT_OK) {
        mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_ALERT, MMUKO_TRANSFER_NATIVE_C_ENTRY);
        return status;
    }

    status = phase3_superposition_entanglement(sys);
    if (status != BOOT_OK) {
        mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_ALERT, MMUKO_TRANSFER_NATIVE_C_ENTRY);
        return status;
    }

    status = phase4_frame_centering(sys);
    if (status != BOOT_OK) {
        mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_ALERT, MMUKO_TRANSFER_NATIVE_C_ENTRY);
        return status;
    }

    status = phase5_nonlinear_resolution(sys);
    if (status != BOOT_OK) {
        mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_ALERT, MMUKO_TRANSFER_NATIVE_C_ENTRY);
        return status;
    }

    status = phase6_rotation_verification(sys);
    if (status != BOOT_OK) {
        mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_ALERT, MMUKO_TRANSFER_NATIVE_C_ENTRY);
        return status;
    }

    sys->boot_complete = true;
    mmuko_contract_mark_phase(sys, 7);
    mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_PASS, MMUKO_TRANSFER_KERNEL_ENTRY);
    return BOOT_OK;
}

// ─────────────────────────────────────────────────────────────────────────────
// DESKTOP SIMULATOR ENTRY POINT
// ─────────────────────────────────────────────────────────────────────────────

#ifdef MMUKO_BOOT_DESKTOP_SIM

static void mmuko_print_cubit_state(const MMUKO_System* sys, size_t byte_idx, int cubit_idx) {
    if (byte_idx >= sys->memory_size || cubit_idx < 0 || cubit_idx >= 8) return;
    const Cubit* cubit = &sys->memory_map[byte_idx].cubit_ring[cubit_idx];
    printf("Byte[%zu].Cubit[%d]: val=%u dir=%s state=%s spin=%.4f super=%s ent=%d\n",
           byte_idx, cubit_idx,
           cubit->value,
           direction_to_string(cubit->direction),
           state_to_string(cubit->state),
           cubit->spin,
           cubit->superposed ? "YES" : "NO",
           cubit->entangled_with);
}

int main(int argc, char** argv) {
    mmuko_boot_contract_t contract;
    MMUKO_System          sys;
    MMUKO_Byte            memory[16];

    mmuko_boot_contract_reset(&contract);
    contract.boot_flags    = MMUKO_BOOT_FLAG_STAGE2_MODE | MMUKO_BOOT_FLAG_NATIVE_C_READY;
    contract.transfer_state = MMUKO_TRANSFER_STAGE2_READY;

    if (argc > 1) {
        if (mmuko_keyboard_buffer_copy_text(&contract.keyboard, argv[1]) > 0) {
            contract.boot_flags |= MMUKO_BOOT_FLAG_KEYBOARD_REQUIRED | MMUKO_BOOT_FLAG_KEYBOARD_PRESENT;
        }
    }

    mmuko_system_init(&sys, memory, sizeof(memory)/sizeof(memory[0]),
                      &contract, MMUKO_TRANSFER_STAGE2_READY);

    printf("MMUKO OS Boot Loader (%s)\n", MMUKO_VERSION);
    printf("Contract @ 0x%04X, keyboard bytes=%u\n\n",
           MMUKO_BOOT_CONTRACT_ADDR, contract.keyboard.length);

    BootStatus status = mmuko_boot(&sys);
    if (status != BOOT_OK) {
        printf("BOOT FAILED: status=%d phase=%u outcome=0x%02X\n",
               status, contract.membrane_phase, contract.membrane_outcome);
        return 1;
    }

    printf("BOOT COMPLETE\n");
    printf("Transfer state=%u frame=%s outcome=0x%02X\n",
           contract.transfer_state,
           direction_to_string(sys.frame_of_reference),
           contract.membrane_outcome);
    printf("Gravity medium: G=%.4f (lepton=%.4f muon=%.4f deep=%.4f)\n",
           G_VACUUM, G_LEPTON, G_MUON, G_DEEP);
    mmuko_print_cubit_state(&sys, 0, 0);
    mmuko_print_cubit_state(&sys, 0, 2);
    mmuko_print_cubit_state(&sys, 5, 5);
    return 0;
}

#endif // MMUKO_BOOT_DESKTOP_SIM

// ============================================================
// END OF MMUKO-BOOT.C (orchestrator)
// ============================================================
