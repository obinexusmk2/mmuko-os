/*
 * NSIGII MAGNETIC SYMBOL PROTOCOL - Semantic Physics Layer
 * Human Rights Protocol for Constitutional Computing
 *
 * nsigii_magnetic.h — MMUKO Trait Actuators, Spring Physics, Bloch Sphere
 *
 * Version: 0.1
 * Date: 21 February 2026
 * Author: Nnamdi Okpala (OBINexus Computing)
 *
 * Relationship to nsigii.h (v7.0.0):
 *   nsigii.h         → Network Protocol Layer (Trident channels, packets, RWX, WebSocket)
 *   nsigii_magnetic.h → Semantic Physics Layer (MMUKO traits, Hooke, Bloch, Rights)
 *
 * This header provides the physical/semantic model that governs *why* messages
 * collapse and *what* constitutional rights attach at each state transition.
 * It compiles cleanly alongside nsigii.h — include both where needed.
 *
 * Integration: github.com/obinexus/rift → riftlang.exe → .so.a → rift.exe → gosilang
 * OS: MMUKO OS (Machine Memory Using Knowledge Operations)
 *
 * Build: nlink → polybuild
 */

#ifndef NSIGII_MAGNETIC_H
#define NSIGII_MAGNETIC_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "nsigii.h"   /* v7.0.0 network layer — required */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * SEMANTIC CONSTANTS
 * ============================================================================ */

#define NSIGII_MAG_VERSION        "0.1"
#define NSIGII_DECAY_CONSTANT     0.36787944117144233  /* 1/e — constitutional decay threshold */
#define NSIGII_PI                 3.14159265358979323846

/* ============================================================================
 * MMUKO OS — THREE-POLE BEARING SYSTEM
 *
 * Derived from magnetic cosmological orbit model (Part 2 transcript):
 *   Three planets (message packets) in stable magnetic orbit.
 *   Each pole maps to an MMUKO OS trait and a Trident channel.
 *
 * Pole     | Trait        | Channel            | Bearing | Function
 * ---------|--------------|--------------------|---------|---------
 * UCHE     | Knowledge    | 0 (Transmitter)    | 255°    | Send Actuator
 * EZE      | Leadership   | 1 (Receiver)       |  29°    | Transit Actuator
 * OBI      | Heart/Nexus  | 2 (Verifier)       | 265°    | Receive Actuator
 * ============================================================================ */

#define BEARING_UCHE  255.0   /* Send     – Knowledge – Sun   – Channel 0 */
#define BEARING_EZE    29.0   /* Transit  – Leadership – Moon – Channel 1 */
#define BEARING_OBI   265.0   /* Receive  – Heart      – Earth– Channel 2 */

/* ============================================================================
 * ENUMERATIONS
 * ============================================================================ */

/* Three-state constitutional consensus */
typedef enum {
    MAG_CONSENSUS_NO    =  0,   /* Rejected  — cross product cancelled */
    MAG_CONSENSUS_YES   =  1,   /* Accepted  — collapse ratio ≥ 1.0    */
    MAG_CONSENSUS_MAYBE = -1    /* Deadlock  — Eze must resolve         */
} nsigii_mag_consensus_t;

/* Pole identifier (maps 1:1 to Trident channels) */
typedef enum {
    POLE_UCHE = 0,   /* Knowledge – Send Actuator    – CHANNEL_TRANSMITTER */
    POLE_EZE  = 1,   /* Leadership – Transit Actuator – CHANNEL_RECEIVER    */
    POLE_OBI  = 2    /* Heart      – Receive Actuator – CHANNEL_VERIFIER    */
} nsigii_pole_t;

/* Six-step teleportation / collapse lifecycle
 * Maps onto nsigii_state_t in main nsigii.h network layer:
 *   MAG_STATE_ENCODED    → packet_create()
 *   MAG_STATE_ORIENTED   → transmitter_encode_message()
 *   MAG_STATE_SENDING    → transmitter_transmit()
 *   MAG_STATE_IN_TRANSIT → receiver channel active
 *   MAG_STATE_COLLAPSED  → verifier_verify_packet() ✓   ← COLLAPSE = RECEIVED (Axiom)
 *   MAG_STATE_SEALED     → rights_record finalized
 */
