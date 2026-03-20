/*
 * NSIGII MAYBE RESOLUTION — Fractional Rotation Encoding with GPS Lattice
 * Drone Delivery + Human Drift Tracking via OBINexus Derivative
 *
 * nsigii_maybe_resolver.h & nsigii_maybe_resolver.c
 * Version: 0.2
 * Author: Nnamdi Okpala
 * Date: 17 March 2026
 *
 * HYPOTHESIS: When EZE enters MAYBE state (0.5 ≤ ratio < 1.0),
 * use fractional rotation multipliers to compute force escalation.
 * As human drifts (GPS moves), drone adjusts via OBINexus derivative tracking.
 *
 * Rotation fractions: ½, ¼, ⅙, ⅛ → lattice order escalation
 * Force multiplier: M = 1 / rotation_fraction
 */

#ifndef NSIGII_MAYBE_RESOLVER_H
#define NSIGII_MAYBE_RESOLVER_H

#include "nsigii_magnetic.h"
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * FRACTIONAL ROTATION ENCODING
 * ============================================================================
 * 
 * Rotation Fraction | Force Multiplier | Physical Meaning
 * ─────────────────────────────────────────────────────────
 * ½ (0.5)          | 2.0×            | Half rotation → double potential
 * ¼ (0.25)         | 4.0×            | Quarter rotation → quad potential
 * ⅙ (0.167)        | 6.0×            | Sixth rotation → escalation level 3
 * ⅛ (0.125)        | 8.0×            | Eighth rotation → full escalation
 *
 * Applied sequentially: ½ → ¼ → ⅙ → ⅛
 * Each step: if MAYBE persists, advance to next fraction
 * ============================================================================ */

#define ROTATION_HALF     0.5      /* ½ rotation */
#define ROTATION_QUARTER  0.25     /* ¼ rotation */
#define ROTATION_SIXTH    0.16666666666666666  /* ⅙ rotation */
#define ROTATION_EIGHTH   0.125    /* ⅛ rotation */

/* ============================================================================
 * GPS LATTICE TRACKING — OBINexus Derivative
 * ============================================================================
 *
 * As human moves, drone must adjust bearing and force.
 * GPS lattice: discrete grid points at ~ 1 meter intervals
 * OBINexus derivative: d(bearing)/d(time) as human drifts
 *
 * Lattice cell: 1.0m × 1.0m square in WGS84 coordinates
 * Drift velocity: |ΔGP S| / Δt (meters per second)
 * ============================================================================ */

typedef struct {
    double latitude;      /* WGS84 latitude (degrees) */
    double longitude;     /* WGS84 longitude (degrees) */
    double altitude;      /* meters above sea level */
    double timestamp;     /* unix time of GPS fix */
} nsigii_gps_point_t;

/* GPS lattice cell — tracks one discrete position */
typedef struct {
    int grid_x;           /* Lattice grid X (integer cell index) */
    int grid_y;           /* Lattice grid Y (integer cell index) */
    nsigii_gps_point_t current;
    nsigii_gps_point_t previous;
    double drift_velocity;  /* meters/second (|Δ GPS| / Δt) */
    double drift_bearing;   /* Direction human is moving (degrees) */
} nsigii_gps_lattice_cell_t;

/* OBINexus derivative — rate of bearing change */
typedef struct {
    double dbearing_dt;     /* dθ/dt — bearing change per second */
    double last_bearing;    /* Previous bearing (degrees) */
    double current_bearing; /* Current bearing (degrees) */
    double last_update;     /* Last timestamp */
} nsigii_obinexus_derivative_t;

/* MAYBE state with rotation context */
typedef struct {
    nsigii_mag_message_t *msg;
    double maybe_entered_at;        /* When state became MAYBE */
    int escalation_level;           /* 0=½, 1=¼, 2=⅙, 3=⅛ */
    double rotation_fractions[4];   /* [0.5, 0.25, 0.167, 0.125] */
    double force_multipliers[4];    /* [2.0, 4.0, 6.0, 8.0] */
    nsigii_gps_lattice_cell_t human_position;
    nsigii_obinexus_derivative_t bearing_derivative;
    double adjusted_force;          /* F' after rotation escalation */
} nsigii_maybe_resolver_t;

/* ============================================================================
 * FUNCTION DECLARATIONS
 * ============================================================================ */

/* Initialize MAYBE resolver with current message & GPS position */
nsigii_maybe_resolver_t *nsigii_maybe_resolver_create(
    nsigii_mag_message_t *msg,
    nsigii_gps_point_t human_pos,
    double current_time);

/* Free resolver */
void nsigii_maybe_resolver_destroy(nsigii_maybe_resolver_t *resolver);

/* Convert GPS (lat/lon) to lattice grid cell (discrete integer position) */
nsigii_gps_lattice_cell_t nsigii_gps_to_lattice(
    nsigii_gps_point_t current,
    nsigii_gps_point_t previous);

/* Compute drift velocity and bearing as human moves */
void nsigii_update_drift(nsigii_gps_lattice_cell_t *cell);

/* Compute OBINexus derivative: dθ/dt (bearing rate of change) */
void nsigii_update_bearing_derivative(
    nsigii_obinexus_derivative_t *deriv,
    double current_bearing,
    double current_time);

/* Get next rotation fraction based on escalation level */
double nsigii_get_rotation_fraction(int escalation_level);

/* Get force multiplier for rotation (M = 1 / fraction) */
double nsigii_rotation_force_multiplier(double rotation_fraction);

/* MAIN: Resolve MAYBE state with force escalation */
nsigii_mag_consensus_t nsigii_eze_resolve_maybe(
    nsigii_maybe_resolver_t *resolver,
    double elapsed_since_maybe);

/* Check if human has drifted beyond threshold (e.g., 10m) */
bool nsigii_human_drifted_beyond(
    nsigii_gps_lattice_cell_t *cell,
    double drift_threshold_meters);

/* Compensate message force for human drift
 * If human moving away, increase force; if approaching, decrease */
void nsigii_compensate_force_for_drift(
    nsigii_maybe_resolver_t *resolver,
    nsigii_bearing_t drone_bearing,
    nsigii_bearing_t human_drift_bearing);

/* Predict human position at time t_future (linear extrapolation) */
nsigii_gps_point_t nsigii_predict_human_position(
    nsigii_gps_lattice_cell_t *cell,
    double t_future);

#ifdef __cplusplus
}
#endif

#endif /* NSIGII_MAYBE_RESOLVER_H */
