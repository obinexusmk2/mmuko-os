// ============================================================
// BOOT_HELPERS.C — Shared boot utility implementations
// Used by: phase1 through phase6
// ============================================================

#include "boot_helpers.h"

// ─── Superposition lookup table ──────────────────────────────
typedef struct { int base; Direction primary; Direction secondary; } SuperpositionEntry;

static const SuperpositionEntry superposition_table[] = {
    {12, S,  N},
    {10, SE, N},
    {8,  E,  W},
    {6,  SW, E},
    {4,  W,  E},
    {2,  NE, W},
    {1,  N,  S}
};
#define SUPERPOSITION_TABLE_SIZE (sizeof(superposition_table)/sizeof(superposition_table[0]))

// ─── Compass spin values (radians) ───────────────────────────
static const double spin_values[] = {
    PI/4.0, PI/3.0, PI/2.0, PI,
    PI*2.0, PI/2.0, PI/3.0, PI/4.0
};

// ─── Entanglement pairs (mirror positions in 8-cubit ring) ───
static const int entangled_pairs[] = {7, 6, 5, -1, -1, 2, 1, 0};

// ─── Direction / state name tables ───────────────────────────
static const char* direction_names[] = {
    "NORTH","NORTHEAST","EAST","SOUTHEAST",
    "SOUTH","SOUTHWEST","WEST","NORTHWEST","UNDEFINED"
};
static const char* state_names[] = {"UP","DOWN","CHARM","STRANGE","LEFT","RIGHT"};

// ─────────────────────────────────────────────────────────────

const char* direction_to_string(Direction dir) {
    if (dir >= 0 && dir <= UNDEFINED_DIR) return direction_names[dir];
    return "INVALID";
}

const char* state_to_string(State state) {
    if (state >= 0 && state <= RIGHT) return state_names[state];
    return "INVALID";
}

int round_to_even_base(int base) {
    static const int valid_bases[] = {12, 10, 8, 6, 4, 2, 1};
    int nearest  = valid_bases[0];
    int min_diff = base > nearest ? base - nearest : nearest - base;
    for (size_t i = 1; i < sizeof(valid_bases)/sizeof(valid_bases[0]); i++) {
        int candidate = valid_bases[i];
        int diff      = base > candidate ? base - candidate : candidate - base;
        if (diff < min_diff) { min_diff = diff; nearest = candidate; }
    }
    return nearest;
}

void lookup_superposition(int base, Direction* primary, Direction* secondary) {
    for (size_t i = 0; i < SUPERPOSITION_TABLE_SIZE; i++) {
        if (superposition_table[i].base == base) {
            *primary   = superposition_table[i].primary;
            *secondary = superposition_table[i].secondary;
            return;
        }
    }
    int nearest = round_to_even_base(base);
    for (size_t i = 0; i < SUPERPOSITION_TABLE_SIZE; i++) {
        if (superposition_table[i].base == nearest) {
            *primary   = superposition_table[i].primary;
            *secondary = superposition_table[i].secondary;
            return;
        }
    }
    *primary   = N;
    *secondary = S;
}

int get_middle_base(void) { return 12 / 2; }

uint8_t rotate_bits(uint8_t value, int n) {
    n %= 8;
    if (n == 0) return value;
    return (uint8_t)(((value >> n) | (value << (8 - n))) & 0xFFu);
}

uint8_t bit_shift_semantic(uint8_t value, ShiftOp op, int n) {
    switch (op) {
        case RSHIFT: return (uint8_t)(value >> n);
        case LSHIFT: return (uint8_t)(value << n);
        case ROTATE: return rotate_bits(value, n);
        default:     return value;
    }
}

State resolve_state(int index, uint8_t byte_val) {
    uint8_t bit      = (uint8_t)((byte_val >> index) & 1u);
    uint8_t neighbor = (uint8_t)((byte_val >> ((index + 1) % 8)) & 1u);
    if (bit == 1u && neighbor == 1u) return UP;
    if (bit == 1u && neighbor == 0u) return CHARM;
    if (bit == 0u && neighbor == 1u) return STRANGE;
    return DOWN;
}

State flip_state(State state) {
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

void init_cubit_ring(MMUKO_Byte* byte) {
    static const Direction directions[] = {N, NE, E, SE, S, SW, W, NW};
    for (int i = 0; i < 8; i++) {
        Cubit* cubit       = &byte->cubit_ring[i];
        cubit->index       = i;
        cubit->value       = (uint8_t)((byte->raw_value >> i) & 1u);
        cubit->spin        = spin_values[i];
        cubit->direction   = directions[i];
        cubit->state       = resolve_state(i, byte->raw_value);
        cubit->entangled_with = entangled_pairs[i];
        cubit->superposed  = entangled_pairs[i] != -1;
    }
}

Cubit* get_cubit_from_byte(MMUKO_Byte* byte, int index) {
    if (index < 0 || index >= 8) return NULL;
    return &byte->cubit_ring[index];
}

Direction resolve_direction_from_neighbors(MMUKO_Byte* byte, int cubit_index) {
    int dir_count[8] = {0};
    int max_count    = 0;
    Direction max_dir = N;
    for (int delta = -1; delta <= 1; delta++) {
        if (delta == 0) continue;
        int neighbor_idx  = (cubit_index + delta + 8) % 8;
        Direction ndir    = byte->cubit_ring[neighbor_idx].direction;
        if (ndir == UNDEFINED_DIR) continue;
        dir_count[ndir]++;
        if (dir_count[ndir] > max_count) { max_count = dir_count[ndir]; max_dir = ndir; }
    }
    return max_count == 0 ? N : max_dir;
}

void set_frame_of_reference(MMUKO_System* sys, Direction center_dir) {
    sys->frame_of_reference = center_dir;
}
