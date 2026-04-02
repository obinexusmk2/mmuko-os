/**
 * include/ring_model.h
 * MMUKO-OS Electromagnetic Protection Ring Model
 * OBINexus Computing | Nnamdi Michael Okpala | 29 March 2026
 *
 * Maps x86 protection rings to the electromagnetic computation duality:
 *
 *   Ring 0 (Kernel):  Boot chain + runtime — magnetic/compile-time state
 *   Ring 1 (Driver):  BIOS firmware interface (SpinPair, Mosaic, RTC)
 *   Ring 2 (Service): Membrane + MPDA + tripartite discriminant layer
 *   Ring 3 (User):    Python/Cython UI + applications — electric/runtime state
 *
 * Electromagnetic duality (stateless double-compile):
 *   Electric = runtime execution (sine wave, digital square waveform)
 *   Magnetic = compile-time linking (cosine wave, analog waveform)
 *   Isomorphic: sin^2 + cos^2 = 1 (conservation — same information,
 *               inscribed into each other, same ground covered)
 *
 * Ring transition gates use the NSIGII 6-phase protocol:
 *   Escalation (user → kernel) requires membrane PASS + NSIGII phase
 *   Demotion (kernel → user) is always permitted
 */

#ifndef MMUKO_RING_MODEL_H
#define MMUKO_RING_MODEL_H

#include "boot_contract.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Electromagnetic computation mode — the "double compile" duality.
 * One runtime for electric (execution), one for magnetic (linking).
 * The two are isomorphic: every electric signal has a magnetic equivalent.
 */
typedef enum {
    MMUKO_COMPUTE_ELECTRIC = 0,  /* Runtime execution (sine/digital) */
    MMUKO_COMPUTE_MAGNETIC = 1   /* Compile-time linking (cosine/analog) */
} mmuko_compute_mode_t;

/**
 * Ring state — combines protection level, compute mode, and NSIGII gate.
 */
typedef struct {
    mmuko_ring_level_t   ring;          /* Current protection ring (0-3) */
    mmuko_compute_mode_t mode;          /* Electric (runtime) or magnetic (link) */
    mmuko_membrane_outcome_t gate;      /* PASS / HOLD / ALERT */
    uint8_t              nsigii_phase;  /* Current NSIGII phase (1-6) */
} mmuko_ring_state_t;

/**
 * mmuko_ring_transition_allowed — check if a ring escalation is permitted.
 *
 * Transition rules (from electromagnetic model):
 *   Demotion (higher ring number = less privilege): always allowed
 *   Ring 0 → Ring 1 (kernel → driver): always allowed
 *   Ring 3 → Ring 2: requires membrane PASS
 *   Ring 2 → Ring 1: requires membrane PASS + NSIGII phase >= 4 (Governance)
 *   Ring 1 → Ring 0: requires membrane PASS + NSIGII phase == 6 (Integrity)
 */
static inline bool mmuko_ring_transition_allowed(
    mmuko_ring_level_t from, mmuko_ring_level_t to,
    mmuko_membrane_outcome_t membrane, uint8_t nsigii_phase)
{
    /* Demotion always allowed */
    if (to > from) return true;
    /* Kernel can always reach drivers */
    if (from == MMUKO_RING_0_KERNEL && to == MMUKO_RING_1_DRIVER) return true;
    /* Same ring — no transition needed */
    if (from == to) return true;
    /* Escalation requires membrane PASS */
    if (membrane != MMUKO_MEMBRANE_PASS) return false;
    /* Ring 3 → 2: membrane PASS is sufficient */
    if (from == MMUKO_RING_3_USER && to == MMUKO_RING_2_SERVICE)
        return true;
    /* Ring 2 → 1: needs NSIGII Governance phase */
    if (from == MMUKO_RING_2_SERVICE && to == MMUKO_RING_1_DRIVER)
        return nsigii_phase >= 4;
    /* Ring 1 → 0: needs full NSIGII Integrity verification */
    if (from == MMUKO_RING_1_DRIVER && to == MMUKO_RING_0_KERNEL)
        return nsigii_phase >= 6;
    return false;
}

/**
 * mmuko_ring_state_init — initialize ring state at boot (Ring 0, magnetic mode).
 */
static inline mmuko_ring_state_t mmuko_ring_state_init(void)
{
    mmuko_ring_state_t state;
    state.ring = MMUKO_RING_0_KERNEL;
    state.mode = MMUKO_COMPUTE_MAGNETIC;
    state.gate = MMUKO_MEMBRANE_HOLD;
    state.nsigii_phase = 0;
    return state;
}

#ifdef __cplusplus
}
#endif

#endif /* MMUKO_RING_MODEL_H */
