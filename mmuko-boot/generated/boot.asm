; AUTO-GENERATED from MMUKO-OS.txt — do not edit by hand
; Skeleton only: map canonical phases and handoff fields into the boot sector implementation
; Filesystem target: FAT12 image mmuko-os.img
; Handoff structure: MMUKO_BOOT_HANDOFF magic=MMKO revision=0x0001

BITS 16
ORG 0x7C00

; Canonical phase order
;   1. PHASE_NEED_STATE_INIT (N) — Need-state initialization
;   2. PHASE_SAFETY_SCAN (S) — Safety scan
;   3. PHASE_IDENTITY_CALIBRATION (I) — Identity calibration
;   4. PHASE_GOVERNANCE_CHECK (G) — Governance check
;   5. PHASE_INTERNAL_PROBE (I) — Internal probe
;   6. PHASE_INTEGRITY_VERIFICATION (I) — Integrity verification

; Required handoff fields to populate before kernel transfer:
;   completed_phases
;   last_completed_phase
;   filesystem_target
;   kernel_path
;   artifact_manifest_path
;   config_path
;   kernel_entry_segment
;   kernel_entry_offset
;   validation_flags
;   handoff_checksum

start:
    ; TODO: implement canonical phase execution and populate MMUKO_BOOT_HANDOFF
    cli
    hlt
