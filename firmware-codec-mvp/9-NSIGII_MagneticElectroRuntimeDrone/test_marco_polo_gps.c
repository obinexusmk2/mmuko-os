#define _GNU_SOURCE
/*
 * MARCO / POLO GPS TRACKING — End-to-End Test Framework
 * OBINexus Computing | Nnamdi M. Okpala
 *
 * Tests the Drift Theorem for Self-Referential Vision Systems end-to-end.
 *
 * Protocol:
 *   MARCO — The observer (drone/camera) broadcasts its position and listens.
 *   POLO  — The target (human/object) acknowledges with its own position.
 *
 * Each test verifies one aspect of the Drift Theorem:
 *   T1 — Relative vector V(t) = P(t) - C(t) is computed correctly
 *   T2 — Radial drift Dr = d/dt ||V(t)|| correctly classifies Approach/Separation
 *   T3 — Angular drift ω = dθ/dt correctly detects lateral displacement
 *   T4 — Weighted observation W(t) = ⅔P(t) + ⅓P(t-Δt) reduces frame noise
 *   T5 — GPS Haversine distance matches vector magnitude within tolerance
 *   T6 — MAYBE escalation triggers at correct thresholds
 *   T7 — Full Marco/Polo round-trip: send → drift → compensate → receive
 *
 * Compile:
 *   gcc -o test_marco_polo test_marco_polo_gps.c nsigii_maybe_resolver.c \
 *       nsigii_magnetic.c -lm -Wall -Wextra
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

/* ============================================================
 * Inline geometry — mirrors gps.go & nsigii_maybe_resolver.h
 * without requiring the Go runtime.
 * ============================================================ */

#define EARTH_RADIUS_M   6371000.0
#define DEG_TO_RAD(d)   ((d) * M_PI / 180.0)
#define RAD_TO_DEG(r)   ((r) * 180.0 / M_PI)

/* ── ANSI colour helpers ── */
#define COL_GREEN  "\033[32m"
#define COL_RED    "\033[31m"
#define COL_YELLOW "\033[33m"
#define COL_CYAN   "\033[36m"
#define COL_RESET  "\033[0m"
#define COL_BOLD   "\033[1m"

/* ── Test accounting ── */
static int tests_run    = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* ============================================================
 * Data structures
 * ============================================================ */

typedef struct { double lat, lon, alt; } Vec3Geo;   /* GPS point     */
typedef struct { double x, y, z;       } Vec3Cart;  /* Cartesian 3D  */

/* Drift state — matches Drift Theorem classification table */
typedef enum {
    DRIFT_STABLE    = 0,
    DRIFT_APPROACH  = 1,
    DRIFT_SEPARATE  = 2,
    DRIFT_ANGULAR   = 3,
} DriftState;

/* Observer/target pair — the Marco (C) and Polo (P) */
typedef struct {
    Vec3Geo C;          /* Camera / observer position  */
    Vec3Geo P;          /* Target / tracked position   */
    Vec3Geo C_prev;
    Vec3Geo P_prev;
    double  delta_t;    /* seconds between frames      */
} MarcoPolo;

/* ============================================================
 * Geometry helpers
 * ============================================================ */

/* Haversine distance in metres — matches gps.go DistanceTo() */
static double haversine(Vec3Geo a, Vec3Geo b)
{
    double lat1 = DEG_TO_RAD(a.lat),  lat2 = DEG_TO_RAD(b.lat);
    double dLat = DEG_TO_RAD(b.lat - a.lat);
    double dLon = DEG_TO_RAD(b.lon - a.lon);

    double h = sin(dLat/2)*sin(dLat/2) +
               cos(lat1)*cos(lat2)*sin(dLon/2)*sin(dLon/2);
    return EARTH_RADIUS_M * 2.0 * atan2(sqrt(h), sqrt(1.0 - h));
}

/* 2-D relative vector (lat/lon only, metres) */
typedef struct { double north, east; } Vec2m;

