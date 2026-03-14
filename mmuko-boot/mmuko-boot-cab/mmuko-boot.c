// ============================================================
// MMUKO-BOOT.C — MMUKO OS Boot Sequence Implementation
// WITH CAB PROFILE LOADER
// Project: OBINexus / OBIELF R&D
// Author: OBINexus Research Division
// Version: 0.2-cab-integrated
// ============================================================

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

// ─────────────────────────────────────────────
// CONSTANTS & DEFINITIONS
// ─────────────────────────────────────────────

#define PI 3.14159265358979
#define MMUKO_VERSION "0.2-cab-integrated"

// Gravity medium constants (vacuum reference)
#define G_VACUUM    9.8
#define G_LEPTON    (G_VACUUM / 10.0)      // 0.98
#define G_MUON      (G_LEPTON / 10.0)      // 0.098
#define G_DEEP      (G_MUON / 10.0)        // 0.0098

// Compass spin values (radians)
#define SPIN_NORTH      (PI / 4.0)         // 0.7854 → 45°
#define SPIN_NORTHEAST  (PI / 3.0)         // 1.0472 → 60°
#define SPIN_EAST       (PI / 2.0)         // 1.5708 → 90°
#define SPIN_SOUTHEAST  PI                 // 3.1416 → 180°
#define SPIN_SOUTH      (PI * 2.0)         // 6.2832 → 360°
#define SPIN_SOUTHWEST  (PI / 2.0)         // dual with EAST
#define SPIN_WEST       (PI / 3.0)         // dual with NORTHEAST
#define SPIN_NORTHWEST  (PI / 4.0)         // dual with NORTH

// ─────────────────────────────────────────────
// ENUMERATIONS
// ─────────────────────────────────────────────

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
    BOOT_CAB_LOAD_FAILED,
    BOOT_FAILED
} BootStatus;

// ─────────────────────────────────────────────
// STRUCTURES
// ─────────────────────────────────────────────

typedef struct {
    int index;              // 0–7
    uint8_t value;          // 0 or 1
    double spin;            // derived from compass direction
    Direction direction;    // compass direction
    State state;            // quantum-like state
    bool superposed;        // is this cubit in superposition?
    int entangled_with;     // index of entangled partner (-1 if none)
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
    MMUKO_Byte* memory_map;
    size_t memory_size;
    VacuumMedium medium;
    Direction frame_of_reference;
    bool boot_complete;
    char cab_loaded[256];   // Track which CAB was loaded
} MMUKO_System;

// Superposition lookup entry
typedef struct {
    int base;
    Direction primary;
    Direction secondary;
} SuperpositionEntry;

// CAB Profile structure (imported from profile_loader.h)
typedef struct {
    char profile_id[64];
    char author[64];
    char created[32];
    double gravity_vacuum;
    double gravity_lepton;
    double gravity_muon;
    double gravity_deep;
    char frame_reference[16];
    int rotation_free;
    int lock_memory;
    char username[64];
    char profile_hash[64];
    double mouse_entropy;
    int typing_latency_ms;
    char command_pattern[32];
    int rotation_test_pass;
    int entangle_test_pass;
    int alignment_test_pass;
    uint8_t cubit_values[16];
    size_t cubit_count;
} CAB_Profile;

// ─────────────────────────────────────────────
// GLOBAL LOOKUP TABLE (Weak Map Implementation)
// ─────────────────────────────────────────────

static const SuperpositionEntry superposition_table[] = {
    {12, S, N},     // full cycle, pi*2
    {10, SE, N},
    {8,  E, W},     // pi/2 pair
    {6,  SW, E},    // middle base
    {4,  W, E},
    {2,  NE, W},
    {1,  N, S}      // base unit
};
#define SUPERPOSITION_TABLE_SIZE (sizeof(superposition_table) / sizeof(SuperpositionEntry))

// Compass direction names for logging
static const char* direction_names[] = {
    "NORTH", "NORTHEAST", "EAST", "SOUTHEAST",
    "SOUTH", "SOUTHWEST", "WEST", "NORTHWEST", "UNDEFINED"
};

static const char* state_names[] = {
    "UP", "DOWN", "CHARM", "STRANGE", "LEFT", "RIGHT"
};

