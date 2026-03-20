/*
 * NSIGII MAYBE RESOLUTION — Implementation
 * nsigii_maybe_resolver.c
 *
 * Resolves MAYBE deadlock (0.5 ≤ ratio < 1.0) using:
 *   1. Fractional rotation escalation: ½ → ¼ → ⅙ → ⅛
 *   2. GPS lattice tracking of human drift
 *   3. OBINexus derivative for bearing rate correction
 *   4. Force compensation based on distance & velocity
 */

#include "nsigii_maybe_resolver.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ============================================================================
 * GPS/MATH UTILITIES
 * ============================================================================ */

/* Earth radius in meters (WGS84) */
#define EARTH_RADIUS_M 6371000.0

/* Convert degrees to radians */
static double deg_to_rad(double deg) {
    return deg * M_PI / 180.0;
}

/* Convert radians to degrees */
static double rad_to_deg(double rad) {
    return rad * 180.0 / M_PI;
}

/* Haversine distance between two GPS points (meters) */
static double gps_distance_m(nsigii_gps_point_t p1, nsigii_gps_point_t p2) {
    double lat1_rad = deg_to_rad(p1.latitude);
    double lat2_rad = deg_to_rad(p2.latitude);
    double dlat_rad = deg_to_rad(p2.latitude - p1.latitude);
    double dlon_rad = deg_to_rad(p2.longitude - p1.longitude);

    double a = sin(dlat_rad / 2.0) * sin(dlat_rad / 2.0) +
               cos(lat1_rad) * cos(lat2_rad) *
               sin(dlon_rad / 2.0) * sin(dlon_rad / 2.0);

    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return EARTH_RADIUS_M * c;
}

/* Bearing from p1 to p2 (degrees, 0=North, 90=East, 180=South, 270=West) */
static double gps_bearing(nsigii_gps_point_t p1, nsigii_gps_point_t p2) {
    double lat1_rad = deg_to_rad(p1.latitude);
    double lat2_rad = deg_to_rad(p2.latitude);
    double dlon_rad = deg_to_rad(p2.longitude - p1.longitude);

    double y = sin(dlon_rad) * cos(lat2_rad);
    double x = cos(lat1_rad) * sin(lat2_rad) -
               sin(lat1_rad) * cos(lat2_rad) * cos(dlon_rad);

    double bearing_rad = atan2(y, x);
    double bearing_deg = rad_to_deg(bearing_rad);

    /* Normalize to [0, 360) */
    while (bearing_deg < 0.0) bearing_deg += 360.0;
    while (bearing_deg >= 360.0) bearing_deg -= 360.0;

    return bearing_deg;
}

/* Convert lat/lon to lattice grid coordinates (1m × 1m cells)
 * Approximate: 1 degree ≈ 111,111 meters at equator */
static int gps_to_grid_coord(double lat_or_lon, bool is_latitude) {
    /* Simplified: 1 grid unit = ~1 meter */
    double meters = lat_or_lon * 111111.0;
    return (int)floor(meters);
}

/* ============================================================================
 * MAYBE RESOLVER LIFECYCLE
 * ============================================================================ */

nsigii_maybe_resolver_t *nsigii_maybe_resolver_create(
    nsigii_mag_message_t *msg,
    nsigii_gps_point_t human_pos,
    double current_time) {

    if (!msg) return NULL;

    nsigii_maybe_resolver_t *resolver = malloc(sizeof(nsigii_maybe_resolver_t));
    if (!resolver) return NULL;

    resolver->msg = msg;
    resolver->maybe_entered_at = current_time;
    resolver->escalation_level = 0;  /* Start at ½ rotation */

    /* Rotation fractions: ½, ¼, ⅙, ⅛ */
    resolver->rotation_fractions[0] = ROTATION_HALF;
    resolver->rotation_fractions[1] = ROTATION_QUARTER;
    resolver->rotation_fractions[2] = ROTATION_SIXTH;
    resolver->rotation_fractions[3] = ROTATION_EIGHTH;

    /* Force multipliers: 2.0, 4.0, 6.0, 8.0 */
    for (int i = 0; i < 4; i++) {
        resolver->force_multipliers[i] = 
            nsigii_rotation_force_multiplier(resolver->rotation_fractions[i]);
    }

    /* Initialize GPS lattice */
    resolver->human_position.current = human_pos;
    resolver->human_position.previous = human_pos;  /* No prior position yet */
    resolver->human_position.grid_x = 
        gps_to_grid_coord(human_pos.latitude, true);
    resolver->human_position.grid_y = 
        gps_to_grid_coord(human_pos.longitude, false);
    resolver->human_position.drift_velocity = 0.0;
    resolver->human_position.drift_bearing = 0.0;

    /* Initialize bearing derivative */
    resolver->bearing_derivative.dbearing_dt = 0.0;
    resolver->bearing_derivative.last_bearing = 0.0;
    resolver->bearing_derivative.current_bearing = 0.0;
    resolver->bearing_derivative.last_update = current_time;

    resolver->adjusted_force = msg->spring.force;

    printf("[MAYBE] Resolver created for %s | escalation_level=%d\n",
           msg->id, resolver->escalation_level);

    return resolver;
}

