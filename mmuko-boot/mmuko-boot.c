// ============================================================
// MMUKO-BOOT.C — MMUKO OS Boot Contract Runtime
// Project: OBINexus / OBIELF R&D
// Author: OBINexus Research Division
// Version: 0.2-contract-runtime
// ============================================================
//
// This runtime consumes the shared boot contract defined in
// mmuko_boot_contract.h. The intent is that boot.asm populates the
// contract at MMUKO_BOOT_CONTRACT_PHYS_ADDR and hands execution to a
// richer runtime that continues the same phase identifiers and status
// codes instead of inventing a separate demo-only ABI.
// ============================================================

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mmuko_boot_contract.h"

#define MMUKO_VERSION "0.2-contract-runtime"
#define MMUKO_CONTRACT_FLAG_NEEDS_VALID  0x0001u
#define MMUKO_CONTRACT_FLAG_RUNTIME_OWNS 0x0002u
#define MMUKO_CONTRACT_FLAG_ROTATION_OK  0x0004u

#define G_VACUUM 9.8

typedef enum {
    N, NE, E, SE, S, SW, W, NW, UNDEFINED_DIR
} Direction;

typedef enum {
    UP, DOWN, CHARM, STRANGE, LEFT, RIGHT
} State;

typedef struct {
    int index;
    uint8_t value;
    Direction direction;
    State state;
    bool superposed;
    int entangled_with;
} Cubit;

typedef struct {
    uint8_t raw_value;
    Cubit cubit_ring[8];
    int base_index;
    Direction primary_superposition;
    Direction secondary_superposition;
} MMUKO_Byte;

typedef struct {
    MMUKO_Byte *memory_map;
    size_t memory_size;
    double gravity;
    Direction frame_of_reference;
    mmuko_boot_contract_record *contract;
} MMUKO_Runtime;

typedef struct {
    int base;
    Direction primary;
    Direction secondary;
} SuperpositionEntry;

static const SuperpositionEntry superposition_table[] = {
    {12, S, N},
    {10, SE, N},
    {8, E, W},
    {6, SW, E},
    {4, W, E},
    {2, NE, W},
    {1, N, S}
};

static const char *direction_names[] = {
    "NORTH", "NORTHEAST", "EAST", "SOUTHEAST",
    "SOUTH", "SOUTHWEST", "WEST", "NORTHWEST", "UNDEFINED"
};

static const int entangled_pairs[] = {7, 6, 5, -1, -1, 2, 1, 0};

static const char *mmuko_phase_name(mmuko_boot_phase_id phase)
{
    switch (phase) {
        case MMUKO_BOOT_PHASE_PREPARE: return "PREPARE";
        case MMUKO_BOOT_PHASE_N: return "N";
        case MMUKO_BOOT_PHASE_S: return "S";
        case MMUKO_BOOT_PHASE_I_IDENT: return "I_IDENT";
        case MMUKO_BOOT_PHASE_G: return "G";
        case MMUKO_BOOT_PHASE_I_PROBE: return "I_PROBE";
        case MMUKO_BOOT_PHASE_I_INTEG: return "I_INTEG";
        case MMUKO_BOOT_PHASE_HANDOFF: return "HANDOFF";
        case MMUKO_BOOT_PHASE_COMPLETE: return "COMPLETE";
        default: return "UNKNOWN";
    }
}

static const char *mmuko_status_name(mmuko_boot_status status)
{
    switch (status) {
        case MMUKO_BOOT_STATUS_HOLD: return "HOLD";
        case MMUKO_BOOT_STATUS_PASS: return "PASS";
        case MMUKO_BOOT_STATUS_ALERT: return "ALERT";
        case MMUKO_BOOT_STATUS_FAULT: return "FAULT";
        default: return "UNKNOWN";
    }
}

static const char *direction_to_string(Direction dir)
{
    if (dir >= 0 && dir <= UNDEFINED_DIR) {
        return direction_names[dir];
    }
    return "INVALID";
}

static State resolve_state(int index, uint8_t byte_val)
{
    uint8_t bit = (uint8_t)((byte_val >> index) & 1u);
    uint8_t neighbor = (uint8_t)((byte_val >> ((index + 1) % 8)) & 1u);

    if (bit == 1u && neighbor == 1u) return UP;
    if (bit == 1u && neighbor == 0u) return CHARM;
    if (bit == 0u && neighbor == 1u) return STRANGE;
    return DOWN;
}

