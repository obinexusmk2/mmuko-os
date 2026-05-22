#ifndef BOOT_TYPES_H
#define BOOT_TYPES_H

// ============================================================
// BOOT_TYPES.H — Shared types for all mmuko-boot phase files
// Included by every phase .c/.h pair.
// ============================================================

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define MMUKO_VERSION "0.2-portable"

// ─── Physical constants ───────────────────────────────────────
#define PI       3.14159265358979
#define G_VACUUM 9.8
#define G_LEPTON (G_VACUUM / 10.0)
#define G_MUON   (G_LEPTON / 10.0)
#define G_DEEP   (G_MUON   / 10.0)

// ─── Direction compass ───────────────────────────────────────
typedef enum {
    N, NE, E, SE, S, SW, W, NW, UNDEFINED_DIR
} Direction;

// ─── Quantum-like cubit state ─────────────────────────────────
typedef enum {
    UP, DOWN, CHARM, STRANGE, LEFT, RIGHT
} State;

// ─── Bit-shift operator ──────────────────────────────────────
typedef enum {
    RSHIFT, LSHIFT, ROTATE
} ShiftOp;

// ─── Boot outcome codes ──────────────────────────────────────
typedef enum {
    BOOT_OK,
    BOOT_LOCK_DETECTED,
    BOOT_ROTATION_LOCK,
    BOOT_UNDEFINED_DIRECTION,
    BOOT_FAILED
} BootStatus;

// ─── Cubit: one bit in the 8-cubit ring of a MMUKO byte ──────
typedef struct {
    int       index;
    uint8_t   value;
    double    spin;
    Direction direction;
    State     state;
    bool      superposed;
    int       entangled_with;
} Cubit;

// ─── MMUKO_Byte: 8-cubit ring + superposition metadata ───────
typedef struct {
    uint8_t   raw_value;
    Cubit     cubit_ring[8];
    int       base_index;
    Direction primary_superposition;
    Direction secondary_superposition;
} MMUKO_Byte;

// ─── Vacuum medium ───────────────────────────────────────────
typedef struct {
    double gravity;
    double air;
    double water;
} VacuumMedium;

// ─── Full contract type (needed by phases to write membrane_phase/outcome)
#include "../../include/boot_contract.h"

// ─── MMUKO_System (minimal view needed by phases) ────────────
typedef struct {
    MMUKO_Byte            *memory_map;
    size_t                 memory_size;
    VacuumMedium           medium;
    double                 gravity;
    Direction              frame_of_reference;
    bool                   boot_complete;
    mmuko_boot_contract_t *contract;
    uint8_t                current_phase;
} MMUKO_System;

#endif // BOOT_TYPES_H
