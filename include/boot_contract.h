#ifndef MMUKO_BOOT_CONTRACT_H
#define MMUKO_BOOT_CONTRACT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MMUKO_BOOT_CONTRACT_MAGIC 0x4D42u /* 'MB' */
#define MMUKO_BOOT_CONTRACT_VERSION 0x0001u
#define MMUKO_BOOT_CONTRACT_ADDR 0x0500u
#define MMUKO_BOOT_KEYBOARD_MAX 32u

enum {
    MMUKO_BOOT_FLAG_KEYBOARD_REQUIRED = 1u << 0,
    MMUKO_BOOT_FLAG_KEYBOARD_PRESENT  = 1u << 1,
    MMUKO_BOOT_FLAG_NATIVE_C_READY    = 1u << 2,
    MMUKO_BOOT_FLAG_STAGE2_MODE       = 1u << 3,
    MMUKO_BOOT_FLAG_KERNEL_MODE       = 1u << 4,
    MMUKO_BOOT_FLAG_NSIGII_READY      = 1u << 5
};

typedef enum {
    MMUKO_MEMBRANE_PASS  = 0xAA,
    MMUKO_MEMBRANE_HOLD  = 0xBB,
    MMUKO_MEMBRANE_ALERT = 0xCC
} mmuko_membrane_outcome_t;

typedef enum {
    MMUKO_TRANSFER_RESET          = 0,
    MMUKO_TRANSFER_STAGE1_READY   = 1,
    MMUKO_TRANSFER_KEYBOARD_DONE  = 2,
    MMUKO_TRANSFER_STAGE2_READY   = 3,
    MMUKO_TRANSFER_NATIVE_C_ENTRY = 4,
    MMUKO_TRANSFER_KERNEL_ENTRY   = 5
} mmuko_transfer_state_t;

typedef enum {
    MMUKO_RING_0_KERNEL  = 0,  /* Boot chain + runtime (EM: magnetic/compile-time) */
    MMUKO_RING_1_DRIVER  = 1,  /* BIOS firmware interface (SpinPair, Mosaic, RTC) */
    MMUKO_RING_2_SERVICE = 2,  /* Membrane + MPDA + discriminant layer */
    MMUKO_RING_3_USER    = 3   /* Python/Cython UI + applications */
} mmuko_ring_level_t;

typedef struct {
    uint8_t length;
    uint8_t capacity;
    uint8_t last_scan_code;
    uint8_t flags;
    char bytes[MMUKO_BOOT_KEYBOARD_MAX];
} mmuko_keyboard_buffer_t;

typedef struct {
    uint16_t magic;
    uint16_t version;
    uint16_t total_size;
    uint16_t boot_flags;
    uint8_t transfer_state;
    uint8_t membrane_outcome;
    uint8_t membrane_phase;
    uint8_t ring_level;          /* mmuko_ring_level_t — current protection ring */
    uint16_t native_entry_offset;
    uint16_t native_entry_segment;
    mmuko_keyboard_buffer_t keyboard;
} mmuko_boot_contract_t;

enum {
    MMUKO_CONTRACT_OFF_MAGIC                = 0x00,
    MMUKO_CONTRACT_OFF_VERSION              = 0x02,
    MMUKO_CONTRACT_OFF_TOTAL_SIZE           = 0x04,
    MMUKO_CONTRACT_OFF_BOOT_FLAGS           = 0x06,
    MMUKO_CONTRACT_OFF_TRANSFER_STATE       = 0x08,
    MMUKO_CONTRACT_OFF_MEMBRANE_OUTCOME     = 0x09,
    MMUKO_CONTRACT_OFF_MEMBRANE_PHASE       = 0x0A,
    MMUKO_CONTRACT_OFF_RING_LEVEL           = 0x0B,
    MMUKO_CONTRACT_OFF_NATIVE_ENTRY_OFFSET  = 0x0C,
    MMUKO_CONTRACT_OFF_NATIVE_ENTRY_SEGMENT = 0x0E,
    MMUKO_CONTRACT_OFF_KEYBOARD_LENGTH      = 0x10,
    MMUKO_CONTRACT_OFF_KEYBOARD_CAPACITY    = 0x11,
    MMUKO_CONTRACT_OFF_KEYBOARD_SCAN_CODE   = 0x12,
    MMUKO_CONTRACT_OFF_KEYBOARD_FLAGS       = 0x13,
    MMUKO_CONTRACT_OFF_KEYBOARD_BYTES       = 0x14
};

_Static_assert(sizeof(mmuko_boot_contract_t) == 52, "boot contract layout drifted");

static inline void mmuko_boot_contract_reset(mmuko_boot_contract_t *contract)
{
    if (!contract) {
        return;
    }

    *contract = (mmuko_boot_contract_t){
        .magic = MMUKO_BOOT_CONTRACT_MAGIC,
        .version = MMUKO_BOOT_CONTRACT_VERSION,
        .total_size = (uint16_t)sizeof(mmuko_boot_contract_t),
        .boot_flags = 0,
        .transfer_state = MMUKO_TRANSFER_RESET,
        .membrane_outcome = MMUKO_MEMBRANE_HOLD,
        .membrane_phase = 0,
        .ring_level = MMUKO_RING_0_KERNEL,
        .native_entry_offset = 0,
        .native_entry_segment = 0,
        .keyboard = {
            .length = 0,
            .capacity = MMUKO_BOOT_KEYBOARD_MAX,
            .last_scan_code = 0,
            .flags = 0,
            .bytes = {0}
        }
    };
}

static inline bool mmuko_keyboard_buffer_append(mmuko_keyboard_buffer_t *keyboard,
                                                char ascii,
                                                uint8_t scan_code)
{
    if (!keyboard || keyboard->length >= keyboard->capacity) {
        return false;
    }

    keyboard->bytes[keyboard->length++] = ascii;
    keyboard->last_scan_code = scan_code;
    keyboard->flags |= 0x01u;
    return true;
}

static inline size_t mmuko_keyboard_buffer_copy_text(mmuko_keyboard_buffer_t *keyboard,
                                                     const char *text)
{
    size_t copied = 0;

    if (!keyboard || !text) {
        return 0;
    }

    while (text[copied] != '\0' && copied < keyboard->capacity) {
        keyboard->bytes[copied] = text[copied];
        copied++;
    }

    keyboard->length = (uint8_t)copied;
    keyboard->flags |= copied > 0 ? 0x01u : 0u;
    return copied;
}

#endif /* MMUKO_BOOT_CONTRACT_H */
