        /* Generated file. Do not edit by hand.
         * Authoritative input: MMUKO-OS.txt
         * Primary pseudocode: pseudocode/mmuko-boot.psc
         * Parsed ENUMs: MMUKO_BOOT_OUTCOME, MMUKO_BOOT_PHASE
         * Parsed STRUCTs: MMUKO_BOOT_HANDOFF
         */
        #ifndef MMUKO_CODEGEN_H
        #define MMUKO_CODEGEN_H

        #include <stddef.h>
        #include <stdint.h>

        #ifdef __cplusplus
        extern "C" {
        #endif

        /* --- Enums parsed from mmuko-boot.psc --- */
        typedef enum {
    MMUKO_BOOT_OUTCOME_PASS = 0xAA,
    MMUKO_BOOT_OUTCOME_HOLD = 0xBB,
    MMUKO_BOOT_OUTCOME_ALERT = 0xCC
} MMUKO_BOOT_OUTCOME;

typedef enum {
    MMUKO_BOOT_PHASE_PHASE_NEED_STATE_INIT = 1,
    MMUKO_BOOT_PHASE_PHASE_SAFETY_SCAN = 2,
    MMUKO_BOOT_PHASE_PHASE_IDENTITY_CALIBRATION = 3,
    MMUKO_BOOT_PHASE_PHASE_GOVERNANCE_CHECK = 4,
    MMUKO_BOOT_PHASE_PHASE_INTERNAL_PROBE = 5,
    MMUKO_BOOT_PHASE_PHASE_INTEGRITY_VERIFICATION = 6
} MMUKO_BOOT_PHASE;

        /* --- Structs parsed from mmuko-boot.psc --- */
        typedef struct {
    char magic[4];
    uint16_t revision;
    char firmware_id[6];
    uint32_t outcome;
    uint8_t completed_phases;
    uint32_t last_completed_phase;
    const char * filesystem_target;
    const char * kernel_path;
    const char * artifact_manifest_path;
    const char * config_path;
    uint16_t kernel_entry_segment;
    uint16_t kernel_entry_offset;
    uint32_t validation_flags;
    uint32_t handoff_checksum;
} MMUKO_BOOT_HANDOFF_t;

        /* --- Phase descriptor API --- */
        typedef struct mmuko_phase_descriptor {
            const char *phase_id;
            const char *title;
            const char *summary;
        } mmuko_phase_descriptor;

        size_t mmuko_stage2_phase_count(void);
        const mmuko_phase_descriptor *mmuko_stage2_phases(void);
        const char *mmuko_stage2_boot_summary(void);
        size_t mmuko_pseudocode_source_count(void);
        const char *mmuko_pseudocode_source(size_t index);

        /* --- Boot handoff API --- */
        MMUKO_BOOT_OUTCOME mmuko_boot(MMUKO_BOOT_HANDOFF_t *handoff);
        int mmuko_verify_entry_contract(const MMUKO_BOOT_HANDOFF_t *handoff);

        #ifdef __cplusplus
        }
        #endif

        #endif /* MMUKO_CODEGEN_H */