static void lookup_superposition(int base, Direction *primary, Direction *secondary)
{
    size_t table_size = sizeof(superposition_table) / sizeof(superposition_table[0]);

    for (size_t i = 0; i < table_size; ++i) {
        if (superposition_table[i].base == base) {
            *primary = superposition_table[i].primary;
            *secondary = superposition_table[i].secondary;
            return;
        }
    }

    *primary = N;
    *secondary = S;
}

static uint8_t rotate_bits(uint8_t value, int n)
{
    n %= 8;
    if (n == 0) {
        return value;
    }
    return (uint8_t)(((value >> n) | (value << (8 - n))) & 0xFFu);
}

static void contract_set_status(mmuko_boot_contract_record *contract,
                                mmuko_boot_status status,
                                uint8_t reason)
{
    contract->status = (uint8_t)status;
    contract->status_reason = reason;
    mmuko_boot_contract_finalize(contract);
}

static void contract_enter_phase(mmuko_boot_contract_record *contract,
                                 mmuko_boot_phase_id phase)
{
    contract->stage.last_phase = contract->stage.current_phase;
    contract->stage.current_phase = (uint16_t)phase;
    mmuko_boot_contract_finalize(contract);
}

static void init_cubit_ring(MMUKO_Byte *byte)
{
    static const Direction directions[] = {N, NE, E, SE, S, SW, W, NW};

    for (int i = 0; i < 8; ++i) {
        Cubit *cubit = &byte->cubit_ring[i];
        cubit->index = i;
        cubit->value = (uint8_t)((byte->raw_value >> i) & 1u);
        cubit->direction = directions[i];
        cubit->state = resolve_state(i, byte->raw_value);
        cubit->entangled_with = entangled_pairs[i];
        cubit->superposed = (entangled_pairs[i] != -1);
    }
}

static MMUKO_Runtime *mmuko_runtime_create(mmuko_boot_contract_record *contract)
{
    MMUKO_Runtime *runtime = calloc(1, sizeof(*runtime));
    if (!runtime) {
        return NULL;
    }

    runtime->memory_size = contract->layout.memory_bytes == 0 ? 16u : contract->layout.memory_bytes;
    runtime->memory_map = calloc(runtime->memory_size, sizeof(*runtime->memory_map));
    if (!runtime->memory_map) {
        free(runtime);
        return NULL;
    }

    runtime->gravity = G_VACUUM;
    runtime->frame_of_reference = N;
    runtime->contract = contract;

    for (size_t i = 0; i < runtime->memory_size; ++i) {
        runtime->memory_map[i].raw_value = (uint8_t)(0x2Au + (uint8_t)(i * 17u));
    }

    return runtime;
}

static void mmuko_runtime_destroy(MMUKO_Runtime *runtime)
{
    if (!runtime) {
        return;
    }

    free(runtime->memory_map);
    free(runtime);
}

static mmuko_boot_status phase_prepare(MMUKO_Runtime *runtime)
{
    mmuko_boot_contract_record *contract = runtime->contract;

    contract_enter_phase(contract, MMUKO_BOOT_PHASE_PREPARE);
    printf("[CONTRACT] magic=0x%08X signature=0x%08X stage=%u current=%s\n",
           contract->magic,
           contract->signature,
           contract->stage.stage_id,
           mmuko_phase_name((mmuko_boot_phase_id)contract->stage.current_phase));

    if (!mmuko_boot_contract_is_valid(contract)) {
        puts("[CONTRACT] invalid handoff record");
        contract_set_status(contract, MMUKO_BOOT_STATUS_FAULT, 0xE1u);
        return MMUKO_BOOT_STATUS_FAULT;
    }

    if (contract->need_state.tripwire != 0u) {
        puts("[PHASE 0] tripwire present in handoff");
        contract_set_status(contract, MMUKO_BOOT_STATUS_ALERT, 0x21u);
        return MMUKO_BOOT_STATUS_ALERT;
    }

    contract->stage.flags |= MMUKO_CONTRACT_FLAG_NEEDS_VALID | MMUKO_CONTRACT_FLAG_RUNTIME_OWNS;
    mmuko_boot_contract_finalize(contract);
    printf("[PHASE 0] Vacuum medium initialized: G=%.4f\n", runtime->gravity);
    return MMUKO_BOOT_STATUS_HOLD;
}