void nsigii_maybe_resolver_destroy(nsigii_maybe_resolver_t *resolver) {
    if (resolver) free(resolver);
}

/* ============================================================================
 * GPS LATTICE TRACKING
 * ============================================================================ */

nsigii_gps_lattice_cell_t nsigii_gps_to_lattice(
    nsigii_gps_point_t current,
    nsigii_gps_point_t previous) {

    nsigii_gps_lattice_cell_t cell;
    cell.current = current;
    cell.previous = previous;
    cell.grid_x = gps_to_grid_coord(current.latitude, true);
    cell.grid_y = gps_to_grid_coord(current.longitude, false);

    nsigii_update_drift(&cell);
    return cell;
}

void nsigii_update_drift(nsigii_gps_lattice_cell_t *cell) {
    if (!cell) return;

    double distance_m = gps_distance_m(cell->previous, cell->current);
    double time_delta = cell->current.timestamp - cell->previous.timestamp;

    if (time_delta > 0.001) {  /* Avoid division by near-zero */
        cell->drift_velocity = distance_m / time_delta;
    } else {
        cell->drift_velocity = 0.0;
    }

    if (distance_m > 0.1) {  /* Only update bearing if moved >10cm */
        cell->drift_bearing = gps_bearing(cell->previous, cell->current);
    }

    printf("[DRIFT] Distance: %.2f m | Velocity: %.2f m/s | Bearing: %.1f°\n",
           distance_m, cell->drift_velocity, cell->drift_bearing);
}

/* ============================================================================
 * OBINEXUS DERIVATIVE — Bearing Rate of Change
 * ============================================================================ */

void nsigii_update_bearing_derivative(
    nsigii_obinexus_derivative_t *deriv,
    double current_bearing,
    double current_time) {

    if (!deriv) return;

    deriv->last_bearing = deriv->current_bearing;
    deriv->current_bearing = current_bearing;

    double time_delta = current_time - deriv->last_update;
    if (time_delta > 0.001) {
        double dbearing = deriv->current_bearing - deriv->last_bearing;
        /* Handle wraparound (e.g., 359° → 1° = +2°, not -358°) */
        while (dbearing > 180.0) dbearing -= 360.0;
        while (dbearing < -180.0) dbearing += 360.0;

        deriv->dbearing_dt = dbearing / time_delta;  /* degrees/second */
    } else {
        deriv->dbearing_dt = 0.0;
    }

    deriv->last_update = current_time;

    printf("[DERIV] dθ/dt = %.2f °/s | bearing: %.1f° → %.1f°\n",
           deriv->dbearing_dt, deriv->last_bearing, deriv->current_bearing);
}

/* ============================================================================
 * ROTATION FRACTION & FORCE MULTIPLIER
 * ============================================================================ */

double nsigii_get_rotation_fraction(int escalation_level) {
    switch (escalation_level) {
        case 0: return ROTATION_HALF;     /* ½ = 0.5 */
        case 1: return ROTATION_QUARTER;  /* ¼ = 0.25 */
        case 2: return ROTATION_SIXTH;    /* ⅙ ≈ 0.167 */
        case 3: return ROTATION_EIGHTH;   /* ⅛ = 0.125 */
        default: return ROTATION_EIGHTH;  /* Cap at ⅛ */
    }
}