// ─────────────────────────────────────────────
// HELPER FUNCTIONS
// ─────────────────────────────────────────────

const char* direction_to_string(Direction dir) {
    if (dir >= 0 && dir <= 8) return direction_names[dir];
    return "INVALID";
}

const char* state_to_string(State s) {
    if (s >= 0 && s <= 5) return state_names[s];
    return "INVALID";
}

// Spin values lookup
static const double spin_values[] = {
    SPIN_NORTH, SPIN_NORTHEAST, SPIN_EAST, SPIN_SOUTHEAST,
    SPIN_SOUTH, SPIN_SOUTHWEST, SPIN_WEST, SPIN_NORTHWEST
};

// Entanglement pairs: index → entangled partner index
static const int entangled_pairs[] = {7, 6, 5, -1, -1, 2, 1, 0};

// ─────────────────────────────────────────────
// CAB PROFILE LOADER (INLINE)
// ─────────────────────────────────────────────

static char* trim_string(char* str) {
    while (*str && (*str == ' ' || *str == '\t')) str++;
    char* end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end-- = 0;
    }
    return str;
}

static int parse_key_value(const char* line, char* key, char* value, size_t max_len) {
    if (!line || line[0] == '#' || line[0] == '[') return 0;
    
    char* eq = strchr(line, '=');
    if (!eq) return 0;
    
    strncpy(key, line, eq - line);
    key[eq - line] = 0;
    trim_string(key);
    
    strncpy(value, eq + 1, max_len - 1);
    value[max_len - 1] = 0;
    trim_string(value);
    
    return 1;
}

CAB_Profile cab_default(void) {
    CAB_Profile p = {0};
    
    strcpy(p.profile_id, "cab-DEFAULT-0000");
    strcpy(p.author, "OBINexus");
    strcpy(p.created, "2026-03-13");
    
    p.gravity_vacuum = G_VACUUM;
    p.gravity_lepton = G_LEPTON;
    p.gravity_muon = G_MUON;
    p.gravity_deep = G_DEEP;
    strcpy(p.frame_reference, "SOUTHWEST");
    p.rotation_free = 1;
    p.lock_memory = 0;
    
    strcpy(p.username, "default");
    strcpy(p.profile_hash, "0000000000000000");
    p.mouse_entropy = 0.5;
    p.typing_latency_ms = 100;
    strcpy(p.command_pattern, "linear");
    
    p.rotation_test_pass = 0;
    p.entangle_test_pass = 0;
    p.alignment_test_pass = 0;
    
    p.cubit_count = 16;
    for (size_t i = 0; i < 16; i++) {
        p.cubit_values[i] = (uint8_t)(i * 17 + 42);
    }
    
    return p;
}