static mmuko_boot_status phase1_cubit_init(MMUKO_Runtime *runtime)
{
    contract_enter_phase(runtime->contract, MMUKO_BOOT_PHASE_N);
    printf("[PHASE 1/N] Initializing %zu cubit rings from handoff layout\n", runtime->memory_size);

    for (size_t i = 0; i < runtime->memory_size; ++i) {
        MMUKO_Byte *byte = &runtime->memory_map[i];
        byte->base_index = (int)((byte->raw_value % 12u) + 1u);
        init_cubit_ring(byte);
        lookup_superposition(byte->base_index,
                             &byte->primary_superposition,
                             &byte->secondary_superposition);
    }

    return MMUKO_BOOT_STATUS_HOLD;
}

static mmuko_boot_status phase2_compass_alignment(MMUKO_Runtime *runtime)
{
    contract_enter_phase(runtime->contract, MMUKO_BOOT_PHASE_S);
    puts("[PHASE 2/S] Compass alignment");

    for (size_t b = 0; b < runtime->memory_size; ++b) {
        for (int i = 0; i < 8; ++i) {
            if (runtime->memory_map[b].cubit_ring[i].direction == UNDEFINED_DIR) {
                contract_set_status(runtime->contract, MMUKO_BOOT_STATUS_FAULT, 0x31u);
                return MMUKO_BOOT_STATUS_FAULT;
            }
        }
    }

    return MMUKO_BOOT_STATUS_HOLD;
}

static State flip_state(State state)
{
    switch (state) {
        case UP: return DOWN;
        case DOWN: return UP;
        case CHARM: return STRANGE;
        case STRANGE: return CHARM;
        case LEFT: return RIGHT;
        case RIGHT: return LEFT;
        default: return state;
    }
}

static mmuko_boot_status phase3_superposition_entanglement(MMUKO_Runtime *runtime)
{
    contract_enter_phase(runtime->contract, MMUKO_BOOT_PHASE_I_IDENT);
    puts("[PHASE 3/I_IDENT] Resolving entangled cubits");

    for (size_t b = 0; b < runtime->memory_size; ++b) {
        MMUKO_Byte *byte = &runtime->memory_map[b];
        for (int i = 0; i < 8; ++i) {
            Cubit *cubit = &byte->cubit_ring[i];
            if (cubit->superposed && cubit->entangled_with >= 0) {
                Cubit *partner = &byte->cubit_ring[cubit->entangled_with];
                if (cubit->state == partner->state) {
                    partner->state = flip_state(partner->state);
                }
            }
        }
    }

    return MMUKO_BOOT_STATUS_HOLD;
}

static mmuko_boot_status phase4_frame_centering(MMUKO_Runtime *runtime)
{
    contract_enter_phase(runtime->contract, MMUKO_BOOT_PHASE_G);
    lookup_superposition(6, &runtime->frame_of_reference, &runtime->memory_map[0].secondary_superposition);
    printf("[PHASE 4/G] Frame of reference set to %s\n",
           direction_to_string(runtime->frame_of_reference));
    return MMUKO_BOOT_STATUS_HOLD;
}

static mmuko_boot_status phase5_probe(MMUKO_Runtime *runtime)
{
    static const int boot_order[] = {12, 6, 8, 4, 10, 2, 1};
    contract_enter_phase(runtime->contract, MMUKO_BOOT_PHASE_I_PROBE);
    puts("[PHASE 5/I_PROBE] Nonlinear index resolution");

    for (size_t i = 0; i < sizeof(boot_order) / sizeof(boot_order[0]); ++i) {
        Direction primary;
        Direction secondary;
        lookup_superposition(boot_order[i], &primary, &secondary);
        printf("  base %d -> %s/%s\n",
               boot_order[i],
               direction_to_string(primary),
               direction_to_string(secondary));
    }

    if (runtime->contract->need_state.tier1 == 0u || runtime->contract->need_state.tier2 == 0u) {
        contract_set_status(runtime->contract, MMUKO_BOOT_STATUS_HOLD, 0x41u);
        return MMUKO_BOOT_STATUS_HOLD;
    }

    return MMUKO_BOOT_STATUS_PASS;
}

static mmuko_boot_status phase6_integrity(MMUKO_Runtime *runtime)
{
    contract_enter_phase(runtime->contract, MMUKO_BOOT_PHASE_I_INTEG);
    puts("[PHASE 6/I_INTEG] Rotation freedom check");

    for (size_t b = 0; b < runtime->memory_size; ++b) {
        for (int i = 0; i < 8; ++i) {
            uint8_t original = runtime->memory_map[b].cubit_ring[i].value;
            uint8_t rotated = rotate_bits(rotate_bits(original, 4), 4);
            if (rotated != original) {
                contract_set_status(runtime->contract, MMUKO_BOOT_STATUS_FAULT, 0x61u);
                return MMUKO_BOOT_STATUS_FAULT;
            }
        }
    }

    runtime->contract->stage.flags |= MMUKO_CONTRACT_FLAG_ROTATION_OK;
    mmuko_boot_contract_finalize(runtime->contract);
    return MMUKO_BOOT_STATUS_PASS;
}

