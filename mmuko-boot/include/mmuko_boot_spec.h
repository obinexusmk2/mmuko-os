#ifndef MMUKO_BOOT_SPEC_H
#define MMUKO_BOOT_SPEC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MMUKO_BOOT_MAGIC "MMKO"
#define MMUKO_BOOT_REVISION 0x0001
#define MMUKO_BOOT_FILESYSTEM_TARGET "FAT12:mmuko-os.img"
#define MMUKO_BOOT_KERNEL_PATH "/boot/mmuko.kernel"
#define MMUKO_BOOT_FALLBACK_PATH "/boot/mmuko-runtime.bin"
#define MMUKO_BOOT_CONFIG_PATH "/boot/mmuko-boot.cfg"
#define MMUKO_BOOT_MANIFEST_PATH "/boot/mmuko-artifacts.json"
#define MMUKO_BOOT_SIGNATURE 0xAA55
#define MMUKO_BOOT_SECTOR_BYTES 512
#define MMUKO_BOOT_PHASE_COUNT 6

typedef enum {
    MMUKO_BOOT_OUTCOME_PASS = 0xAA,
    MMUKO_BOOT_OUTCOME_HOLD = 0xBB,
    MMUKO_BOOT_OUTCOME_ALERT = 0xCC
} MMUKO_BOOT_OUTCOME;

typedef enum {
    PHASE_NEED_STATE_INIT = 1, /* Need-state initialization */
    PHASE_SAFETY_SCAN = 2, /* Safety scan */
    PHASE_IDENTITY_CALIBRATION = 3, /* Identity calibration */
    PHASE_GOVERNANCE_CHECK = 4, /* Governance check */
    PHASE_INTERNAL_PROBE = 5, /* Internal probe */
    PHASE_INTEGRITY_VERIFICATION = 6, /* Integrity verification */
} MMUKO_BOOT_PHASE;

typedef struct {
    char magic[4];
    uint16_t revision;
    uint8_t outcome;
    uint8_t completed_phases;
    uint8_t phase_count;
    uint8_t last_completed_phase;
    char filesystem_target[32];
    char kernel_path[64];
    char artifact_manifest_path[64];
    char config_path[64];
    uint16_t kernel_entry_segment;
    uint16_t kernel_entry_offset;
    uint32_t validation_flags;
    uint32_t handoff_checksum;
} MMUKO_BOOT_HANDOFF;

uint32_t mmuko_boot_checksum(const MMUKO_BOOT_HANDOFF *handoff);
bool mmuko_boot_validate(const MMUKO_BOOT_HANDOFF *handoff);

#endif