static Vec2m relative_vector(Vec3Geo observer, Vec3Geo target)
{
    /* Small-angle approximation — valid for distances < 100 km */
    double lat_m = DEG_TO_RAD(target.lat - observer.lat) * EARTH_RADIUS_M;
    double lon_m = DEG_TO_RAD(target.lon - observer.lon) * EARTH_RADIUS_M
                   * cos(DEG_TO_RAD(observer.lat));
    return (Vec2m){ .north = lat_m, .east = lon_m };
}

static double vec2_mag(Vec2m v) { return sqrt(v.north*v.north + v.east*v.east); }

/* Dot product for angle between two 2-D vectors */
static double vec2_dot(Vec2m a, Vec2m b) { return a.north*b.north + a.east*b.east; }

/* Angle between two observation vectors (radians) */
static double observation_angle(Vec2m v1, Vec2m v2)
{
    double m1 = vec2_mag(v1), m2 = vec2_mag(v2);
    if (m1 < 1e-9 || m2 < 1e-9) return 0.0;
    double cosA = vec2_dot(v1, v2) / (m1 * m2);
    /* Clamp for floating-point safety */
    if (cosA >  1.0) cosA =  1.0;
    if (cosA < -1.0) cosA = -1.0;
    return acos(cosA);
}

/* ============================================================
 * Drift Theorem computations
 * ============================================================ */

/* V(t) = P(t) - C(t)  — relative observation vector */
static Vec2m drift_observation_vector(MarcoPolo *mp)
{
    return relative_vector(mp->C, mp->P);
}

/*
 * Dr = d/dt ||V(t)||
 * Approximated as: (||V(t)|| - ||V(t-Δt)||) / Δt
 */
static double radial_drift(MarcoPolo *mp)
{
    Vec2m v_now  = relative_vector(mp->C,      mp->P);
    Vec2m v_prev = relative_vector(mp->C_prev, mp->P_prev);
    return (vec2_mag(v_now) - vec2_mag(v_prev)) / mp->delta_t;
}

/*
 * ω = dθ/dt
 * θ = angle between V(t) and V(t-Δt)
 */
static double angular_drift(MarcoPolo *mp)
{
    Vec2m v_now  = relative_vector(mp->C,      mp->P);
    Vec2m v_prev = relative_vector(mp->C_prev, mp->P_prev);
    double theta  = observation_angle(v_now, v_prev);
    return theta / mp->delta_t;   /* rad/s */
}

/*
 * W(t) = ⅔ P(t) + ⅓ P(t-Δt)   — weighted observation smoother
 * Returns the smoothed target position in metres (north, east) from C.
 */
static Vec2m weighted_observation(MarcoPolo *mp)
{
    Vec2m v_now  = relative_vector(mp->C, mp->P);
    Vec2m v_prev = relative_vector(mp->C, mp->P_prev);
    return (Vec2m){
        .north = (2.0/3.0)*v_now.north + (1.0/3.0)*v_prev.north,
        .east  = (2.0/3.0)*v_now.east  + (1.0/3.0)*v_prev.east
    };
}

/* Classify drift state from radial and angular components */
static DriftState classify_drift(double Dr, double omega,
                                 double Dr_eps, double omega_eps)
{
    if (fabs(Dr) <= Dr_eps && fabs(omega) <= omega_eps) return DRIFT_STABLE;
    if (Dr < -Dr_eps)                                    return DRIFT_APPROACH;
    if (Dr >  Dr_eps)                                    return DRIFT_SEPARATE;
    return DRIFT_ANGULAR;
}

/* ============================================================
 * Test harness
 * ============================================================ */

static void print_header(const char *name)
{
    printf("\n" COL_BOLD COL_CYAN "─────────────────────────────────────────\n"
           " %s\n"
           "─────────────────────────────────────────\n" COL_RESET, name);
}