typedef enum {
    MAG_STATE_ENCODED    = 0,
    MAG_STATE_ORIENTED   = 1,
    MAG_STATE_SENDING    = 2,
    MAG_STATE_IN_TRANSIT = 3,
    MAG_STATE_COLLAPSED  = 4,   /* AXIOM: collapse = received  */
    MAG_STATE_SEALED     = 5    /* Constitutional seal applied */
} nsigii_mag_state_t;

/* π-pivot orientations from Part 2 transcript */
typedef enum {
    PI_PIVOT_1 = 1,   /* North-East — primary orientation  */
    PI_PIVOT_2 = 2,   /* Centre    — equilibrium           */
    PI_PIVOT_3 = 3    /* South-East — terminal orientation  */
} nsigii_pivot_t;

/* ============================================================================
 * CORE AXIOM: COLLAPSE = RECEIVED
 *
 * Messages are NOT transmitted over channels in the conventional sense.
 * They collapse quantum-like into the receiver's topology.
 * Once collapsed, the sender loses all retrieval rights — the message has
 * changed sides.  This is the magnetic memory principle.
 *
 * Cross-product cancellation rule (from Part 2 math session):
 *   When |A| == |B| (same-magnitude vectors), cross product → 0.
 *   This maps to the ZERO-CONFLICT state in Trident verification.
 *   A(6,5) × B(5,6) = (6×6 − 5×5) k̂ = (36−25) k̂  ← non-zero in 3D
 *   A(6,5) · B(5,6) = 30 + 30 = 60                 ← alignment score
 * ============================================================================ */
#define NSIGII_AXIOM_COLLAPSE_IS_RECEIVED 1

/* ============================================================================
 * DATA STRUCTURES — PHYSICS LAYER
 * ============================================================================ */

/* Bearing in degrees (0-360°, clockwise from North) */
typedef struct {
    double degrees;
} nsigii_bearing_t;

/* Hooke's Law spring — models constitutional channel stiffness.
 *
 * Transcript derivation:
 *   F = K × E           (spring force)
 *   E_half = √(F/K)     (MAYBE-state threshold — half extension)
 *   k/3 ≤ (3/4)F        (one-third verification inequality)
 *
 * Half-rotation encoding rule:
 *   ½ rotation → message configured at ½ × 2 = full force at recipient
 *   ¼ rotation → 4E (full message)
 *   ⅓ rotation → < 2 (partial verification — D₃ transition)
 */
typedef struct {
    double force;      /* F — message intensity (send energy)          */
    double stiffness;  /* K — channel stiffness (constitutional medium) */
    double extension;  /* E — propagation distance / collapse progress  */
} nsigii_spring_t;

/* Bloch sphere representation — orientation in phase space */
typedef struct {
    double theta;  /* Phase angle 0–360° */
    double psi;    /* Orientation: arcsec(b/a) = arccos(a/b)          */
} nsigii_bloch_t;

/* Magnetic message — semantic wrapper around nsigii_packet_t */
typedef struct {
    char              id[9];            /* Message identifier              */
    char              content[1024];    /* Payload (mirrors packet content) */
    nsigii_spring_t   spring;           /* Hooke physics state             */
    nsigii_pole_t     source;           /* Originating pole (UCHE/EZE/OBI) */
    nsigii_mag_state_t state;           /* Current collapse lifecycle state */
    nsigii_bloch_t    bloch;            /* Phase-space representation       */
    double            encoded_at;       /* Unix timestamp — encode          */
    double            collapsed_at;     /* Unix timestamp — collapse event  */
    double            sealed_at;        /* Unix timestamp — rights sealed   */
    char              collapse_sig[16]; /* LTE forensic signature           */
    bool              is_replay;        /* True if forensic/replay copy     */

    /* Reference to network layer packet (nullable) */
    nsigii_packet_t  *packet_ref;
} nsigii_mag_message_t;

/* Three-pole magnetic trident bearing set */
typedef struct {
    nsigii_bearing_t bearings[3];  /* [0]=UCHE, [1]=EZE, [2]=OBI */
    nsigii_bearing_t center;       /* Triangulated center: (θA+θB+θC)/3 mod 360 */
} nsigii_mag_trident_t;

