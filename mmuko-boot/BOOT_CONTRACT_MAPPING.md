# MMUKO Boot Contract Mapping Table

This table cross-references the core routines in `mmuko-boot/pseudocode/mmuko-boot.psc` with the identifiers that now implement or approximate the same contract in the assembly boot path, the hosted C runtime, and the membrane calibrator.

| Pseudocode identifier | `boot.asm` identifier | `mmuko-boot/mmuko-boot.c` identifier | `heartfull_membrane.c` identifier | Notes |
|---|---|---|---|---|
| `mmuko_boot()` | `start` | `mmuko_boot` | `membrane_calibrate` | Top-level orchestration entrypoints. |
| `init_cubit_ring()` | `write_contract` + `contract_template` | `init_cubit_ring` | `membrane_init` | Assembly has no cubit struct, so it emits the shared handoff record instead of full ring objects. |
| `resolve_state()` | implicit `t1` / `t2` branch decisions | `resolve_state` | `trinary_resolve` | Hosted runtime resolves per-bit state; membrane runtime resolves trinary scan state. |
| `lookup_superposition()` | phase labels `PH_*` plus final `status`/`reason` selection | `lookup_superposition` | `nsigii_run_phase` | All three choose phase-dependent orientation / gate outcomes. |
| `rotate_bits()` | `FL_ROTOK` flag committed before `PH_DONE` | `rotate_bits` | `compass_rotate` | Rotation is explicit in C runtime and membrane layer, implicit in assembly via verified flag. |
| `resolve_direction_from_neighbors()` | `phase_cur` / `phase_last` progression through `PH_S` | `phase2_compass_alignment` | `nsigii_run_phase` (`NSIGII_PHASE_S`) | Safety/alignment phase ensures the system has a coherent direction. |
| `flip_state()` | `.alert` / `ST_ALRT` / `ST_FAULT` branches | `flip_state` | `enzyme_apply` | Interference/fault handling maps to state repair or escalation. |
| `get_middle_base()` | `PH_G` and `phase_last` midpoint tracking | `phase4_frame_centering` | `nsigii_run_phase` (`NSIGII_PHASE_G`) | Governance is the center-of-reference step in each implementation. |
| `resolve_base_state()` | `status` / `reason` committed before `.write` | `phase5_probe` | `nsigii_run_phase` (`NSIGII_PHASE_I2`) | Probe/gate phase resolves readiness for the final handoff. |
| rotation verification block in `mmuko_boot()` | `PH_IIN` / `FL_ROTOK` / `PH_DONE` | `phase6_integrity` | `nsigii_run_phase` (`NSIGII_PHASE_I3`) | Final integrity verification before PASS/HOLD/ALERT/FAULT. |

## Shared phase identifiers

| Contract phase | `boot.asm` symbol | `mmuko-boot/mmuko_boot_contract.h` symbol | `heartfull_firmware.h` symbol |
|---|---|---|---|
| Prepare | `PH_PREP` | `MMUKO_BOOT_PHASE_PREPARE` | `membrane_init` pre-phase |
| Need | `PH_N` | `MMUKO_BOOT_PHASE_N` | `NSIGII_PHASE_N` |
| Safety | `PH_S` | `MMUKO_BOOT_PHASE_S` | `NSIGII_PHASE_S` |
| Identity | `PH_IID` | `MMUKO_BOOT_PHASE_I_IDENT` | `NSIGII_PHASE_I` |
| Governance | `PH_G` | `MMUKO_BOOT_PHASE_G` | `NSIGII_PHASE_G` |
| Probe | `PH_IPR` | `MMUKO_BOOT_PHASE_I_PROBE` | `NSIGII_PHASE_I2` |
| Integrity | `PH_IIN` | `MMUKO_BOOT_PHASE_I_INTEG` | `NSIGII_PHASE_I3` |
| Handoff | `PH_HO` | `MMUKO_BOOT_PHASE_HANDOFF` | `membrane_calibrate` gate |
| Complete | `PH_DONE` | `MMUKO_BOOT_PHASE_COMPLETE` | `MEMBRANE_PASS` / `MEMBRANE_HOLD` / `MEMBRANE_ALERT` outcome |

## Shared status identifiers

| Semantic outcome | `boot.asm` | `mmuko-boot/mmuko_boot_contract.h` | `heartfull_firmware.h` |
|---|---|---|---|
| Hold / pending | `ST_HOLD` | `MMUKO_BOOT_STATUS_HOLD` | `MEMBRANE_HOLD` |
| Pass / proceed | `ST_PASS` | `MMUKO_BOOT_STATUS_PASS` | `MEMBRANE_PASS` |
| Alert / halt | `ST_ALRT` | `MMUKO_BOOT_STATUS_ALERT` | `MEMBRANE_ALERT` |
| Fault / invariant break | `ST_FAULT` | `MMUKO_BOOT_STATUS_FAULT` | negative discriminant path in `membrane_calibrate` |