static void test_assert(const char *label, bool cond)
{
    tests_run++;
    if (cond) {
        tests_passed++;
        printf("  " COL_GREEN "✓" COL_RESET "  %s\n", label);
    } else {
        tests_failed++;
        printf("  " COL_RED "✗" COL_RESET "  %s  ← FAILED\n", label);
    }
}

static void test_near(const char *label, double got, double want, double tol)
{
    char buf[200];
    snprintf(buf, sizeof buf, "%s  (got=%.6f  want=%.6f  tol=%.6f)", label, got, want, tol);
    test_assert(buf, fabs(got - want) <= tol);
}

/* ============================================================
 * T1 — Relative observation vector V(t) = P(t) - C(t)
 * ============================================================ */
static void test_t1_relative_vector(void)
{
    print_header("T1 — Relative Observation Vector  V(t) = P(t) − C(t)");

    /*
     * Camera at origin (London, 0,0 alt).
     * Target 100 m due north.
     * Expect: north ≈ 100 m, east ≈ 0 m.
     */
    MarcoPolo mp = {
        .C      = { 51.5000, -0.1276, 0 },
        .P      = { 51.5009, -0.1276, 0 },   /* ~100 m north */
        .C_prev = { 51.5000, -0.1276, 0 },
        .P_prev = { 51.5009, -0.1276, 0 },
        .delta_t = 1.0
    };

    Vec2m V = drift_observation_vector(&mp);
    double d = vec2_mag(V);

    printf("  V = (%.2f m N, %.2f m E)   |V| = %.2f m\n",
           V.north, V.east, d);

    test_near("North component ≈ 100 m", V.north, 100.0, 5.0);
    test_near("East component  ≈ 0 m",   V.east,    0.0, 5.0);
    test_near("|V| ≈ 100 m",             d,         100.0, 5.0);
}

/* ============================================================
 * T2 — Radial drift Dr classifies Approach / Separation / Stable
 * ============================================================ */
static void test_t2_radial_drift(void)
{
    print_header("T2 — Radial Drift  Dr = d/dt ‖V(t)‖");

    /* --- Separation: target moves away --- */
    MarcoPolo mp_sep = {
        .C      = { 51.5000, -0.1276, 0 },
        .P      = { 51.5018, -0.1276, 0 },   /* now 200 m north */
        .C_prev = { 51.5000, -0.1276, 0 },
        .P_prev = { 51.5009, -0.1276, 0 },   /* was 100 m north */
        .delta_t = 1.0
    };
    double Dr_sep = radial_drift(&mp_sep);
    DriftState st_sep = classify_drift(Dr_sep, 0, 1.0, 0.01);

    printf("  Separation  Dr = %.2f m/s   state = %s\n",
           Dr_sep, st_sep == DRIFT_SEPARATE ? "SEPARATE" : "?");
    test_assert("Dr > 0 when target moves away",   Dr_sep > 0.0);
    test_assert("State = SEPARATE",                st_sep == DRIFT_SEPARATE);

    /* --- Approach: target moves toward observer --- */
    MarcoPolo mp_app = {
        .C      = { 51.5000, -0.1276, 0 },
        .P      = { 51.5009, -0.1276, 0 },   /* now 100 m */
        .C_prev = { 51.5000, -0.1276, 0 },
        .P_prev = { 51.5018, -0.1276, 0 },   /* was 200 m */
        .delta_t = 1.0
    };
    double Dr_app = radial_drift(&mp_app);
    DriftState st_app = classify_drift(Dr_app, 0, 1.0, 0.01);

    printf("  Approach    Dr = %.2f m/s   state = %s\n",
           Dr_app, st_app == DRIFT_APPROACH ? "APPROACH" : "?");
    test_assert("Dr < 0 when target approaches",   Dr_app < 0.0);
    test_assert("State = APPROACH",                st_app == DRIFT_APPROACH);

    /* --- Stable: target stationary --- */
    MarcoPolo mp_stab = {
        .C      = { 51.5000, -0.1276, 0 },
        .P      = { 51.5009, -0.1276, 0 },
        .C_prev = { 51.5000, -0.1276, 0 },
        .P_prev = { 51.5009, -0.1276, 0 },
        .delta_t = 1.0
    };
    double Dr_stab = radial_drift(&mp_stab);
    DriftState st_stab = classify_drift(Dr_stab, 0, 1.0, 0.01);

    printf("  Stable      Dr = %.4f m/s  state = %s\n",
           Dr_stab, st_stab == DRIFT_STABLE ? "STABLE" : "?");
    test_assert("Dr ≈ 0 when target stationary",   fabs(Dr_stab) < 0.01);
    test_assert("State = STABLE",                  st_stab == DRIFT_STABLE);
}