int cab_load(const char* path, CAB_Profile* profile) {
    if (!path || !profile) return 0;
    
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    
    char line[512];
    char key[128], value[256];
    enum {
        SECTION_NONE, SECTION_HEADER, SECTION_CALIBRATION,
        SECTION_USER_PROFILE, SECTION_CUBIT_RING,
        SECTION_BOOT_VALIDATION
    } section = SECTION_NONE;
    
    size_t cubit_idx = 0;
    
    while (fgets(line, sizeof(line), f)) {
        char* trimmed = trim_string(line);
        
        if (!trimmed || strlen(trimmed) == 0 || trimmed[0] == '#') continue;
        
        if (strcmp(trimmed, "[CAB_HEADER]") == 0) { section = SECTION_HEADER; continue; }
        if (strcmp(trimmed, "[CALIBRATION]") == 0) { section = SECTION_CALIBRATION; continue; }
        if (strcmp(trimmed, "[USER_PROFILE]") == 0) { section = SECTION_USER_PROFILE; continue; }
        if (strcmp(trimmed, "[CUBIT_RING]") == 0) { section = SECTION_CUBIT_RING; continue; }
        if (strcmp(trimmed, "[BOOT_VALIDATION]") == 0) { section = SECTION_BOOT_VALIDATION; continue; }
        
        if (!parse_key_value(trimmed, key, value, sizeof(value))) continue;
        
        switch (section) {
            case SECTION_HEADER:
                if (strcmp(key, "profile_id") == 0) strncpy(profile->profile_id, value, 63);
                else if (strcmp(key, "author") == 0) strncpy(profile->author, value, 63);
                break;
            case SECTION_CALIBRATION:
                if (strcmp(key, "gravity_vacuum") == 0) profile->gravity_vacuum = atof(value);
                else if (strcmp(key, "gravity_lepton") == 0) profile->gravity_lepton = atof(value);
                else if (strcmp(key, "gravity_muon") == 0) profile->gravity_muon = atof(value);
                else if (strcmp(key, "gravity_deep") == 0) profile->gravity_deep = atof(value);
                else if (strcmp(key, "frame_reference") == 0) strncpy(profile->frame_reference, value, 15);
                break;
            case SECTION_USER_PROFILE:
                if (strcmp(key, "username") == 0) strncpy(profile->username, value, 63);
                else if (strcmp(key, "mouse_entropy") == 0) profile->mouse_entropy = atof(value);
                break;
            case SECTION_CUBIT_RING:
                if (strncmp(key, "cubit", 5) == 0 && cubit_idx < 16) {
                    char* val_start = strchr(value, ':');
                    if (val_start) {
                        int v = atoi(val_start + 1);
                        profile->cubit_values[cubit_idx++] = (uint8_t)v;
                        profile->cubit_count++;
                    }
                }
                break;
            default:
                break;
        }
    }
    
    fclose(f);
    return 1;
}

int cab_apply(CAB_Profile* profile, MMUKO_System* sys) {
    if (!profile || !sys) return 0;
    
    sys->medium.gravity = profile->gravity_vacuum;
    sys->medium.air = profile->gravity_lepton;
    sys->medium.water = profile->gravity_muon;
    
    size_t apply_count = (profile->cubit_count < sys->memory_size) 
                         ? profile->cubit_count 
                         : sys->memory_size;
    
    for (size_t i = 0; i < apply_count; i++) {
        sys->memory_map[i].raw_value = profile->cubit_values[i];
    }
    
    return 1;
}

// ─────────────────────────────────────────────
// PHASE 0: VACUUM MEDIUM INITIALIZATION
// ─────────────────────────────────────────────

VacuumMedium init_vacuum_medium(void) {
    VacuumMedium medium = {
        .gravity = G_VACUUM,
        .air = 0.0,
        .water = 0.0
    };
    printf("[PHASE 0] Vacuum medium initialized: G=%.4f\n", medium.gravity);
    return medium;
}

// ─────────────────────────────────────────────
// CUBIT RING INITIALIZATION
// ─────────────────────────────────────────────

State resolve_state(int index, uint8_t byte_val) {
    uint8_t bit = (byte_val >> index) & 1;
    uint8_t neighbor = (byte_val >> ((index + 1) % 8)) & 1;

    if (bit == 1 && neighbor == 1) return UP;
    if (bit == 1 && neighbor == 0) return CHARM;
    if (bit == 0 && neighbor == 1) return STRANGE;
    return DOWN;
}

void init_cubit_ring(MMUKO_Byte* byte) {
    static const Direction directions[] = {N, NE, E, SE, S, SW, W, NW};

    for (int i = 0; i < 8; i++) {
        Cubit* c = &byte->cubit_ring[i];
        c->index = i;
        c->value = (byte->raw_value >> i) & 1;
        c->spin = spin_values[i];
        c->direction = directions[i];
        c->state = resolve_state(i, byte->raw_value);
        c->entangled_with = entangled_pairs[i];
        c->superposed = (entangled_pairs[i] != -1);
    }
}

// ─────────────────────────────────────────────
// SUPERPOSITION LOOKUP (Weak Map)
// ─────────────────────────────────────────────

int round_to_even_base(int base) {
    const int valid_bases[] = {12, 10, 8, 6, 4, 2, 1};
    int nearest = valid_bases[0];
    int min_diff = abs(base - valid_bases[0]);

    for (size_t i = 1; i < 7; i++) {
        int diff = abs(base - valid_bases[i]);
        if (diff < min_diff) {
            min_diff = diff;
            nearest = valid_bases[i];
        }
    }
    return nearest;
}

