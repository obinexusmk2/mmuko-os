#ifndef BOOT_HELPERS_H
#define BOOT_HELPERS_H

// ============================================================
// BOOT_HELPERS.H — Shared utility functions used by boot phases
// ============================================================

#include "boot_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// Lookup superposition direction pair for a given numeric base
void lookup_superposition(int base, Direction* primary, Direction* secondary);

// Round an arbitrary base value to the nearest valid even base
int  round_to_even_base(int base);

// Return the midpoint base value (used in phase4 frame centering)
int  get_middle_base(void);

// Rotate byte bits left by n positions (circular)
uint8_t rotate_bits(uint8_t value, int n);

// Apply a semantic bit-shift or rotation
uint8_t bit_shift_semantic(uint8_t value, ShiftOp op, int n);

// Resolve a cubit's quantum state from bit value and neighbor
State resolve_state(int index, uint8_t byte_val);

// Flip a cubit's state to its complement (entanglement anti-correlation)
State flip_state(State state);

// Initialise all 8 cubits in a MMUKO_Byte
void  init_cubit_ring(MMUKO_Byte* byte);

// Return a pointer to a specific cubit by index (bounds-checked)
Cubit* get_cubit_from_byte(MMUKO_Byte* byte, int index);

// Resolve direction for a cubit by majority-vote of its neighbours
Direction resolve_direction_from_neighbors(MMUKO_Byte* byte, int cubit_index);

// Set the system's frame of reference
void set_frame_of_reference(MMUKO_System* sys, Direction center_dir);

// Direction / state name strings
const char* direction_to_string(Direction dir);
const char* state_to_string(State state);

#ifdef __cplusplus
}
#endif

#endif // BOOT_HELPERS_H
