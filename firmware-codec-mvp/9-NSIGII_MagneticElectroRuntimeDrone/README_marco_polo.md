# Marco/Polo GPS Tracking — Drift Theorem Test Framework

**OBINexus Computing | Nnamdi M. Okpala**  
`test_marco_polo_gps.c` — Standalone C test suite  
Status: **30/30 PASS** — verified on Linux and Windows (PowerShell 7.5.4 / GCC)

---

## What This Is

This is the end-to-end verification suite for the **Drift Theorem for Self-Referential Vision Systems**. It tests the complete mathematical pipeline that allows a drone or camera to track a moving human using pure vector mathematics — no machine learning, no heuristics.

The name comes from the protocol: **MARCO** is the observer (drone/camera) that broadcasts its position, and **POLO** is the target (human) that acknowledges with its own position. The system continuously computes the spatial relationship between them and classifies their relative motion.

---

## The Drift Theorem

```
V(t) = P(t) − C(t)          Relative observation vector
D(t) = dV(t)/dt              Drift vector (time derivative)
Dr   = d/dt ‖V(t)‖           Radial drift (approach / separation)
ω    = dθ/dt                 Angular drift (lateral displacement)
W(t) = ⅔P(t) + ⅓P(t−Δt)    Weighted smoother (noise reduction)
```

Where `C(t)` is the camera/drone position and `P(t)` is the tracked target position.

### Drift State Classification

| State    | Condition              | Meaning                        |
|----------|------------------------|--------------------------------|
| APPROACH | Dr < 0                 | Target moving toward observer  |
| SEPARATE | Dr > 0                 | Target moving away             |
| ANGULAR  | ω > threshold          | Lateral displacement detected  |
| STABLE   | Dr ≈ 0 and ω ≈ 0       | No significant motion          |

---

## Compile and Run

### Linux / macOS

```bash
gcc -o test_marco_polo test_marco_polo_gps.c -lm -std=c11
./test_marco_polo
```

### Windows (PowerShell)

```powershell
gcc -o test_marco_polo test_marco_polo_gps.c -lm -std=c11
chcp 65001
$OutputEncoding = [Console]::OutputEncoding = [Text.Encoding]::UTF8
./test_marco_polo
```

The `chcp 65001` step is required for correct rendering of the box-drawing characters and Unicode math symbols (ω, θ, Δ, ⅔, ⅓) in the Windows console. The tests pass either way — it is purely cosmetic.

To save a clean copy of the output:

```powershell
./test_marco_polo | Out-File -Encoding utf8 result.txt
```

### Dependencies

- GCC (any modern version)
- Standard C library (`math.h`, `assert.h`, `string.h`)
- No external libraries required
- No NSIGII runtime linkage required (the test is self-contained)

---

## Test Suite — T1 through T7

### T1 — Relative Observation Vector

Verifies that `V(t) = P(t) − C(t)` is computed correctly using the small-angle geodetic approximation. Places MARCO at a fixed London coordinate and POLO 100 m due north. Checks that the north component is within 5 m of 100 m and the east component is within 5 m of 0 m.

### T2 — Radial Drift Classification

Verifies all three states of `Dr = d/dt ‖V(t)‖`:

- **Separation**: target steps from 100 m to 200 m — Dr is positive, state = SEPARATE
- **Approach**: target steps from 200 m to 100 m — Dr is negative, state = APPROACH
- **Stable**: target does not move — Dr = 0.0000, state = STABLE

### T3 — Angular Drift Detection

Moves the target laterally from due north to NE while the observer stays fixed. Verifies that `ω = dθ/dt` fires at 0.557 rad/s (≈ 32°/s) and that pure radial motion returns exactly `ω = 0.00000000 rad/s` with no false positives.

### T4 — Weighted Observation Smoother

Injects a jittery two-frame position sequence (144.55 m / 88.96 m). Verifies that `W(t) = ⅔P(t) + ⅓P(t−Δt)` produces exactly 126.02 m, that the result lies strictly between the two raw readings, and that it is closer to the current frame than to the previous one.

### T5 — Haversine / Vector Magnitude Consistency

Runs three distance cases (1 km north, 800 m east, NE diagonal) and confirms that the Haversine great-circle formula from `gps.go` and the small-angle vector approximation agree to within 0.019 m in the worst case — well inside any practical GPS accuracy budget.