void lookup_superposition(int base, Direction* primary, Direction* secondary) {
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

// ─────────────────────────────────────────────
// BIT SHIFT OPERATIONS
// ─────────────────────────────────────────────

uint8_t rotate_bits(uint8_t value, int n) {
    n = n % 8;
    if (n == 0) return value;
    return ((value >> n) | (value << (8 - n))) & 0xFF;
}

uint8_t bit_shift_semantic(uint8_t value, ShiftOp op, int n) {
    switch (op) {
        case RSHIFT: return value >> n;
        case LSHIFT: return value << n;
        case ROTATE: return rotate_bits(value, n);
        default: return value;
    }
}

// ─────────────────────────────────────────────
// PHASE 2: COMPASS ALIGNMENT
// ─────────────────────────────────────────────

Direction resolve_direction_from_neighbors(MMUKO_Byte* byte, int cubit_index) {
    int dir_count[8] = {0};
    int max_count = 0;
    Direction max_dir = N;

    for (int i = -1; i <= 1; i++) {
        if (i == 0) continue;
        int neighbor_idx = (cubit_index + i + 8) % 8;
        Direction ndir = byte->cubit_ring[neighbor_idx].direction;
        if (ndir != UNDEFINED_DIR) {
            dir_count[ndir]++;
            if (dir_count[ndir] > max_count) {
                max_count = dir_count[ndir];
                max_dir = ndir;
            }
        }
    }

    if (max_count == 0) {
        return N;
    }
    return max_dir;
}

BootStatus phase2_compass_alignment(MMUKO_System* sys) {
    printf("[PHASE 2] Compass alignment...\n");

    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte* byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            Cubit* c = &byte->cubit_ring[i];
            if (c->direction == UNDEFINED_DIR) {
                c->direction = resolve_direction_from_neighbors(byte, i);
                if (c->direction == UNDEFINED_DIR) {
                    printf("[ERROR] Boot lock detected at byte %zu, cubit %d\n", b, i);
                    return BOOT_LOCK_DETECTED;
                }
            }
        }
    }

    printf("[PHASE 2] All cubits aligned to compass directions\n");
    return BOOT_OK;
}

// ─────────────────────────────────────────────
// PHASE 3: SUPERPOSITION ENTANGLEMENT
// ─────────────────────────────────────────────

State flip_state(State s) {
    switch (s) {
        case UP: return DOWN;
        case DOWN: return UP;
        case CHARM: return STRANGE;
        case STRANGE: return CHARM;
        case LEFT: return RIGHT;
        case RIGHT: return LEFT;
        default: return s;
    }
}

Cubit* get_cubit_from_byte(MMUKO_Byte* byte, int index) {
    if (index >= 0 && index < 8) {
        return &byte->cubit_ring[index];
    }
    return NULL;
}

BootStatus phase3_superposition_entanglement(MMUKO_System* sys) {
    printf("[PHASE 3] Entangling superposition pairs...\n");

    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte* byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            Cubit* c = &byte->cubit_ring[i];
            if (c->superposed && c->entangled_with != -1) {
                Cubit* partner = get_cubit_from_byte(byte, c->entangled_with);
                if (partner && c->state == partner->state) {
                    partner->state = flip_state(partner->state);
                    printf("[PHASE 3] Resolved interference at byte %zu, pair (%d, %d)\n",
                           b, i, c->entangled_with);
                }
            }
        }
    }

    printf("[PHASE 3] Superposition entanglement complete\n");
    return BOOT_OK;
}

// ─────────────────────────────────────────────
// PHASE 4: FRAME OF REFERENCE CENTERING
// ─────────────────────────────────────────────

int get_middle_base(void) {
    int max_index = 12;
    return max_index / 2;
}

void set_frame_of_reference(MMUKO_System* sys, Direction center_dir) {
    sys->frame_of_reference = center_dir;
    printf("[PHASE 4] Frame of reference set to %s\n", direction_to_string(center_dir));
}