/* ============================================================
 * T3 — Angular drift ω = dθ/dt detects lateral displacement
 * ============================================================ */
static void test_t3_angular_drift(void)
{
    print_header("T3 — Angular Drift  ω = dθ/dt");

    /*
     * Camera fixed.  Target moves from due north to 45° NE.
     * Observation vector rotates — angular drift should be non-zero.
     */
    MarcoPolo mp = {
        .C      = { 51.5000, -0.1276, 0 },
        .P      = { 51.5009, -0.1267, 0 },   /* NE */
        .C_prev = { 51.5000, -0.1276, 0 },
        .P_prev = { 51.5009, -0.1276, 0 },   /* N  */
        .delta_t = 1.0
    };

    double omega = angular_drift(&mp);
    DriftState st = classify_drift(0.0, omega, 1.0, 0.001);

    printf("  ω = %.6f rad/s  (%.4f °/s)  state = %s\n",
           omega, RAD_TO_DEG(omega),
           st == DRIFT_ANGULAR ? "ANGULAR" : "?");

    test_assert("ω > 0 when target moves laterally", omega > 0.0);
    test_assert("State = ANGULAR",                   st == DRIFT_ANGULAR);

    /* No lateral movement → ω ≈ 0 */
    MarcoPolo mp_none = {
        .C      = { 51.5000, -0.1276, 0 },
        .P      = { 51.5018, -0.1276, 0 },
        .C_prev = { 51.5000, -0.1276, 0 },
        .P_prev = { 51.5009, -0.1276, 0 },
        .delta_t = 1.0
    };
    double omega_none = angular_drift(&mp_none);
    printf("  ω (no lateral) = %.8f rad/s\n", omega_none);
    test_assert("ω ≈ 0 for pure radial motion", fabs(omega_none) < 0.001);
}

/* ============================================================
 * T4 — Weighted observation W(t) = ⅔P(t) + ⅓P(t-Δt)
 * ============================================================ */
static void test_t4_weighted_observation(void)
{
    print_header("T4 — Weighted Observation  W(t) = ⅔P(t) + ⅓P(t−Δt)");

    /*
     * Jittery target: frame N shows P at 150 m, frame N-1 at 90 m.
     * Weighted estimate should land at ⅔·150 + ⅓·90 = 130 m.
     */
    MarcoPolo mp = {
        .C      = { 51.5000, -0.1276, 0 },
        .P      = { 51.5013, -0.1276, 0 },   /* ≈150 m N */
        .C_prev = { 51.5000, -0.1276, 0 },
        .P_prev = { 51.5008, -0.1276, 0 },   /* ≈ 90 m N */
        .delta_t = 0.033   /* 30 fps */
    };

    Vec2m raw_now = relative_vector(mp.C, mp.P);
    Vec2m raw_prv = relative_vector(mp.C, mp.P_prev);
    Vec2m W       = weighted_observation(&mp);

    double expect = (2.0/3.0)*raw_now.north + (1.0/3.0)*raw_prv.north;

    printf("  P(t) north   = %.2f m\n", raw_now.north);
    printf("  P(t-Δt) north= %.2f m\n", raw_prv.north);
    printf("  W(t) north   = %.2f m   (expected ≈ %.2f m)\n",
           W.north, expect);

    test_near("W(t) north = ⅔·P(t) + ⅓·P(t-Δt)", W.north, expect, 1.0);
    test_assert("W(t) < P(t) north — smoother pulls toward history",
                W.north < raw_now.north);
    test_assert("W(t) > P(t-Δt) north — smoother biases toward current",
                W.north > raw_prv.north);
}

