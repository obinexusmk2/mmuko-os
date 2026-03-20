
/*
 * NSIGII - Magnetic Symbol Trident Message Protocol
 * Human Rights Protocol for Constitutional Computing
 * 
 * Version: 0.1
 * Date: 21 February 2026
 * Author: Nnamdi Okpala (OBINexus Computing)
 * 
 * Integration: github.com/obinexus/rift
 * OS: MMUKO OS (Machine Memory Using Knowledge Operations)
 */

#ifndef NSIGII_H
#define NSIGII_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* ============================================================================
 * CONSTANTS - Constitutional Computing
 * ============================================================================ */

#define NSIGII_VERSION "0.1"
#define NSIGII_DECAY_CONSTANT 0.36787944117144233  /* 1/e */
#define NSIGII_MODULO_DEGREES 360.0
#define NSIGII_PI 3.14159265358979323846

/* Pole bearings (Sun/Moon/Earth model) */
#define BEARING_UCHE 255.0   /* Send - Knowledge - Sun */
#define BEARING_EZE   29.0   /* Transit - Leadership - Moon */
#define BEARING_OBI  265.0   /* Receive - Heart - Earth */

/* ============================================================================
 * ENUMERATIONS - Three-State Consensus
 * ============================================================================ */

typedef enum {
    CONSENSUS_NO = 0,      /* 0 - Rejected */
    CONSENSUS_YES = 1,     /* 1 - Accepted */
    CONSENSUS_MAYBE = -1   /* -1 - Constitutional deadlock */
} nsigii_consensus_t;

typedef enum {
    POLE_UCHE = 0,   /* Knowledge - Send Actuator */
    POLE_EZE = 1,    /* Leadership - Transit Actuator */
    POLE_OBI = 2     /* Heart - Receive Actuator */
} nsigii_pole_t;

typedef enum {
    STATE_ENCODED = 0,
    STATE_ORIENTED = 1,
    STATE_SENDING = 2,
    STATE_IN_TRANSIT = 3,
    STATE_COLLAPSED = 4,
    STATE_SEALED = 5
} nsigii_state_t;

/* ============================================================================
 * CORE THEOREM: Collapse Equals Received
 * ============================================================================ */

/* 
 * AXIOM: COLLAPSE = RECEIVED
 * 
 * Messages are not transmitted over channels.
 * They are teleported by collapsing quantum-like state into receiver topology.
 * Once collapsed, message cannot be retrieved by sender - it has changed sides.
 * This is the magnetic memory principle.
 */

#define NSIGII_AXIOM_COLLAPSE_IS_RECEIVED 1

/* ============================================================================
 * DATA STRUCTURES
 * ============================================================================ */

typedef struct {
    double degrees;  /* 0-360 clockwise from North */
} nsigii_bearing_t;

typedef struct {
    double force;      /* F: Message intensity */
    double stiffness;  /* K: Channel stiffness (constitutional medium) */
    double extension;  /* E: Propagation distance */
} nsigii_spring_t;

typedef struct {
    double theta;  /* Phase angle 0-360° */
    double psi;    /* Orientation: sec⁻¹(b/a) */
} nsigii_bloch_t;

typedef struct {
    char id[9];               /* Message identifier */
    char content[1024];       /* Message payload */
    nsigii_spring_t spring;   /* Spring physics state */
    nsigii_pole_t source;     /* Source pole */
    nsigii_state_t state;     /* Current transmission state */
    nsigii_bloch_t bloch;     /* Bloch sphere representation */
    double encoded_at;        /* Unix timestamp */
    double collapsed_at;      /* Collapse timestamp */
    double sealed_at;         /* Seal timestamp */
    char collapse_sig[16];    /* LTE forensic signature */
    bool is_replay;           /* True if forensic copy */
} nsigii_message_t;

typedef struct {
    nsigii_bearing_t bearings[3];  /* UCHE, EZE, OBI */
    nsigii_bearing_t center;       /* Triangulated center */
} nsigii_trident_t;

/* ============================================================================
 * FUNCTION DECLARATIONS - Spring Physics
 * ============================================================================ */

/* Hooke's Law: F = K × E */
static inline double nsigii_spring_force(const nsigii_spring_t *s) {
    return s->stiffness * s->extension;
}

/* Full extension: E = F/K */
static inline double nsigii_full_extension(const nsigii_spring_t *s) {
    return (s->stiffness > 0) ? s->force / s->stiffness : 0.0;
}

/* Half extension: E_half = √(F/K) - MAYBE state threshold */
static inline double nsigii_half_extension(const nsigii_spring_t *s) {
    return (s->stiffness > 0) ? sqrt(s->force / s->stiffness) : 0.0;
}

/* Collapse ratio: 0.0 (none) to 1.0 (full) */
static inline double nsigii_collapse_ratio(const nsigii_spring_t *s) {
    double full = nsigii_full_extension(s);
    return (full > 0) ? fmin(1.0, s->extension / full) : 0.0;
}