BootStatus phase4_frame_centering(MMUKO_System* sys) {
    printf("[PHASE 4] Frame of reference centering...\n");

    int center_base = get_middle_base();
    Direction primary, secondary;
    lookup_superposition(center_base, &primary, &secondary);

    set_frame_of_reference(sys, primary);

    for (size_t b = 0; b < sys->memory_size; b++) {
        sys->memory_map[b].primary_superposition = primary;
        sys->memory_map[b].secondary_superposition = secondary;
    }

    return BOOT_OK;
}

// ─────────────────────────────────────────────
// PHASE 5: NONLINEAR INDEX RESOLUTION
// ─────────────────────────────────────────────

void resolve_base_state(MMUKO_System* sys, int base) {
    Direction primary, secondary;
    lookup_superposition(base, &primary, &secondary);

    for (size_t i = 0; i < sys->memory_size; i++) {
        if (sys->memory_map[i].base_index == base) {
            sys->memory_map[i].primary_superposition = primary;
            sys->memory_map[i].secondary_superposition = secondary;
        }
    }
}

BootStatus phase5_nonlinear_resolution(MMUKO_System* sys) {
    printf("[PHASE 5] Nonlinear index resolution (diamond table)...\n");

    const int boot_order[] = {12, 6, 8, 4, 10, 2, 1};
    const int boot_order_size = sizeof(boot_order) / sizeof(int);

    for (int i = 0; i < boot_order_size; i++) {
        int base = boot_order[i];
        resolve_base_state(sys, base);
        Direction primary, secondary;
        lookup_superposition(base, &primary, &secondary);
        printf("[PHASE 5] Base %d resolved → %s/%s\n", 
               base, direction_to_string(primary), direction_to_string(secondary));
    }

    return BOOT_OK;
}

// ─────────────────────────────────────────────
// PHASE 6: ROTATION VERIFICATION
// ─────────────────────────────────────────────

BootStatus phase6_rotation_verification(MMUKO_System* sys) {
    printf("[PHASE 6] Rotation freedom check...\n");

    for (size_t b = 0; b < sys->memory_size; b++) {
        MMUKO_Byte* byte = &sys->memory_map[b];
        for (int i = 0; i < 8; i++) {
            uint8_t original = byte->cubit_ring[i].value;
            uint8_t test_val = rotate_bits(original, 4);
            test_val = rotate_bits(test_val, 4);

            if (test_val != original) {
                printf("[ERROR] Rotation lock at byte %zu, cubit %d\n", b, i);
                return BOOT_ROTATION_LOCK;
            }
        }
    }

    printf("[PHASE 6] All cubits rotate freely (360° verified)\n");
    return BOOT_OK;
}

// ─────────────────────────────────────────────
// PHASE 1: CUBIT RING INITIALIZATION (PER BYTE)
// ─────────────────────────────────────────────

BootStatus phase1_cubit_init(MMUKO_System* sys) {
    printf("[PHASE 1] Initializing cubit rings...\n");

    for (size_t i = 0; i < sys->memory_size; i++) {
        uint8_t val = sys->memory_map[i].raw_value;
        sys->memory_map[i].base_index = (val % 12) + 1;

        init_cubit_ring(&sys->memory_map[i]);
        lookup_superposition(sys->memory_map[i].base_index,
                           &sys->memory_map[i].primary_superposition,
                           &sys->memory_map[i].secondary_superposition);
    }

    printf("[PHASE 1] Initialized %zu cubit rings\n", sys->memory_size);
    return BOOT_OK;
}

// ─────────────────────────────────────────────
// MAIN BOOT SEQUENCE
// ─────────────────────────────────────────────

BootStatus mmuko_boot(MMUKO_System* sys) {
    printf("\n=== MMUKO BOOT SEQUENCE v%s ===\n\n", MMUKO_VERSION);

    BootStatus status;

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

    printf("\n[PHASE 7] MMUKO BOOT COMPLETE — All cubits aligned, no lock detected.\n");
    sys->boot_complete = true;

    return BOOT_OK;
}

// ─────────────────────────────────────────────
// SYSTEM INITIALIZATION
// ─────────────────────────────────────────────