double nsigii_rotation_force_multiplier(double rotation_fraction) {
    return (rotation_fraction > 0.0) ? 1.0 / rotation_fraction : 0.0;
}

/* ============================================================================
 * MAIN: RESOLVE MAYBE WITH ESCALATION
 * ============================================================================ */

nsigii_mag_consensus_t nsigii_eze_resolve_maybe(
    nsigii_maybe_resolver_t *resolver,
    double elapsed_since_maybe) {

    if (!resolver || !resolver->msg) return MAG_CONSENSUS_NO;

    double rotation_frac = nsigii_get_rotation_fraction(resolver->escalation_level);
    double force_mult = resolver->force_multipliers[resolver->escalation_level];

    printf("\n[EZE-RESOLVE] Message %s | escalation_level=%d (rotation=1/%d)\n",
           resolver->msg->id,
           resolver->escalation_level,
           (int)(1.0 / rotation_frac));

    printf("             Elapsed: %.3f s | F_mult: %.1f× | Threshold: %.3f s\n",
           elapsed_since_maybe, force_mult, NSIGII_DECAY_CONSTANT);

    /* ===================================================================
     * ESCALATION LOGIC: Each step escalates force via rotation fraction
     * ===================================================================
     *
     * Step 0 (½ rotation):   F' = F × 2.0
     * Step 1 (¼ rotation):   F' = F × 4.0
     * Step 2 (⅙ rotation):   F' = F × 6.0
     * Step 3 (⅛ rotation):   F' = F × 8.0 (final)
     */

    /* Apply current force multiplier */
    resolver->adjusted_force = resolver->msg->spring.force * force_mult;
    resolver->msg->spring.force = resolver->adjusted_force;

    printf("[FORCE] F_original=%.2f | F_adjusted=%.2f (×%.1f)\n",
           resolver->msg->spring.force / force_mult,
           resolver->msg->spring.force,
           force_mult);

    /* Recompute consensus with new force */
    double new_extension = nsigii_full_extension(&resolver->msg->spring);
    resolver->msg->spring.extension = new_extension;
    double new_ratio = nsigii_collapse_ratio(&resolver->msg->spring);

    printf("[RATIO] new_extension=%.4f | collapse_ratio=%.4f\n",
           new_extension, new_ratio);

    nsigii_mag_consensus_t consensus = nsigii_spring_consensus(&resolver->msg->spring);

    /* ===================================================================
     * DECISION TREE
     * ===================================================================
     */

    if (consensus == MAG_CONSENSUS_YES) {
        printf("[EZE] ✓ ESCALATION SUCCESS → Consensus=YES (ratio=%.4f)\n", new_ratio);
        return MAG_CONSENSUS_YES;
    }

    if (elapsed_since_maybe >= NSIGII_DECAY_CONSTANT) {
        /* Decay threshold exceeded */
        if (resolver->escalation_level < 3) {
            /* Escalate to next level */
            resolver->escalation_level++;
            printf("[EZE] ⏱ Decay threshold exceeded (%.3f s)\n", 
                   elapsed_since_maybe);
            printf("      → Escalating to level %d (rotation=1/%d)\n",
                   resolver->escalation_level,
                   (int)(1.0 / nsigii_get_rotation_fraction(resolver->escalation_level)));

            /* Recursively resolve with next level */
            return nsigii_eze_resolve_maybe(resolver, 0.0);  /* Reset timer */
        } else {
            /* Already at max escalation (⅛) */
            printf("[EZE] ⚠ ESCALATION EXHAUSTED at level 3 (⅛ rotation)\n");
            printf("      → Forcing YES (best-effort delivery)\n");
            return MAG_CONSENSUS_YES;  /* Force success */
        }
    }

    printf("[EZE] ? Still MAYBE (ratio=%.4f) | waiting...\n", new_ratio);
    return MAG_CONSENSUS_MAYBE;
}

/* ============================================================================
 * HUMAN DRIFT COMPENSATION
 * ============================================================================ */

bool nsigii_human_drifted_beyond(
    nsigii_gps_lattice_cell_t *cell,
    double drift_threshold_meters) {

    if (!cell) return false;
    double distance = gps_distance_m(cell->previous, cell->current);
    return distance > drift_threshold_meters;
}

