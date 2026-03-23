// ============================================================
// MMUKO-BOOT.C — Portable MMUKO boot core and desktop simulator
// Project: OBINexus / OBIELF R&D
// ============================================================

#include "boot_contract.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef MMUKO_BOOT_DESKTOP_SIM
#include <stdio.h>
#endif

#define PI 3.14159265358979
#define MMUKO_VERSION "0.2-portable"
#define G_VACUUM 9.8
#define G_LEPTON (G_VACUUM / 10.0)
#define G_MUON   (G_LEPTON / 10.0)
#define G_DEEP   (G_MUON / 10.0)

typedef enum {
    N, NE, E, SE, S, SW, W, NW, UNDEFINED_DIR
} Direction;

typedef enum {
    UP, DOWN, CHARM, STRANGE, LEFT, RIGHT
} State;

typedef enum {
    RSHIFT, LSHIFT, ROTATE
} ShiftOp;

typedef enum {
    BOOT_OK,
    BOOT_LOCK_DETECTED,
    BOOT_ROTATION_LOCK,
    BOOT_UNDEFINED_DIRECTION,
    BOOT_FAILED
} BootStatus;

typedef struct {
    int index;
    uint8_t value;
    double spin;
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
    double gravity;
    double air;
    double water;
} VacuumMedium;

typedef struct {
    MMUKO_Byte *memory_map;
    size_t memory_size;
    double gravity;
    Direction frame_of_reference;
    bool boot_complete;
    mmuko_boot_contract_t *contract;
    uint8_t current_phase;
} MMUKO_System;

typedef struct {
    int base;
    Direction primary;
    Direction secondary;
} SuperpositionEntry;

static const SuperpositionEntry superposition_table[] = {
    {12, S, N},
    {10, SE, N},
    {8,  E, W},
    {6,  SW, E},
    {4,  W, E},
    {2,  NE, W},
    {1,  N, S}
};

#define SUPERPOSITION_TABLE_SIZE (sizeof(superposition_table) / sizeof(superposition_table[0]))

static const char *direction_names[] = {
    "NORTH", "NORTHEAST", "EAST", "SOUTHEAST",
    "SOUTH", "SOUTHWEST", "WEST", "NORTHWEST", "UNDEFINED"
};

static const char *state_names[] = {
    "UP", "DOWN", "CHARM", "STRANGE", "LEFT", "RIGHT"
};

static const double spin_values[] = {
    PI / 4.0, PI / 3.0, PI / 2.0, PI,
    PI * 2.0, PI / 2.0, PI / 3.0, PI / 4.0
};

static const int entangled_pairs[] = {7, 6, 5, -1, -1, 2, 1, 0};

static const char *direction_to_string(Direction dir)
{
    if (dir >= 0 && dir <= UNDEFINED_DIR) {
        return direction_names[dir];
    }
    return "INVALID";
}

static const char *state_to_string(State state)
{
    if (state >= 0 && state <= RIGHT) {
        return state_names[state];
    }
    return "INVALID";
}

static void mmuko_contract_mark_phase(MMUKO_System *sys, uint8_t phase)
{
    sys->current_phase = phase;
    if (sys->contract) {
        sys->contract->membrane_phase = phase;
    }
}

static void mmuko_contract_mark_outcome(MMUKO_System *sys,
                                        mmuko_membrane_outcome_t outcome,
                                        mmuko_transfer_state_t transfer)
{
    if (!sys->contract) {
        return;
    }

    sys->contract->membrane_outcome = (uint8_t)outcome;
    sys->contract->transfer_state = (uint8_t)transfer;
    sys->contract->boot_flags |= MMUKO_BOOT_FLAG_NSIGII_READY;
}

