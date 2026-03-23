#ifndef MMUKO_BOOT_IMAGE_LAYOUT_H
#define MMUKO_BOOT_IMAGE_LAYOUT_H

#include <stdint.h>

#define MMUKO_STAGE2_START_LBA      1u
#define MMUKO_STAGE2_SECTOR_COUNT   16u
#define MMUKO_RUNTIME_START_LBA     (MMUKO_STAGE2_START_LBA + MMUKO_STAGE2_SECTOR_COUNT)
#define MMUKO_RUNTIME_SECTOR_COUNT  32u
#define MMUKO_TOTAL_IMAGE_SECTORS   2880u
#define MMUKO_SECTOR_SIZE           512u

#define MMUKO_RUNTIME_LOAD_SEGMENT  0x1000u
#define MMUKO_RUNTIME_LOAD_OFFSET   0x0000u
#define MMUKO_RUNTIME_LOAD_ADDRESS  0x00010000u

#define MMUKO_STAGE2_LOAD_SEGMENT   0x0800u
#define MMUKO_STAGE2_LOAD_OFFSET    0x0000u
#define MMUKO_STAGE2_LOAD_ADDRESS   0x00008000u

#define MMUKO_RUNTIME_MAGIC         0x4D4D4B52u /* 'MMKR' */
#define MMUKO_RUNTIME_SIGNATURE     0x4E534947u /* 'NSIG' */
#define MMUKO_RUNTIME_VERSION       1u

typedef struct __attribute__((packed)) mmuko_runtime_header {
    uint32_t magic;
    uint32_t signature;
    uint16_t version;
    uint16_t header_size;
    uint32_t image_size;
    uint32_t entry_offset;
    uint32_t load_address;
    uint32_t reserved;
} mmuko_runtime_header_t;

#endif
