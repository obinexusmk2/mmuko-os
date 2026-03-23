#ifndef MMUKO_BOOT_CONTRACT_H
#define MMUKO_BOOT_CONTRACT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MMUKO_BOOT_CONTRACT_MAGIC      0x4D425443u /* 'MBTC' */
#define MMUKO_BOOT_CONTRACT_SIGNATURE  0x4E534947u /* 'NSIG' */
#define MMUKO_BOOT_CONTRACT_VERSION    0x0001u

#define MMUKO_BOOT_CONTRACT_SEGMENT    0x0000u
#define MMUKO_BOOT_CONTRACT_OFFSET     0x0600u
#define MMUKO_BOOT_CONTRACT_PHYS_ADDR  ((uint32_t)MMUKO_BOOT_CONTRACT_OFFSET)

enum {
    MMUKO_BOOT_STAGE1_ID = 1,
    MMUKO_BOOT_STAGE2_ID = 2,
    MMUKO_BOOT_RUNTIME_ID = 3
};

typedef enum {
    MMUKO_BOOT_STATUS_HOLD  = 0,
    MMUKO_BOOT_STATUS_PASS  = 1,
    MMUKO_BOOT_STATUS_ALERT = 2,
    MMUKO_BOOT_STATUS_FAULT = 3
} mmuko_boot_status;

typedef enum {
    MMUKO_BOOT_PHASE_PREPARE   = 0,
    MMUKO_BOOT_PHASE_N         = 1,
    MMUKO_BOOT_PHASE_S         = 2,
    MMUKO_BOOT_PHASE_I_IDENT   = 3,
    MMUKO_BOOT_PHASE_G         = 4,
    MMUKO_BOOT_PHASE_I_PROBE   = 5,
    MMUKO_BOOT_PHASE_I_INTEG   = 6,
    MMUKO_BOOT_PHASE_HANDOFF   = 7,
    MMUKO_BOOT_PHASE_COMPLETE  = 8
} mmuko_boot_phase_id;

typedef struct __attribute__((packed)) {
    uint32_t version;
    uint16_t stage_id;
    uint16_t current_phase;
    uint16_t last_phase;
    uint16_t flags;
} mmuko_boot_stage_metadata;

typedef struct __attribute__((packed)) {
    uint8_t tier1;
    uint8_t tier2;
    uint8_t tripwire;
    uint8_t reserved0;
    uint16_t threshold;
    uint16_t discriminant_hint;
} mmuko_boot_need_state;

typedef struct __attribute__((packed)) {
    uint32_t memory_base;
    uint16_t memory_bytes;
    uint16_t entry_offset;
    uint16_t payload_bytes;
    uint16_t reserved1;
} mmuko_boot_memory_layout;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t signature;
    mmuko_boot_stage_metadata stage;
    mmuko_boot_need_state need_state;
    mmuko_boot_memory_layout layout;
    uint32_t reserved[2];
    uint8_t status;
    uint8_t status_reason;
    uint8_t checksum;
    uint8_t structure_bytes;
} mmuko_boot_contract_record;

static inline uint8_t mmuko_boot_contract_checksum(const mmuko_boot_contract_record *record)
{
    const uint8_t *bytes = (const uint8_t *)record;
    uint8_t checksum = 0;

    if (!record) {
        return 0;
    }

    for (uint32_t i = 0; i + 1U < (uint32_t)sizeof(*record); ++i) {
        checksum = (uint8_t)(checksum + bytes[i]);
    }

    return checksum;
}

static inline void mmuko_boot_contract_finalize(mmuko_boot_contract_record *record)
{
    if (!record) {
        return;
    }

    record->structure_bytes = (uint8_t)sizeof(*record);
    record->checksum = mmuko_boot_contract_checksum(record);
}

static inline int mmuko_boot_contract_is_valid(const mmuko_boot_contract_record *record)
{
    if (!record) {
        return 0;
    }

    if (record->magic != MMUKO_BOOT_CONTRACT_MAGIC ||
        record->signature != MMUKO_BOOT_CONTRACT_SIGNATURE ||
        record->structure_bytes != sizeof(*record)) {
        return 0;
    }

    return record->checksum == mmuko_boot_contract_checksum(record);
}

#ifdef __cplusplus
}
#endif

#endif /* MMUKO_BOOT_CONTRACT_H */