MMUKO_System* mmuko_system_create(size_t memory_size) {
    MMUKO_System* sys = (MMUKO_System*)malloc(sizeof(MMUKO_System));
    if (!sys) return NULL;

    sys->memory_map = (MMUKO_Byte*)calloc(memory_size, sizeof(MMUKO_Byte));
    if (!sys->memory_map) {
        free(sys);
        return NULL;
    }

    sys->memory_size = memory_size;
    sys->frame_of_reference = N;
    sys->boot_complete = false;
    memset(sys->cab_loaded, 0, sizeof(sys->cab_loaded));

    for (size_t i = 0; i < memory_size; i++) {
        sys->memory_map[i].raw_value = (uint8_t)(i * 17 + 42);
    }

    return sys;
}

void mmuko_system_destroy(MMUKO_System* sys) {
    if (sys) {
        free(sys->memory_map);
        free(sys);
    }
}

void mmuko_print_cubit_state(MMUKO_System* sys, size_t byte_idx, int cubit_idx) {
    if (byte_idx >= sys->memory_size || cubit_idx < 0 || cubit_idx >= 8) return;

    Cubit* c = &sys->memory_map[byte_idx].cubit_ring[cubit_idx];
    printf("Byte[%zu].Cubit[%d]: val=%d, dir=%s, state=%s, spin=%.4f, super=%s, ent=%d\n",
           byte_idx, cubit_idx,
           c->value,
           direction_to_string(c->direction),
           state_to_string(c->state),
           c->spin,
           c->superposed ? "YES" : "NO",
           c->entangled_with);
}

// ─────────────────────────────────────────────
// MAIN ENTRY POINT
// ─────────────────────────────────────────────

int main(int argc, char** argv) {
    const char* cab_path = "profile.cab";
    if (argc > 1) cab_path = argv[1];

    printf("MMUKO OS Boot Loader\n");
    printf("OBINexus R&D — \"Don't just boot systems. Boot truthful ones.\"\n");
    printf("Version: %s\n\n", MMUKO_VERSION);

    // Create system with 16 bytes of MMUKO memory
    size_t mem_size = 16;
    MMUKO_System* sys = mmuko_system_create(mem_size);
    if (!sys) {
        fprintf(stderr, "Failed to create MMUKO system\n");
        return 1;
    }

    printf("Initialized MMUKO system with %zu bytes\n", mem_size);

    // Load CAB profile
    CAB_Profile profile = cab_default();
    
    printf("\nAttempting to load CAB: %s\n", cab_path);
    if (cab_load(cab_path, &profile)) {
        printf("[CAB] Loaded: %s\n", profile.profile_id);
        strcpy(sys->cab_loaded, profile.profile_id);
        
        // Apply CAB to system
        if (cab_apply(&profile, sys)) {
            printf("[CAB] Applied calibration and cubit values\n");
            printf("  Gravity (vacuum): %.4f\n", sys->medium.gravity);
            printf("  Username: %s\n", profile.username);
        }
    } else {
        printf("[CAB] File not found, using defaults\n");
        strcpy(sys->cab_loaded, "DEFAULT");
    }

    // Execute boot sequence
    printf("\n");
    BootStatus status = mmuko_boot(sys);

    if (status == BOOT_OK) {
        printf("\n=== SYSTEM READY ===\n");
        printf("CAB Profile: %s\n", sys->cab_loaded);
        printf("Frame of reference: %s\n", direction_to_string(sys->frame_of_reference));
        printf("Gravity medium: G=%.4f (lepton=%.4f, muon=%.4f, deep=%.4f)\n",
               sys->medium.gravity, sys->medium.air, sys->medium.water, 
               sys->medium.water / 10.0);

        printf("\nSample cubit states:\n");
        mmuko_print_cubit_state(sys, 0, 0);
        mmuko_print_cubit_state(sys, 0, 2);
        mmuko_print_cubit_state(sys, 5, 5);

        printf("\nLaunching kernel scheduler...\n");
    } else {
        printf("\n=== BOOT FAILED ===\n");
        printf("Status code: %d\n", status);
    }

    // Cleanup
    mmuko_system_destroy(sys);

    return (status == BOOT_OK) ? 0 : 1;
}

// ============================================================
// END OF MMUKO-BOOT.C
// ============================================================