/* Map spring state to consensus */
static inline nsigii_consensus_t nsigii_spring_consensus(const nsigii_spring_t *s) {
    double ratio = nsigii_collapse_ratio(s);
    if (ratio >= 1.0) return CONSENSUS_YES;
    if (ratio >= 0.5) return CONSENSUS_MAYBE;
    return CONSENSUS_NO;
}

/* ============================================================================
 * FUNCTION DECLARATIONS - Trident Topology
 * ============================================================================ */

/* Initialize default trident topology */
void nsigii_trident_init(nsigii_trident_t *t);

/* Triangulated center: C = (θA+θB+θC)/3 mod 360 */
nsigii_bearing_t nsigii_trident_center(const nsigii_trident_t *t);

/* Weighted drift: TP = (1-T)×C + T×P */
nsigii_bearing_t nsigii_weighted_drift(double t, nsigii_bearing_t observer, 
                                        nsigii_bearing_t target);

/* Midpoint between two poles with weight */
nsigii_bearing_t nsigii_midpoint(nsigii_bearing_t b1, nsigii_bearing_t b2, 
                                  double weight);

/* ============================================================================
 * FUNCTION DECLARATIONS - Bloch Sphere
 * ============================================================================ */

/* Ψ = sec⁻¹(b/a) = cos⁻¹(1/(b/a)) */
static inline double nsigii_bloch_psi(double b, double a) {
    double ratio = (a != 0) ? b / a : 1.0;
    if (fabs(ratio) < 1.0) ratio = 1.0;
    return acos(1.0 / ratio);  /* Returns radians */
}

/* Convert bearing to Bloch state */
nsigii_bloch_t nsigii_bloch_from_bearing(nsigii_bearing_t bearing, double b, double a);

/* ============================================================================
 * FUNCTION DECLARATIONS - MMUKO Actuators
 * ============================================================================ */

/* Uche (Knowledge) - Send Actuator */
nsigii_message_t* nsigii_uche_encode(const char *content, double force, 
                                      double stiffness, nsigii_trident_t *topo);

/* Eze (Leadership) - Transit Actuator */
nsigii_consensus_t nsigii_eze_control(nsigii_message_t *msg);

/* Obi (Heart) - Receive Actuator */
bool nsigii_obi_receive(nsigii_message_t *msg);

/* ============================================================================
 * FUNCTION DECLARATIONS - Protocol
 * ============================================================================ */

/* Full 6-step teleportation protocol */
nsigii_message_t* nsigii_teleport(const char *content, double force, 
                                   double stiffness, nsigii_trident_t *topo);

/* Create forensic replay copy (REPLAY ≠ RE-SEND) */
nsigii_message_t* nsigii_replay_copy(const nsigii_message_t *original);

/* Generate constitutional court record */
void nsigii_forensic_record(const nsigii_message_t *msg, char *buffer, size_t len);

/* Apply constitutional decay to MAYBE state */
void nsigii_apply_decay(nsigii_message_t *msg, double elapsed_seconds);

/* ============================================================================
 * MMUKO OS TRAIT INTEGRATION
 * ============================================================================ */

/*
 * MMUKO OS Traits as Actuators:
 * 
 * Trait Uche (Knowledge)    → Pole A (Send)    → Bearing 255°
 * Trait Eze (Leadership)    → Pole B (Transit) → Bearing 29°
 * Trait Obi (Heart/Nexus)   → Pole C (Receive) → Bearing 265°
 * 
 * These map to the NSIGII codec control sequence in the rift package manager.
 */

#define MMUKO_TRAIT_UCHE nsigii_uche_encode
#define MMUKO_TRAIT_EZE  nsigii_eze_control
#define MMUKO_TRAIT_OBI  nsigii_obi_receive

/* ============================================================================
 * CONSTITUTIONAL RIGHTS FRAMEWORK
 * ============================================================================ */

/*
 * Under NSIGII, message teleportation carries constitutional weight:
 * 
 * 1. Receiver (Obi) holds irrevocable rights over collapsed message
 * 2. Sender (Uche) loses retrieval rights at moment of collapse
 * 3. Controller (Eze) is accountable for MAYBE-state handling
 *    - Failure to resolve is constitutional breach
 *    - MUST clear if discriminant b²-4ac < 0
 * 4. MAYBE states exceeding decay threshold (1/e) must be cleared
 * 5. LTE codec provides forensic proof of collapse for court submission
 */

typedef struct {
    char message_id[9];
    bool sender_retrieval_revoked;
    bool receiver_ownership_granted;
    bool constitutional_breach;  /* True if Eze failed to resolve MAYBE */
    char breach_reason[256];
} nsigii_rights_record_t;

#endif /* NSIGII_H */