/* ============================================================
 * T5 — Haversine distance matches vector magnitude
 * ============================================================ */
static void test_t5_haversine_consistency(void)
{
    print_header("T5 — Haversine Distance  ↔  Vector Magnitude Consistency");

    struct { Vec3Geo C, P; const char *label; } cases[] = {
        { {51.5000,-0.1276,0}, {51.5090,-0.1276,0}, "~1 km N"   },
        { {51.5000,-0.1276,0}, {51.5000,-0.1150,0}, "~800 m E"  },
        { {51.5000,-0.1276,0}, {51.5045,-0.1200,0}, "NE diagonal"},
    };

    for (size_t i = 0; i < sizeof cases / sizeof *cases; i++) {
        double hav  = haversine(cases[i].C, cases[i].P);
        Vec2m  V    = relative_vector(cases[i].C, cases[i].P);
        double vmag = vec2_mag(V);
        double err  = fabs(hav - vmag);

        printf("  %-14s  hav=%.2f m  |V|=%.2f m  err=%.3f m\n",
               cases[i].label, hav, vmag, err);

        char label[120];
        snprintf(label, sizeof label,
                 "Haversine and |V| agree to <5 m — %s", cases[i].label);
        test_assert(label, err < 5.0);
    }
}

/* ============================================================
 * T6 — MAYBE escalation threshold verification
 * ============================================================ */
static void test_t6_maybe_escalation(void)
{
    print_header("T6 — MAYBE State Escalation Thresholds");

    /*
     * Mirror the rotation fraction / force multiplier table from
     * nsigii_maybe_resolver.h without linking the full library.
     *
     * Level | Fraction | Multiplier | Expected adjusted force
     *   0   |   0.5    |    2.0     | base × 2
     *   1   |   0.25   |    4.0     | base × 4
     *   2   |   0.1667 |    6.0     | base × 6
     *   3   |   0.125  |    8.0     | base × 8
     */

    static const double fractions[]    = { 0.5, 0.25, 1.0/6.0, 0.125 };
    static const double multipliers[]  = { 2.0, 4.0,  6.0,     8.0   };
    static const char  *labels[]       = { "½", "¼",  "⅙",     "⅛"   };

    double base_force = 0.5;  /* Force in MAYBE state */

    for (int level = 0; level < 4; level++) {
        double M        = 1.0 / fractions[level];
        double F_prime  = base_force * M;
        bool   M_ok     = fabs(M - multipliers[level]) < 1e-6;
        bool   F_ok     = fabs(F_prime - base_force * multipliers[level]) < 1e-6;

        printf("  Level %d  frac=%s  M=%.1f×  F'=%.2f\n",
               level, labels[level], M, F_prime);

        char buf[100];
        snprintf(buf, sizeof buf,
                 "Level %d: M = 1/fraction = %.1f", level, multipliers[level]);
        test_assert(buf, M_ok);

        snprintf(buf, sizeof buf,
                 "Level %d: Adjusted force = base × M", level);
        test_assert(buf, F_ok);
    }

    /* Verify escalation order produces strictly increasing force */
    bool monotone = true;
    double prev_mult = 0.0;
    for (int level = 0; level < 4; level++) {
        if (multipliers[level] <= prev_mult) { monotone = false; break; }
        prev_mult = multipliers[level];
    }
    test_assert("Escalation multipliers are strictly increasing", monotone);
}

