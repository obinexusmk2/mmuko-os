/*
 * EXAMPLE: Drone Delivery with MAYBE Resolution
 * Using NSIGII fractional rotation + GPS lattice tracking
 *
 * Scenario: Drone delivering food/water/shelter to human (Bob)
 *           Bob moves away during delivery → MAYBE state triggered
 *           System auto-escalates with ½→¼→⅙→⅛ rotation fractions
 *           GPS tracks human drift, compensates force
 */

#include <stdio.h>
#include <time.h>
#include "nsigii_maybe_resolver.h"

/* Mock current time for testing */
static double mock_time = 1710710400.0;  /* March 17, 2026 14:00:00 UTC */

int main(void) {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║ NSIGII DRONE DELIVERY — MAYBE Resolution with GPS Tracking ║\n");
    printf("║ OBINexus Computing | Fractional Rotation Escalation        ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");

    /* ===================================================================
     * STEP 1: Initialize magnetic trident & UCHE encode
     * ===================================================================
     */

    printf("[STEP 1] UCHE Encode — Bob sends \"I need food, water, shelter\"\n\n");

    nsigii_mag_trident_t trident;
    nsigii_mag_trident_init(&trident);

    nsigii_mag_message_t *msg = nsigii_uche_encode(
        "I need food, water, and shelter now.",
        1.0,     /* Force: human urgency */
        0.67,    /* Stiffness: network capacity */
        &trident
    );

    if (!msg) {
        fprintf(stderr, "UCHE encode failed\n");
        return 1;
    }

    printf("\n");

    /* ===================================================================
     * STEP 2: EZE transit — extend spring, check consensus
     * ===================================================================
     */

    printf("[STEP 2] EZE Control — Transit Actuator checks consensus\n\n");

    nsigii_mag_consensus_t consensus = nsigii_eze_control(msg);

    printf("Consensus: %s\n", 
           consensus == MAG_CONSENSUS_YES ? "YES ✓" :
           consensus == MAG_CONSENSUS_MAYBE ? "MAYBE ?" : "NO ✗");

    printf("\nSpring state after EZE:\n");
    printf("  Force: %.2f\n", msg->spring.force);
    printf("  Stiffness: %.2f\n", msg->spring.stiffness);
    printf("  Extension: %.4f\n", msg->spring.extension);
    printf("  Collapse ratio: %.4f\n", nsigii_collapse_ratio(&msg->spring));

    printf("\n");

    /* ===================================================================
     * STEP 3: Simulate MAYBE state (force reduced during transit)
     * ===================================================================
     */

    printf("[STEP 3] Simulate Network Weakness → MAYBE State\n\n");

    /* Force reduced to half (simulating lossy channel) */
    msg->spring.force = 0.5;
    msg->spring.extension = nsigii_full_extension(&msg->spring);

    double collapse_ratio = nsigii_collapse_ratio(&msg->spring);
    printf("Reduced force: %.2f → extension: %.4f → ratio: %.4f\n",
           msg->spring.force, msg->spring.extension, collapse_ratio);

    consensus = nsigii_spring_consensus(&msg->spring);
    printf("Consensus now: %s\n\n",
           consensus == MAG_CONSENSUS_MAYBE ? "MAYBE ?" : "other");

    /* ===================================================================
     * STEP 4: Initialize MAYBE resolver with human GPS position
     * ===================================================================
     */

    printf("[STEP 4] Initialize MAYBE Resolver — Track Human Position\n\n");

    /* Bob's initial position (San Francisco Bay Area) */
    nsigii_gps_point_t bob_initial = {
        .latitude = 37.7749,
        .longitude = -122.4194,
        .altitude = 10.0,
        .timestamp = mock_time
    };

    nsigii_maybe_resolver_t *resolver = nsigii_maybe_resolver_create(
        msg, bob_initial, mock_time);

    if (!resolver) {
        fprintf(stderr, "Resolver creation failed\n");
        free(msg);
        return 1;
    }

    printf("\n");

    /* ===================================================================
     * STEP 5: Simulate human drifting (moving away from drone)
     * ===================================================================
     */

    printf("[STEP 5] Human Moves Away — Update GPS Lattice\n\n");

    /* After 2 seconds, Bob has moved ~20 meters east */
    mock_time += 2.0;
    nsigii_gps_point_t bob_position_2 = {
        .latitude = 37.7749,
        .longitude = -122.4194 + 0.0002,  /* ~20 meters east */
        .altitude = 10.0,
        .timestamp = mock_time
    };

    resolver->human_position.previous = resolver->human_position.current;
    resolver->human_position.current = bob_position_2;
    nsigii_update_drift(&resolver->human_position);

    printf("\n");

    /* ===================================================================
     * STEP 6: Escalate via fractional rotation (½ rotation)
     * ===================================================================
     */

    printf("[STEP 6] EZE ESCALATES via Fractional Rotation\n\n");

    double elapsed = mock_time - resolver->maybe_entered_at;
    printf("Time elapsed in MAYBE: %.3f s (threshold: %.3f s)\n\n",
           elapsed, NSIGII_DECAY_CONSTANT);

    consensus = nsigii_eze_resolve_maybe(resolver, elapsed);

    printf("Result: %s\n\n",
           consensus == MAG_CONSENSUS_YES ? "SUCCESS ✓" :
           consensus == MAG_CONSENSUS_MAYBE ? "Still waiting..." :
           "FAILED ✗");

    /* ===================================================================
     * STEP 7: Second escalation (¼ rotation) if needed
     * ===================================================================
     */

    if (consensus == MAG_CONSENSUS_MAYBE) {
        printf("[STEP 7] Escalate to Next Level (¼ Rotation)\n\n");

        mock_time += 0.1;  /* Another 100ms passes */
        resolver->human_position.previous = resolver->human_position.current;
        resolver->human_position.current = (nsigii_gps_point_t) {
            .latitude = 37.7749,
            .longitude = -122.4194 + 0.0004,  /* Another ~20 meters */
            .altitude = 10.0,
            .timestamp = mock_time
        };

        nsigii_update_drift(&resolver->human_position);

        elapsed = mock_time - resolver->maybe_entered_at;
        consensus = nsigii_eze_resolve_maybe(resolver, elapsed);

        printf("Result: %s\n\n",
               consensus == MAG_CONSENSUS_YES ? "SUCCESS ✓" : "Still escalating...");
    }

    /* ===================================================================
     * STEP 8: Apply drift compensation
     * ===================================================================
     */

    printf("[STEP 8] Drift Compensation — Adjust Force for Human Movement\n\n");

    nsigii_bearing_t drone_bearing = { .degrees = 90.0 };    /* Drone flying east */
    nsigii_bearing_t human_drift = { .degrees = 90.0 };      /* Human moving east */

    nsigii_compensate_force_for_drift(resolver, drone_bearing, human_drift);

    printf("\n");

    /* ===================================================================
     * STEP 9: Predict human future position
     * ===================================================================
     */

    printf("[STEP 9] Predict Human Position in 5 Seconds\n\n");

    nsigii_gps_point_t bob_future = nsigii_predict_human_position(
        &resolver->human_position, 5.0);

    printf("Predicted position: (%.6f, %.6f)\n\n", 
           bob_future.latitude, bob_future.longitude);

    /* ===================================================================
     * STEP 10: OBI collapse (receive) once consensus=YES
     * ===================================================================
     */

    if (consensus == MAG_CONSENSUS_YES) {
        printf("[STEP 10] OBI Collapse — Message Received by Bob\n\n");

        nsigii_rights_record_t rights = {0};
        bool received = nsigii_obi_receive(msg, &rights);

        if (received) {
            printf("✓ Message collapsed and sealed\n");
            printf("  Bob (OBI) now holds IRREVOCABLE ownership\n");
            printf("  Sender (UCHE) rights REVOKED\n");
            printf("  Collapse signature: %s\n\n", msg->collapse_sig);

            /* Forensic record */
            char forensic_buffer[1024];
            nsigii_mag_forensic_record(msg, &rights, forensic_buffer, sizeof(forensic_buffer));
            printf("%s\n", forensic_buffer);
        }
    }

    /* ===================================================================
     * STEP 11: Log MAYBE resolution details
     * ===================================================================
     */

    printf("[STEP 11] MAYBE Resolution Forensic Log\n\n");

    char resolver_log[1024];
    nsigii_maybe_resolver_log(resolver, resolver_log, sizeof(resolver_log));
    printf("%s\n", resolver_log);

    /* ===================================================================
     * CLEANUP
     * ===================================================================
     */

    nsigii_maybe_resolver_destroy(resolver);
    free(msg);

    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║ DRONE DELIVERY COMPLETE — Food, Water, Shelter Delivered  ║\n");
    printf("║ Human Rights Message Secured with NSIGII Constitutional    ║\n");
    printf("║ Collapse = Received | AXIOM ENFORCED                       ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");

    return 0;
}