static VacuumMedium init_vacuum_medium(void)
{
    return (VacuumMedium){
        .gravity = G_VACUUM,
        .air = 0.0,
        .water = 0.0
    };
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

static void init_cubit_ring(MMUKO_Byte *byte)
{
    static const Direction directions[] = {N, NE, E, SE, S, SW, W, NW};

    for (int i = 0; i < 8; i++) {
        Cubit *cubit = &byte->cubit_ring[i];
        cubit->index = i;
        cubit->value = (uint8_t)((byte->raw_value >> i) & 1u);
        cubit->spin = spin_values[i];
        cubit->direction = directions[i];
        cubit->state = resolve_state(i, byte->raw_value);
        cubit->entangled_with = entangled_pairs[i];
        cubit->superposed = entangled_pairs[i] != -1;
    }
}

static int round_to_even_base(int base)
{
    static const int valid_bases[] = {12, 10, 8, 6, 4, 2, 1};
    int nearest = valid_bases[0];
    int min_diff = base > valid_bases[0] ? base - valid_bases[0] : valid_bases[0] - base;

    for (size_t i = 1; i < sizeof(valid_bases) / sizeof(valid_bases[0]); i++) {
        int candidate = valid_bases[i];
        int diff = base > candidate ? base - candidate : candidate - base;
        if (diff < min_diff) {
            min_diff = diff;
            nearest = candidate;
        }
    }

    return nearest;
}

static void lookup_superposition(int base, Direction *primary, Direction *secondary)
{
    for (size_t i = 0; i < SUPERPOSITION_TABLE_SIZE; i++) {
        if (superposition_table[i].base == base) {
            *primary = superposition_table[i].primary;
            *secondary = superposition_table[i].secondary;
            return;
        }
    }

    int nearest = round_to_even_base(base);
    for (size_t i = 0; i < SUPERPOSITION_TABLE_SIZE; i++) {
        if (superposition_table[i].base == nearest) {
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

static uint8_t bit_shift_semantic(uint8_t value, ShiftOp op, int n)
{
    switch (op) {
        case RSHIFT: return (uint8_t)(value >> n);
        case LSHIFT: return (uint8_t)(value << n);
        case ROTATE: return rotate_bits(value, n);
        default:     return value;
    }

static Direction resolve_direction_from_neighbors(MMUKO_Byte *byte, int cubit_index)
{
    int dir_count[8] = {0};
    int max_count = 0;
    Direction max_dir = N;

    for (int delta = -1; delta <= 1; delta++) {
        if (delta == 0) {
            continue;
        }

        int neighbor_idx = (cubit_index + delta + 8) % 8;
        Direction neighbor_dir = byte->cubit_ring[neighbor_idx].direction;
        if (neighbor_dir == UNDEFINED_DIR) {
            continue;
        }

        dir_count[neighbor_dir]++;
        if (dir_count[neighbor_dir] > max_count) {
            max_count = dir_count[neighbor_dir];
            max_dir = neighbor_dir;
        }
    }

    return max_count == 0 ? N : max_dir;
}

static State flip_state(State state)
{
    switch (state) {
        case UP:      return DOWN;
        case DOWN:    return UP;
        case CHARM:   return STRANGE;
        case STRANGE: return CHARM;
        case LEFT:    return RIGHT;
        case RIGHT:   return LEFT;
        default:      return state;
    }
}

static Cubit *get_cubit_from_byte(MMUKO_Byte *byte, int index)
{
    if (index < 0 || index >= 8) {
        return NULL;
    }
    return &byte->cubit_ring[index];
}

static int get_middle_base(void)
{
    return 12 / 2;
}

static void set_frame_of_reference(MMUKO_System *sys, Direction center_dir)
{
    sys->frame_of_reference = center_dir;
}

static uint8_t seed_raw_value_from_contract(const mmuko_boot_contract_t *contract, size_t index)
{
    uint8_t seeded = (uint8_t)(index * 17u + 42u);

    if (!contract) {
        return seeded;
    }

    seeded ^= (uint8_t)(contract->boot_flags & 0xFFu);
    seeded ^= contract->membrane_outcome;

    if (contract->keyboard.length > 0) {
        seeded ^= (uint8_t)contract->keyboard.bytes[index % contract->keyboard.length];
    }

    return bit_shift_semantic(seeded, ROTATE, (int)(index % 8u));
}

static void mmuko_seed_memory_map(MMUKO_System *sys)
{
    for (size_t i = 0; i < sys->memory_size; i++) {
        sys->memory_map[i].raw_value = seed_raw_value_from_contract(sys->contract, i);
    }
}

static void mmuko_boot_bind_contract(MMUKO_System *sys,
                                     mmuko_boot_contract_t *contract,
                                     mmuko_transfer_state_t mode)
{
    sys->contract = contract;
    sys->current_phase = 0;

    if (!contract) {
        return;
    }

    if (contract->magic != MMUKO_BOOT_CONTRACT_MAGIC ||
        contract->total_size != sizeof(mmuko_boot_contract_t)) {
        mmuko_boot_contract_reset(contract);
    }

    contract->transfer_state = (uint8_t)mode;
    contract->boot_flags |= MMUKO_BOOT_FLAG_NATIVE_C_READY;
    contract->membrane_outcome = MMUKO_MEMBRANE_HOLD;
}

static void mmuko_system_init(MMUKO_System *sys,
                              MMUKO_Byte *storage,
                              size_t memory_size,
                              mmuko_boot_contract_t *contract,
                              mmuko_transfer_state_t mode)
{
    memset(sys, 0, sizeof(*sys));
    sys->memory_map = storage;
    sys->memory_size = memory_size;
    sys->frame_of_reference = N;
    mmuko_boot_bind_contract(sys, contract, mode);
    mmuko_seed_memory_map(sys);
}

static BootStatus phase1_cubit_init(MMUKO_System *sys)
{
    mmuko_contract_mark_phase(sys, 1);

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

static BootStatus phase2_compass_alignment(MMUKO_System *sys)
{
    mmuko_contract_mark_phase(sys, 2);

    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte *byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            Cubit *cubit = &byte->cubit_ring[i];
            if (cubit->direction != UNDEFINED_DIR) {
                continue;
            }

            cubit->direction = resolve_direction_from_neighbors(byte, i);
            if (cubit->direction == UNDEFINED_DIR) {
                mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_ALERT, MMUKO_TRANSFER_NATIVE_C_ENTRY);
                return BOOT_LOCK_DETECTED;
            }
        }
    }

    return BOOT_OK;
}

static BootStatus phase3_superposition_entanglement(MMUKO_System *sys)
{
    mmuko_contract_mark_phase(sys, 3);

    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte *byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            Cubit *cubit = &byte->cubit_ring[i];
            if (!cubit->superposed || cubit->entangled_with == -1) {
                continue;
            }

            Cubit *partner = get_cubit_from_byte(byte, cubit->entangled_with);
            if (partner && cubit->state == partner->state) {
                partner->state = flip_state(partner->state);
            }
        }
    }

    return BOOT_OK;
}

static BootStatus phase4_frame_centering(MMUKO_System *sys)
{
    mmuko_contract_mark_phase(sys, 4);

    Direction primary;
    Direction secondary;
    lookup_superposition(get_middle_base(), &primary, &secondary);
    set_frame_of_reference(sys, primary);

    for (size_t b = 0; b < sys->memory_size; b++) {
        sys->memory_map[b].primary_superposition = primary;
        sys->memory_map[b].secondary_superposition = secondary;
    }

    return BOOT_OK;
}

static void resolve_base_state(MMUKO_System *sys, int base)
{
    Direction primary;
    Direction secondary;
    lookup_superposition(base, &primary, &secondary);

    for (size_t i = 0; i < sys->memory_size; i++) {
        if (sys->memory_map[i].base_index == base) {
            sys->memory_map[i].primary_superposition = primary;
            sys->memory_map[i].secondary_superposition = secondary;
        }
    }
}

static BootStatus phase5_nonlinear_resolution(MMUKO_System *sys)
{
    static const int boot_order[] = {12, 6, 8, 4, 10, 2, 1};

    mmuko_contract_mark_phase(sys, 5);
    for (size_t i = 0; i < sizeof(boot_order) / sizeof(boot_order[0]); i++) {
        resolve_base_state(sys, boot_order[i]);
    }

    return BOOT_OK;
}

static BootStatus phase6_rotation_verification(MMUKO_System *sys)
{
    mmuko_contract_mark_phase(sys, 6);

    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte *byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            uint8_t original = byte->cubit_ring[i].value;
            uint8_t test_val = rotate_bits(original, 4);
            test_val = rotate_bits(test_val, 4);
            if (test_val != original) {
                mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_ALERT, MMUKO_TRANSFER_NATIVE_C_ENTRY);
                return BOOT_ROTATION_LOCK;
            }
        }
    }

    return BOOT_OK;
}