void nsigii_compensate_force_for_drift(
    nsigii_maybe_resolver_t *resolver,
    nsigii_bearing_t drone_bearing,
    nsigii_bearing_t human_drift_bearing) {

    if (!resolver) return;

    /* Angle between drone-to-human and human-drift direction */
    double delta_bearing = human_drift_bearing.degrees - drone_bearing.degrees;

    /* Normalize to [-180, 180] */
    while (delta_bearing > 180.0) delta_bearing -= 360.0;
    while (delta_bearing < -180.0) delta_bearing += 360.0;

    /* If human drifting AWAY (δ near 0°), increase force
       If human drifting TOWARD (δ near 180°), decrease force */

    double divergence = fabs(delta_bearing);  /* 0° = moving away, 180° = approaching */
    double force_adjustment = 1.0 + (divergence / 180.0) * 0.5;  /* ±50% adjustment */

    resolver->adjusted_force *= force_adjustment;
    resolver->msg->spring.force = resolver->adjusted_force;

    printf("[DRIFT-COMPENSATE] Δθ=%.1f° | divergence=%.1f° | F_mult=%.2f\n",
           delta_bearing, divergence, force_adjustment);
}

/* ============================================================================
 * HUMAN POSITION PREDICTION
 * ============================================================================ */

nsigii_gps_point_t nsigii_predict_human_position(
    nsigii_gps_lattice_cell_t *cell,
    double t_future) {

    if (!cell) {
        nsigii_gps_point_t zero = {0, 0, 0, 0};
        return zero;
    }

    nsigii_gps_point_t prediction = cell->current;

    if (cell->drift_velocity > 0.001) {
        /* Linear extrapolation: move in drift direction for t_future seconds */
        double distance_m = cell->drift_velocity * t_future;

        /* Convert drift direction (bearing) to lat/lon delta */
        double bearing_rad = deg_to_rad(cell->drift_bearing);
        double lat_delta = (distance_m / 111111.0) * cos(bearing_rad);
        double lon_delta = (distance_m / 111111.0) * sin(bearing_rad) / 
                          cos(deg_to_rad(cell->current.latitude));

        prediction.latitude += lat_delta;
        prediction.longitude += lon_delta;

        printf("[PREDICT] t_future=%.1f s | distance=%.1f m | new_pos=(%.6f, %.6f)\n",
               t_future, distance_m, prediction.latitude, prediction.longitude);
    }

    return prediction;
}

/* ============================================================================
 * FORENSIC LOGGING
 * ============================================================================ */

void nsigii_maybe_resolver_log(const nsigii_maybe_resolver_t *resolver,
                                char *buffer, size_t len) {
    if (!resolver || !buffer) return;

    snprintf(buffer, len,
        "=== NSIGII MAYBE RESOLUTION LOG ===\n"
        "Message ID              : %s\n"
        "Entered MAYBE at        : %.3f\n"
        "Escalation Level        : %d\n"
        "Rotation Fraction       : 1/%d\n"
        "Force Multiplier        : %.2f×\n"
        "Original Force          : %.2f\n"
        "Adjusted Force          : %.2f\n"
        "Human Position (lat/lon): %.6f, %.6f\n"
        "Human Drift Velocity    : %.2f m/s\n"
        "Human Drift Bearing     : %.1f°\n"
        "Bearing Rate (dθ/dt)    : %.2f °/s\n"
        "Grid Cell (X/Y)         : %d, %d\n"
        "=====================================\n",
        resolver->msg->id,
        resolver->maybe_entered_at,
        resolver->escalation_level,
        (int)(1.0 / resolver->rotation_fractions[resolver->escalation_level]),
        resolver->force_multipliers[resolver->escalation_level],
        resolver->msg->spring.force / 
            resolver->force_multipliers[resolver->escalation_level],
        resolver->msg->spring.force,
        resolver->human_position.current.latitude,
        resolver->human_position.current.longitude,
        resolver->human_position.drift_velocity,
        resolver->human_position.drift_bearing,
        resolver->bearing_derivative.dbearing_dt,
        resolver->human_position.grid_x,
        resolver->human_position.grid_y);
}