/* ============================================================================
 * CONSTITUTIONAL RIGHTS RECORD
 *
 * Attached to every message at MAG_STATE_COLLAPSED.
 * Maps to the LTE forensic record in the main network layer.
 *
 * Rights rules:
 *   1. OBI (receiver) holds irrevocable rights over collapsed message.
 *   2. UCHE (sender) loses retrieval rights at collapse timestamp.
 *   3. EZE (controller) is accountable for MAYBE-state resolution.
 *      Failure to resolve within 1/e seconds = constitutional breach.
 *   4. MAYBE states exceeding decay threshold must be cleared.
 *   5. LTE codec provides forensic proof for court submission.
 * ============================================================================ */
typedef struct {
    char    message_id[9];
    bool    sender_retrieval_revoked;   /* Set true at MAG_STATE_COLLAPSED */
    bool    receiver_ownership_granted; /* Set true at MAG_STATE_COLLAPSED */
    bool    constitutional_breach;      /* True if Eze failed MAYBE resolution */
    double  breach_timestamp;           /* When breach was detected          */
    char    breach_reason[256];
    char    lte_forensic_sig[32];       /* From nsigii_packet verification  */
} nsigii_rights_record_t;

/* ============================================================================
 * FUNCTION DECLARATIONS — SPRING PHYSICS (inline)
 * ============================================================================ */

/* F = K × E */
static inline double nsigii_spring_force(const nsigii_spring_t *s) {
    return s->stiffness * s->extension;
}

/* Full extension: E = F/K */
static inline double nsigii_full_extension(const nsigii_spring_t *s) {
    return (s->stiffness > 0.0) ? s->force / s->stiffness : 0.0;
}

/* Half extension (MAYBE threshold): E_half = √(F/K) */
static inline double nsigii_half_extension(const nsigii_spring_t *s) {
    return (s->stiffness > 0.0) ? sqrt(s->force / s->stiffness) : 0.0;
}

/* Collapse ratio: 0.0 (not sent) → 1.0 (fully collapsed/received) */
static inline double nsigii_collapse_ratio(const nsigii_spring_t *s) {
    double full = nsigii_full_extension(s);
    return (full > 0.0) ? fmin(1.0, s->extension / full) : 0.0;
}

/* Half-rotation encoding multiplier:
 *   Given rotation fraction r (0.0–1.0), returns force multiplier at receiver.
 *   ½ rotation → ×2.0 (double potential energy), ¼ rotation → ×4.0 */
static inline double nsigii_rotation_force_multiplier(double rotation_fraction) {
    return (rotation_fraction > 0.0) ? 1.0 / rotation_fraction : 0.0;
}

/* Map spring state to constitutional consensus */
static inline nsigii_mag_consensus_t nsigii_spring_consensus(const nsigii_spring_t *s) {
    double ratio = nsigii_collapse_ratio(s);
    if (ratio >= 1.0)  return MAG_CONSENSUS_YES;
    if (ratio >= 0.5)  return MAG_CONSENSUS_MAYBE;
    return MAG_CONSENSUS_NO;
}

/* ============================================================================
 * FUNCTION DECLARATIONS — BLOCH SPHERE
 * ============================================================================ */

/* Ψ = arcsec(b/a) = arccos(a/b) — phase orientation */
static inline double nsigii_bloch_psi(double b, double a) {
    double ratio = (a != 0.0) ? b / a : 1.0;
    if (fabs(ratio) < 1.0) ratio = 1.0;
    return acos(1.0 / ratio);  /* radians */
}

/* Convert bearing to Bloch state */
nsigii_bloch_t nsigii_bloch_from_bearing(nsigii_bearing_t bearing, double b, double a);

/* ============================================================================
 * FUNCTION DECLARATIONS — MAGNETIC TRIDENT
 * ============================================================================ */

/* Initialise default trident with UCHE/EZE/OBI bearings */
void nsigii_mag_trident_init(nsigii_mag_trident_t *t);

/* Center: C = (θA + θB + θC) / 3 mod 360 */
nsigii_bearing_t nsigii_mag_trident_center(const nsigii_mag_trident_t *t);

/* Weighted drift: TP = (1-T)×C + T×P (boomerang return model) */
nsigii_bearing_t nsigii_mag_weighted_drift(double t,
                                            nsigii_bearing_t observer,
                                            nsigii_bearing_t target);