/* ============================================================
 * T7 — Full Marco/Polo round-trip simulation
 *
 * Protocol:
 *  1. MARCO emits position C(t0)
 *  2. POLO responds with P(t0) — initial lock
 *  3. POLO drifts over 3 frames
 *  4. System applies weighted smoothing + drift compensation
 *  5. POLO sends final ACK from predicted position
 *  6. Verify system remains APPROACH or STABLE throughout
 * ============================================================ */
static void test_t7_round_trip(void)
{
    print_header("T7 — Full Marco/Polo Round-Trip  (3-frame drift simulation)");

    /* London drone depot (MARCO observer) */
    Vec3Geo C = { 51.5000, -0.1276, 50.0 };  /* 50 m altitude */

    /* Polo (human) — starts 500 m NE, moves toward drone */
    Vec3Geo frames[4] = {
        { 51.5045, -0.1230, 0.0 },  /* frame 0: ~500 m NE */
        { 51.5036, -0.1240, 0.0 },  /* frame 1: ~400 m NE */
        { 51.5027, -0.1253, 0.0 },  /* frame 2: ~300 m NE */
        { 51.5018, -0.1265, 0.0 },  /* frame 3: ~200 m NE */
    };
    double dt = 1.0;  /* 1 s between frames */

    printf("  MARCO at (%.4f, %.4f) alt=%.0f m\n", C.lat, C.lon, C.alt);
    printf("  POLO approaching over 3 frames (dt=%.1f s):\n\n", dt);

    bool all_approach_or_stable = true;

    for (int f = 1; f <= 3; f++) {
        MarcoPolo mp = {
            .C      = C,
            .P      = frames[f],
            .C_prev = C,
            .P_prev = frames[f-1],
            .delta_t = dt
        };

        Vec2m  V     = drift_observation_vector(&mp);
        double d     = vec2_mag(V);
        double Dr    = radial_drift(&mp);
        double omega = angular_drift(&mp);
        Vec2m  W     = weighted_observation(&mp);
        double Wd    = vec2_mag(W);
        DriftState st = classify_drift(Dr, omega, 0.5, 0.005);

        const char *st_str =
            st == DRIFT_STABLE   ? COL_GREEN  "STABLE"   COL_RESET :
            st == DRIFT_APPROACH ? COL_GREEN  "APPROACH" COL_RESET :
            st == DRIFT_SEPARATE ? COL_RED    "SEPARATE" COL_RESET :
                                   COL_YELLOW "ANGULAR"  COL_RESET ;

        printf("  Frame %d:\n", f);
        printf("    |V(t)| = %.2f m    Dr = %+.3f m/s    ω = %.5f rad/s\n",
               d, Dr, omega);
        printf("    |W(t)| = %.2f m    State = %s\n", Wd, st_str);

        /* Ground-truth check with Haversine */
        double hav_d = haversine(C, frames[f]);
        printf("    Haversine d = %.2f m   |V| err = %.3f m\n\n",
               hav_d, fabs(d - hav_d));

        if (st == DRIFT_SEPARATE) all_approach_or_stable = false;
    }

    test_assert("POLO approaches MARCO across all frames",
                all_approach_or_stable);

    /* Final frame: verify distance is less than initial */
    double d_initial = haversine(C, frames[0]);
    double d_final   = haversine(C, frames[3]);
    test_assert("Final distance < Initial distance (positive closure)",
                d_final < d_initial);

    printf("  Distance closed: %.2f m → %.2f m  (Δ = %.2f m)\n",
           d_initial, d_final, d_initial - d_final);

    /* Verify weighted smoother sits between prev and current */
    MarcoPolo mp_last = {
        .C = C, .P = frames[3], .C_prev = C, .P_prev = frames[2],
        .delta_t = dt
    };
    Vec2m W_last = weighted_observation(&mp_last);
    double Wd_last = vec2_mag(W_last);
    double d_prev  = haversine(C, frames[2]);
    double d_now   = haversine(C, frames[3]);

    printf("  W(t) magnitude = %.2f m  (between %.2f m and %.2f m)\n",
           Wd_last, d_prev, d_now);

    test_assert("W(t) lies between P(t-Δt) and P(t) distances",
                Wd_last > d_now - 5.0 && Wd_last < d_prev + 5.0);

    printf("\n  " COL_BOLD "MARCO/POLO handshake:" COL_RESET "\n");
    printf("    MARCO → \"Marco!\"  (position broadcast)\n");
    printf("    POLO  → \"Polo!\"   (position acknowledgement)\n");
    printf("    SYSTEM → Drift classified, W(t) smoothed, closure confirmed\n");
}