### T6 — MAYBE State Escalation Thresholds

Verifies the fractional rotation force-multiplier chain from `nsigii_maybe_resolver.h`:

| Level | Rotation Fraction | Force Multiplier |
|-------|-------------------|------------------|
| 0     | ½                 | 2×               |
| 1     | ¼                 | 4×               |
| 2     | ⅙                 | 6×               |
| 3     | ⅛                 | 8×               |

Confirms that `M = 1 / fraction` for each level, that adjusted force equals `base × M`, and that the multiplier sequence is strictly monotone increasing so the system always escalates and never oscillates.

### T7 — Full Marco/Polo Round-Trip (3-Frame Simulation)

The integration test. MARCO is fixed at (51.5000, −0.1276) at 50 m altitude (London). POLO starts 593 m NE and approaches over three one-second frames, closing to 214 m. For each frame the test checks:

- `|V(t)|` via vector approximation
- `Dr` radial drift — all three frames must classify as APPROACH
- `ω` angular drift — increases as POLO curves inward
- `|W(t)|` weighted magnitude
- Haversine ground-truth distance (error < 0.005 m across all frames)

Final assertions confirm that POLO never classifies as SEPARATE, that total closure is 378.94 m, and that the weighted smoother sits correctly between the previous and current frame distances.

---

## Verified Output (clean, with UTF-8 encoding active)

```
╔═══════════════════════════════════════════════════════════╗
║  MARCO/POLO GPS TRACKING — Drift Theorem Test Framework   ║
║  OBINexus Computing | Nnamdi M. Okpala                    ║
║  Based on: Drift Theorem for Self-Referential Vision      ║
╚═══════════════════════════════════════════════════════════╝

  Theorem:  D(t) = dV(t)/dt  where  V(t) = P(t) − C(t)
  Dr = d/dt |V|     (radial)   ω = dθ/dt  (angular)
  W(t) = ⅔P(t) + ⅓P(t−Δt)   (weighted smoother)

  [T1–T7: 30 assertions]

╔═══════════════════════════════════════════════════════════╗
║          MARCO/POLO GPS TRACKING — TEST SUMMARY          ║
╠═══════════════════════════════════════════════════════════╣
║  Tests run:    30                                         ║
║  Passed:       30                                         ║
║  Failed:       0      ALL GREEN                           ║
╠═══════════════════════════════════════════════════════════╣
║  ✓  Drift Theorem verified end-to-end                     ║
║  ✓  Marco/Polo GPS tracking works end-to-end              ║
╚═══════════════════════════════════════════════════════════╝
```

---

## Relationship to the NSIGII Codebase

This test file is standalone but validates the mathematics underlying three NSIGII components:

- **`nsigii_maybe_resolver.c/.h`** — The MAYBE state escalation chain (T6) and GPS lattice drift tracking (T7) are the same model used in the drone delivery example (`example_drone_delivery.c`).
- **`gps.go`** — The Haversine formula in T5 mirrors `Coordinate.DistanceTo()` in the Go package exactly, confirming that the C and Go implementations of the same geometry agree to sub-centimetre precision.
- **`nsigii_magnetic.c/.h`** — The UCHE/EZE/OBI message lifecycle depends on the force escalation model verified in T6.

The weighted smoother `W(t) = ⅔P(t) + ⅓P(t−Δt)` is the same formula documented in the Drift Theorem paper and used in the OpenCV-based vision system for centroid stabilisation between frames.

---

## Pipeline Context

```
riftlang.exe → .so.a → rift.exe → gosilang → ltcodec → nsigii
                                                  ↑
                                          Marco/Polo GPS layer
                                       (Drift Theorem test suite)
```

The GPS tracking layer sits inside `ltcodec`, which anchors every payload to a spacetime fingerprint (WHO + WHERE + WHEN). The Drift Theorem test suite confirms that the WHERE component behaves correctly under motion — approach, separation, lateral drift, and noisy frames.

---

## Author and Project

**Nnamdi M. Okpala** — OBINexus Computing  
Date confirmed: 17 March 2026  
Theorem: *Drift Theorem for Self-Referential Vision Systems*  
Tested on: Ubuntu 24 (GCC) and Windows 11 (PowerShell 7.5.4 / GCC via Conda base)

*OBINexus Computing — #NoGhosting — Constitutional Computing*