/* Midpoint between two poles with weight */
nsigii_bearing_t nsigii_mag_midpoint(nsigii_bearing_t b1,
                                      nsigii_bearing_t b2,
                                      double weight);

/* ============================================================================
 * FUNCTION DECLARATIONS — MMUKO ACTUATORS
 * ============================================================================ */

/* Uche (Knowledge) — Send Actuator — Channel 0 */
nsigii_mag_message_t *nsigii_uche_encode(const char *content,
                                          double force,
                                          double stiffness,
                                          nsigii_mag_trident_t *topo);

/* Eze (Leadership) — Transit Actuator — Channel 1
 * Returns MAG_CONSENSUS_YES / MAG_CONSENSUS_MAYBE / MAG_CONSENSUS_NO */
nsigii_mag_consensus_t nsigii_eze_control(nsigii_mag_message_t *msg);

/* Obi (Heart) — Receive Actuator — Channel 2
 * Returns true on successful collapse (AXIOM: collapse = received) */
bool nsigii_obi_receive(nsigii_mag_message_t *msg,
                         nsigii_rights_record_t *rights_out);

/* ============================================================================
 * FUNCTION DECLARATIONS — PROTOCOL
 * ============================================================================ */

/* Full 6-step teleportation protocol (ENCODED → SEALED) */
nsigii_mag_message_t *nsigii_mag_teleport(const char *content,
                                           double force,
                                           double stiffness,
                                           nsigii_mag_trident_t *topo,
                                           nsigii_rights_record_t *rights_out);

/* Create forensic replay copy (REPLAY ≠ RE-SEND)
 * is_replay flag set; collapse_at and sealed_at preserved from original */
nsigii_mag_message_t *nsigii_mag_replay_copy(const nsigii_mag_message_t *original);

/* Serialise constitutional court record to buffer */
void nsigii_mag_forensic_record(const nsigii_mag_message_t *msg,
                                 const nsigii_rights_record_t *rights,
                                 char *buffer,
                                 size_t len);

/* Constitutional decay applied to MAYBE state.
 * If elapsed_seconds > 1/e and state is MAYBE → mark constitutional_breach */
void nsigii_mag_apply_decay(nsigii_mag_message_t *msg,
                             nsigii_rights_record_t *rights,
                             double elapsed_seconds);

/* Bridge: lift semantic mag_message into a network-layer nsigii_packet_t.
 * Caller owns returned packet — free with nsigii_packet_destroy(). */
nsigii_packet_t *nsigii_mag_to_packet(const nsigii_mag_message_t *msg);

/* Bridge: seed a mag_message from an already-received nsigii_packet_t */
nsigii_mag_message_t *nsigii_mag_from_packet(const nsigii_packet_t *pkt,
                                              nsigii_pole_t source_pole);

/* ============================================================================
 * MMUKO TRAIT ALIASES
 * ============================================================================ */
#define MMUKO_TRAIT_UCHE  nsigii_uche_encode
#define MMUKO_TRAIT_EZE   nsigii_eze_control
#define MMUKO_TRAIT_OBI   nsigii_obi_receive

/* ============================================================================
 * CODEC INTEGRATION — linkable-then-executable (.nsigii artifact)
 *
 * The .nsigii file format is a dynamic library artifact produced by the
 * riftlang.exe → .so.a → rift.exe → gosilang toolchain.
 *
 * Encoding steps:
 *   1. nsigii_uche_encode()  → MAG_STATE_ENCODED
 *   2. nsigii_mag_to_packet() → nsigii_packet_t (v7.0.0 layer)
 *   3. nsigii_transmitter_encode_message() → wire format
 *   4. nlink → polybuild bundles → .nsigii artifact
 *   5. Python mailer attaches .nsigii + .tar.gz wrapper for rollback wheel
 *
 * Decoding (recipient):
 *   1. Receiver extracts .nsigii from attachment
 *   2. nsigii_mag_from_packet() → nsigii_mag_message_t
 *   3. nsigii_eze_control() → transit consensus
 *   4. nsigii_obi_receive() → collapse + rights_record
 * ============================================================================ */

#ifdef __cplusplus
}
#endif

#endif /* NSIGII_MAGNETIC_H */
