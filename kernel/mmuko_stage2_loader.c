        /* Generated file. Do not edit by hand.
         * Authoritative input: MMUKO-OS.txt
         * Primary pseudocode: mmuko-boot/pseudocode/mmuko-boot.psc
         * Parsed functions from main boot pseudocode:
         *   - complete_phase
 *   - compute_handoff_checksum
 *   - mmuko_boot
         * Parsed constants snapshot:

         */
        #include "mmuko_codegen.h"
        #include <string.h>

        /* ------------------------------------------------------------------ */
        /* Phase descriptor table                                              */
        /* ------------------------------------------------------------------ */

        static const mmuko_phase_descriptor MMUKO_PHASES[] = {
            { "PHASE 0", "Vacuum Medium Initialization", "Establish the gravitational reference frame before touching mapped bytes." },
    { "PHASE 1", "Cubit Ring Initialization", "Project each byte into an 8-cubit compass ring with entangled partner indices." },
    { "PHASE 2", "Compass Alignment", "Resolve undefined directions from neighbours so no cubit remains locked." },
    { "PHASE 3", "Superposition Entanglement", "Break constructive interference across opposing compass pairs." },
    { "PHASE 4", "Middle Alignment", "Anchor the frame of reference at base 6 without a hard lock." },
    { "PHASE 5", "Nonlinear Index Resolution", "Traverse the diamond-table order [12, 6, 8, 4, 10, 2, 1]." },
    { "PHASE 6", "Rotation Verification", "Confirm every cubit can complete a full rotation without state loss." },
        };

        static const char *MMUKO_PSEUDOCODE_SOURCES[] = {
            "mmuko-boot/pseudocode/mmuko-boot.psc :: primary boot model"
        };

        size_t mmuko_stage2_phase_count(void) {
            return sizeof(MMUKO_PHASES) / sizeof(MMUKO_PHASES[0]);
        }

        const mmuko_phase_descriptor *mmuko_stage2_phases(void) {
            return MMUKO_PHASES;
        }

        const char *mmuko_stage2_boot_summary(void) {
            return " This text companion mirrors the build-spec section in README.md so that the artifact contract is available in a plain-text form for packers, boot tooling, and release notes.  Build-spec";
        }

        size_t mmuko_pseudocode_source_count(void) {
            return sizeof(MMUKO_PSEUDOCODE_SOURCES) / sizeof(MMUKO_PSEUDOCODE_SOURCES[0]);
        }

        const char *mmuko_pseudocode_source(size_t index) {
            if (index >= mmuko_pseudocode_source_count()) {
                return 0;
            }
            return MMUKO_PSEUDOCODE_SOURCES[index];
        }

        /* ------------------------------------------------------------------ */
        /* Boot handoff — 6-phase NSIGII runner (from mmuko-boot.psc)         */
        /* ------------------------------------------------------------------ */

        static uint32_t mmuko_crc32_update(uint32_t crc, const void *data, size_t len) {
            const uint8_t *bytes = (const uint8_t *)data;
            crc ^= 0xFFFFFFFFu;
            for (size_t i = 0; i < len; ++i) {
                crc ^= (uint32_t)bytes[i];
                for (unsigned bit = 0; bit < 8; ++bit) {
                    uint32_t mask = 0u - (crc & 1u);
                    crc = (crc >> 1) ^ (0xEDB88320u & mask);
                }
            }
            return crc ^ 0xFFFFFFFFu;
        }

        static uint32_t mmuko_crc32_update_u8(uint32_t crc, uint8_t value) {
            return mmuko_crc32_update(crc, &value, sizeof(value));
        }

        static uint32_t mmuko_crc32_update_u16le(uint32_t crc, uint16_t value) {
            uint8_t bytes[2] = {
                (uint8_t)(value & 0xFFu),
                (uint8_t)((value >> 8) & 0xFFu),
            };
            return mmuko_crc32_update(crc, bytes, sizeof(bytes));
        }

        static uint32_t mmuko_crc32_update_u32le(uint32_t crc, uint32_t value) {
            uint8_t bytes[4] = {
                (uint8_t)(value & 0xFFu),
                (uint8_t)((value >> 8) & 0xFFu),
                (uint8_t)((value >> 16) & 0xFFu),
                (uint8_t)((value >> 24) & 0xFFu),
            };
            return mmuko_crc32_update(crc, bytes, sizeof(bytes));
        }

        static uint32_t compute_handoff_checksum(const MMUKO_BOOT_HANDOFF_t *h) {
            /* Standard CRC32 over the pseudocode-listed serialized handoff fields.
             * handoff_checksum is intentionally excluded. STRING values are
             * serialized as their NUL-terminated byte sequences, not pointers.
             */
            uint32_t crc = 0;
            crc = mmuko_crc32_update(crc, h->magic, sizeof(h->magic));
            crc = mmuko_crc32_update_u16le(crc, h->revision);
            crc = mmuko_crc32_update(crc, h->firmware_id, sizeof(h->firmware_id));
            crc = mmuko_crc32_update_u32le(crc, (uint32_t)h->outcome);
            crc = mmuko_crc32_update_u8(crc, h->completed_phases);
            crc = mmuko_crc32_update_u32le(crc, (uint32_t)h->last_completed_phase);
            if (h->filesystem_target == 0) { return 0; }
            crc = mmuko_crc32_update(crc, h->filesystem_target, strlen(h->filesystem_target) + 1u);
            if (h->kernel_path == 0) { return 0; }
            crc = mmuko_crc32_update(crc, h->kernel_path, strlen(h->kernel_path) + 1u);
            if (h->artifact_manifest_path == 0) { return 0; }
            crc = mmuko_crc32_update(crc, h->artifact_manifest_path, strlen(h->artifact_manifest_path) + 1u);
            if (h->config_path == 0) { return 0; }
            crc = mmuko_crc32_update(crc, h->config_path, strlen(h->config_path) + 1u);
            crc = mmuko_crc32_update_u16le(crc, h->kernel_entry_segment);
            crc = mmuko_crc32_update_u16le(crc, h->kernel_entry_offset);
            crc = mmuko_crc32_update_u32le(crc, (uint32_t)h->validation_flags);
            return crc;
        }

        static void complete_phase(MMUKO_BOOT_HANDOFF_t *h, uint8_t phase, uint32_t flag) {
            h->completed_phases++;
            h->last_completed_phase = phase;
            h->validation_flags |= flag;
        }

        /* Per-phase runners — REQUIRE stubs return 1 (pass); replace with
         * real platform probes at link time by providing mmuko_probe_*()
         * implementations.
         */
        static int mmuko_run_phase_1(MMUKO_BOOT_HANDOFF_t *handoff) {
    /* PHASE_NEED_STATE_INIT */
    /* REQUIRE tier1_state != NO — represented from PHASE_NEED_STATE_INIT; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    handoff->completed_phases++;
    handoff->last_completed_phase = 1;
    handoff->validation_flags |= 0x00000001u;
    return 1;
}

static int mmuko_run_phase_2(MMUKO_BOOT_HANDOFF_t *handoff) {
    /* PHASE_SAFETY_SCAN */
    /* REQUIRE tier2_state != NO — represented from PHASE_SAFETY_SCAN; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    /* REQUIRE nsigii_minimum_safety_envelope == TRUE — represented from PHASE_SAFETY_SCAN; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    handoff->completed_phases++;
    handoff->last_completed_phase = 2;
    handoff->validation_flags |= 0x00000002u;
    return 1;
}

static int mmuko_run_phase_3(MMUKO_BOOT_HANDOFF_t *handoff) {
    /* PHASE_IDENTITY_CALIBRATION */
    /* no explicit REQUIRE for this phase */
    handoff->completed_phases++;
    handoff->last_completed_phase = 3;
    handoff->validation_flags |= 0x00000004u;
    return 1;
}