static BootStatus mmuko_boot(MMUKO_System *sys)
{
    BootStatus status;

    mmuko_contract_mark_phase(sys, 0);
    sys->medium = init_vacuum_medium();

    status = phase1_cubit_init(sys);
    if (status != BOOT_OK) return status;
    status = phase2_compass_alignment(sys);
    if (status != BOOT_OK) return status;
    status = phase3_superposition_entanglement(sys);
    if (status != BOOT_OK) return status;
    status = phase4_frame_centering(sys);
    if (status != BOOT_OK) return status;
    status = phase5_nonlinear_resolution(sys);
    if (status != BOOT_OK) return status;
    status = phase6_rotation_verification(sys);
    if (status != BOOT_OK) return status;

    sys->boot_complete = true;
    mmuko_contract_mark_phase(sys, 7);
    mmuko_contract_mark_outcome(sys, MMUKO_MEMBRANE_PASS, MMUKO_TRANSFER_KERNEL_ENTRY);
    return BOOT_OK;
}

#ifdef MMUKO_BOOT_DESKTOP_SIM
static void mmuko_print_cubit_state(const MMUKO_System *sys, size_t byte_idx, int cubit_idx)
{
    if (byte_idx >= sys->memory_size || cubit_idx < 0 || cubit_idx >= 8) {
        return;
    }

    const Cubit *cubit = &sys->memory_map[byte_idx].cubit_ring[cubit_idx];
    printf("Byte[%zu].Cubit[%d]: val=%u dir=%s state=%s spin=%.4f super=%s ent=%d\n",
           byte_idx,
           cubit_idx,
           cubit->value,
           direction_to_string(cubit->direction),
           state_to_string(cubit->state),
           cubit->spin,
           cubit->superposed ? "YES" : "NO",
           cubit->entangled_with);
}