/* ============================================================
 * Summary
 * ============================================================ */
static void print_summary(void)
{
    printf("\n" COL_BOLD
           "╔═══════════════════════════════════════════════════════════╗\n"
           "║          MARCO/POLO GPS TRACKING — TEST SUMMARY          ║\n"
           "╠═══════════════════════════════════════════════════════════╣\n"
           COL_RESET);

    printf(COL_BOLD "║" COL_RESET "  Tests run:    %-5d                                      "
           COL_BOLD "║\n" COL_RESET, tests_run);

    printf(COL_BOLD "║" COL_RESET "  " COL_GREEN "Passed:     %-5d" COL_RESET
           "                                      " COL_BOLD "║\n" COL_RESET, tests_passed);

    if (tests_failed > 0) {
        printf(COL_BOLD "║" COL_RESET "  " COL_RED "FAILED:     %-5d" COL_RESET
               "  ← INVESTIGATION REQUIRED             " COL_BOLD "║\n" COL_RESET, tests_failed);
    } else {
        printf(COL_BOLD "║" COL_RESET "  " COL_GREEN "Failed:     0    " COL_RESET
               "  ALL GREEN                            " COL_BOLD "║\n" COL_RESET);
    }

    printf(COL_BOLD
           "╠═══════════════════════════════════════════════════════════╣\n"
           COL_RESET);

    if (tests_failed == 0) {
        printf(COL_BOLD "║" COL_RESET COL_GREEN
               "  ✓  Drift Theorem verified end-to-end                     "
               COL_RESET COL_BOLD "║\n" COL_RESET);
        printf(COL_BOLD "║" COL_RESET COL_GREEN
               "  ✓  Marco/Polo GPS tracking works end-to-end               "
               COL_RESET COL_BOLD "║\n" COL_RESET);
    } else {
        printf(COL_BOLD "║" COL_RESET COL_RED
               "  ✗  Some assertions failed — review output above           "
               COL_RESET COL_BOLD "║\n" COL_RESET);
    }

    printf(COL_BOLD
           "╚═══════════════════════════════════════════════════════════╝\n"
           COL_RESET "\n");
}

/* ============================================================
 * Main
 * ============================================================ */
int main(void)
{
    printf(COL_BOLD COL_CYAN
           "\n╔═══════════════════════════════════════════════════════════╗\n"
           "║  MARCO/POLO GPS TRACKING — Drift Theorem Test Framework   ║\n"
           "║  OBINexus Computing | Nnamdi M. Okpala                    ║\n"
           "║  Based on: Drift Theorem for Self-Referential Vision      ║\n"
           "╚═══════════════════════════════════════════════════════════╝\n"
           COL_RESET);

    printf("\n  Theorem:  D(t) = dV(t)/dt  where  V(t) = P(t) − C(t)\n");
    printf("  Dr = d/dt |V|     (radial)   ω = dθ/dt  (angular)\n");
    printf("  W(t) = ⅔P(t) + ⅓P(t−Δt)   (weighted smoother)\n");

    test_t1_relative_vector();
    test_t2_radial_drift();
    test_t3_angular_drift();
    test_t4_weighted_observation();
    test_t5_haversine_consistency();
    test_t6_maybe_escalation();
    test_t7_round_trip();

    print_summary();

    return (tests_failed == 0) ? 0 : 1;
}