static mmuko_boot_status mmuko_boot(MMUKO_Runtime *runtime)
{
    mmuko_boot_status status = phase_prepare(runtime);
    if (status == MMUKO_BOOT_STATUS_ALERT || status == MMUKO_BOOT_STATUS_FAULT) return status;

    status = phase1_cubit_init(runtime);
    if (status == MMUKO_BOOT_STATUS_FAULT) return status;

    status = phase2_compass_alignment(runtime);
    if (status == MMUKO_BOOT_STATUS_FAULT) return status;

    status = phase3_superposition_entanglement(runtime);
    if (status == MMUKO_BOOT_STATUS_FAULT) return status;

    status = phase4_frame_centering(runtime);
    if (status == MMUKO_BOOT_STATUS_FAULT) return status;

    status = phase5_probe(runtime);
    if (status != MMUKO_BOOT_STATUS_PASS) {
        contract_enter_phase(runtime->contract, MMUKO_BOOT_PHASE_HANDOFF);
        contract_set_status(runtime->contract, status, 0x51u);
        return status;
    }

    status = phase6_integrity(runtime);
    if (status != MMUKO_BOOT_STATUS_PASS) return status;

    contract_enter_phase(runtime->contract, MMUKO_BOOT_PHASE_HANDOFF);
    contract_set_status(runtime->contract, MMUKO_BOOT_STATUS_PASS, 0x70u);
    contract_enter_phase(runtime->contract, MMUKO_BOOT_PHASE_COMPLETE);
    runtime->contract->stage.stage_id = MMUKO_BOOT_RUNTIME_ID;
    contract_set_status(runtime->contract, MMUKO_BOOT_STATUS_PASS, 0x7Fu);
    return MMUKO_BOOT_STATUS_PASS;
}

static void seed_demo_contract(mmuko_boot_contract_record *contract)
{
    memset(contract, 0, sizeof(*contract));
    contract->magic = MMUKO_BOOT_CONTRACT_MAGIC;
    contract->signature = MMUKO_BOOT_CONTRACT_SIGNATURE;
    contract->stage.version = MMUKO_BOOT_CONTRACT_VERSION;
    contract->stage.stage_id = MMUKO_BOOT_STAGE2_ID;
    contract->stage.current_phase = MMUKO_BOOT_PHASE_N;
    contract->stage.last_phase = MMUKO_BOOT_PHASE_PREPARE;
    contract->need_state.tier1 = 1u;
    contract->need_state.tier2 = 1u;
    contract->need_state.tripwire = 0u;
    contract->need_state.threshold = 240u;
    contract->need_state.discriminant_hint = 1u;
    contract->layout.memory_base = MMUKO_BOOT_CONTRACT_PHYS_ADDR;
    contract->layout.memory_bytes = 16u;
    contract->layout.entry_offset = 0x7C00u;
    contract->layout.payload_bytes = 16u;
    contract->status = MMUKO_BOOT_STATUS_HOLD;
    contract->status_reason = 0x10u;
    mmuko_boot_contract_finalize(contract);
}

int main(void)
{
    mmuko_boot_contract_record contract;
    seed_demo_contract(&contract);

    printf("MMUKO OS Boot Contract Runtime v%s\n", MMUKO_VERSION);
    printf("Using shared contract at 0x%04X:%04X (%u bytes)\n\n",
           MMUKO_BOOT_CONTRACT_SEGMENT,
           MMUKO_BOOT_CONTRACT_OFFSET,
           (unsigned)sizeof(contract));

    MMUKO_Runtime *runtime = mmuko_runtime_create(&contract);
    if (!runtime) {
        fputs("Failed to create MMUKO runtime\n", stderr);
        return 1;
    }

    mmuko_boot_status status = mmuko_boot(runtime);
    printf("\n[RESULT] status=%s reason=0x%02X phase=%s frame=%s checksum=0x%02X\n",
           mmuko_status_name(status),
           contract.status_reason,
           mmuko_phase_name((mmuko_boot_phase_id)contract.stage.current_phase),
           direction_to_string(runtime->frame_of_reference),
           contract.checksum);

    mmuko_runtime_destroy(runtime);
    return status == MMUKO_BOOT_STATUS_PASS ? 0 : 1;
}