int main(int argc, char **argv)
{
    mmuko_boot_contract_t contract;
    MMUKO_System sys;
    MMUKO_Byte memory[16];

    mmuko_boot_contract_reset(&contract);
    contract.boot_flags = MMUKO_BOOT_FLAG_STAGE2_MODE | MMUKO_BOOT_FLAG_NATIVE_C_READY;
    contract.transfer_state = MMUKO_TRANSFER_STAGE2_READY;

    if (argc > 1) {
        if (mmuko_keyboard_buffer_copy_text(&contract.keyboard, argv[1]) > 0) {
            contract.boot_flags |= MMUKO_BOOT_FLAG_KEYBOARD_REQUIRED | MMUKO_BOOT_FLAG_KEYBOARD_PRESENT;
        }
    }

    mmuko_system_init(&sys, memory, sizeof(memory) / sizeof(memory[0]), &contract, MMUKO_TRANSFER_STAGE2_READY);

    printf("MMUKO OS Boot Loader (%s)\n", MMUKO_VERSION);
    printf("Contract @ 0x%04X, keyboard bytes=%u\n\n",
           MMUKO_BOOT_CONTRACT_ADDR,
           contract.keyboard.length);

    BootStatus status = mmuko_boot(&sys);
    if (status != BOOT_OK) {
        printf("BOOT FAILED: status=%d phase=%u outcome=0x%02X\n",
               status,
               contract.membrane_phase,
               contract.membrane_outcome);
        return 1;
    }

    printf("BOOT COMPLETE\n");
    printf("Transfer state=%u frame=%s outcome=0x%02X\n",
           contract.transfer_state,
           direction_to_string(sys.frame_of_reference),
           contract.membrane_outcome);
    printf("Gravity medium: G=%.4f (lepton=%.4f, muon=%.4f, deep=%.4f)\n",
           G_VACUUM, G_LEPTON, G_MUON, G_DEEP);
    mmuko_print_cubit_state(&sys, 0, 0);
    mmuko_print_cubit_state(&sys, 0, 2);
    mmuko_print_cubit_state(&sys, 5, 5);
    return 0;
}
#endif

// ============================================================
// END OF MMUKO-BOOT.C
// ============================================================