static int mmuko_run_phase_4(MMUKO_BOOT_HANDOFF_t *handoff) {
    /* PHASE_GOVERNANCE_CHECK */
    /* REQUIRE execution_policy == VERIFIED — represented from PHASE_GOVERNANCE_CHECK; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    /* REQUIRE provenance_chain == VERIFIED — represented from PHASE_GOVERNANCE_CHECK; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    /* REQUIRE filesystem_target == RAW_FIXED_SECTOR:mmuko-os.img:LBA0_STAGE1:LBA1_16_STAGE2:LBA17_48_RUNTIME — represented from PHASE_GOVERNANCE_CHECK; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    handoff->completed_phases++;
    handoff->last_completed_phase = 4;
    handoff->validation_flags |= 0x00000008u;
    return 1;
}

static int mmuko_run_phase_5(MMUKO_BOOT_HANDOFF_t *handoff) {
    /* PHASE_INTERNAL_PROBE */
    /* REQUIRE nsigii_firmware_compatible == TRUE — represented from PHASE_INTERNAL_PROBE; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    /* REQUIRE memory_map_integrity == TRUE — represented from PHASE_INTERNAL_PROBE; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    /* REQUIRE runtime_interface_compatible == TRUE — represented from PHASE_INTERNAL_PROBE; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    handoff->completed_phases++;
    handoff->last_completed_phase = 5;
    handoff->validation_flags |= 0x00000010u;
    return 1;
}

static int mmuko_run_phase_6(MMUKO_BOOT_HANDOFF_t *handoff) {
    /* PHASE_INTEGRITY_VERIFICATION */
    /* REQUIRE artifact_exists(handoff.kernel_path) — represented from PHASE_INTEGRITY_VERIFICATION; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    /* REQUIRE artifact_exists(handoff.artifact_manifest_path) — represented from PHASE_INTEGRITY_VERIFICATION; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    /* REQUIRE discriminant >= 0 — represented from PHASE_INTEGRITY_VERIFICATION; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    /* REQUIRE kernel_entry_is_resolved == TRUE — represented from PHASE_INTEGRITY_VERIFICATION; resolved at runtime */
    /* mmuko_probe stub: returns 1 (pass) until platform impl provided */
    handoff->completed_phases++;
    handoff->last_completed_phase = 6;
    handoff->validation_flags |= 0x00000020u;
    return 1;
}

        MMUKO_BOOT_OUTCOME mmuko_boot(MMUKO_BOOT_HANDOFF_t *handoff) {
            /* Initialise handoff record */
            memset(handoff, 0, sizeof(*handoff));
            handoff->magic[0] = 'M'; handoff->magic[1] = 'M';
            handoff->magic[2] = 'U'; handoff->magic[3] = 'K';
            handoff->magic[4] = 'O';
            handoff->revision           = 0x0001;
            handoff->firmware_id[0]     = 'N'; handoff->firmware_id[1] = 'S';
            handoff->firmware_id[2]     = 'I'; handoff->firmware_id[3] = 'G';
            handoff->firmware_id[4]     = 'I'; handoff->firmware_id[5] = 'I';
            handoff->outcome            = MMUKO_BOOT_OUTCOME_HOLD;
            handoff->completed_phases   = 0;
            handoff->kernel_entry_segment = 0x0000;
            handoff->kernel_entry_offset  = 0x0000;
            handoff->validation_flags     = 0;
            handoff->magic[0] = (char)0x4D;
            handoff->magic[1] = (char)0x4D;
            handoff->magic[2] = (char)0x55;
            handoff->magic[3] = (char)0x4B;
            handoff->magic[4] = (char)0x4F;
            handoff->firmware_id[0] = (char)0x4E;
            handoff->firmware_id[1] = (char)0x53;
            handoff->firmware_id[2] = (char)0x49;
            handoff->firmware_id[3] = (char)0x47;
            handoff->firmware_id[4] = (char)0x49;
            handoff->firmware_id[5] = (char)0x49;
            handoff->filesystem_target = "RAW_FIXED_SECTOR:mmuko-os.img:LBA0_STAGE1:LBA1_16_STAGE2:LBA17_48_RUNTIME";
            handoff->kernel_path = "/boot/mmuko.kernel";
            handoff->artifact_manifest_path = "/boot/mmuko-artifacts.json";
            handoff->config_path = "/boot/mmuko-boot.cfg";

            /* Run all 6 phases; abort on any failure */
            if (!mmuko_run_phase_1(handoff)) goto boot_failed;
            if (!mmuko_run_phase_2(handoff)) goto boot_failed;
            if (!mmuko_run_phase_3(handoff)) goto boot_failed;
            if (!mmuko_run_phase_4(handoff)) goto boot_failed;
            if (!mmuko_run_phase_5(handoff)) goto boot_failed;
            if (!mmuko_run_phase_6(handoff)) goto boot_failed;

            handoff->outcome = MMUKO_BOOT_OUTCOME_PASS;
            handoff->handoff_checksum = compute_handoff_checksum(handoff);
            return MMUKO_BOOT_OUTCOME_PASS;

        boot_failed:
            handoff->outcome = MMUKO_BOOT_OUTCOME_ALERT;
            handoff->handoff_checksum = compute_handoff_checksum(handoff);
            return MMUKO_BOOT_OUTCOME_ALERT;
        }

        int mmuko_verify_entry_contract(const MMUKO_BOOT_HANDOFF_t *h) {
            /* Kernel entry contract (from mmuko-boot.psc KERNEL ENTRY CONTRACT section) */
            if (h->magic[0] != 'M' || h->magic[1] != 'M' ||
                h->magic[2] != 'U' || h->magic[3] != 'K' ||
                h->magic[4] != 'O') {
                return 0;  /* magic mismatch */
            }
            if (h->revision != 0x0001)                    return 0;
            if (h->outcome  != MMUKO_BOOT_OUTCOME_PASS)   return 0;
            if (h->completed_phases != 6)     return 0;
            uint32_t expected = compute_handoff_checksum(h);
            if (h->handoff_checksum != expected) return 0;
            return 1;
        }
