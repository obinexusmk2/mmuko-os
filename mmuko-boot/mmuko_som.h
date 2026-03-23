#ifndef MMUKO_SOM_H
#define MMUKO_SOM_H

#include <stdint.h>

#define MMUKO_SOM_MAGIC "MSOM"
#define MMUKO_SOM_HEADER_SIZE 64u
#define MMUKO_SOM_ABI_TAG "NSIGII\0\0"
#define MMUKO_SOM_VERSION_MAJOR 1u
#define MMUKO_SOM_VERSION_MINOR 0u

#define MMUKO_SOM_FLAG_NONE        0x00000000u
#define MMUKO_SOM_FLAG_COMPRESSED  0x00000001u
#define MMUKO_SOM_FLAG_SIGNED      0x00000002u

#define MMUKO_SOM_MACHINE_X86_64   0x003Eu
#define MMUKO_SOM_MACHINE_I386     0x0003u

#define MMUKO_SOM_PAYLOAD_ELF_SO   0x0001u
#define MMUKO_SOM_PAYLOAD_PE_DLL   0x0002u
#define MMUKO_SOM_PAYLOAD_RAW_BIN  0x0003u

#define MMUKO_SOM_ENTRY_DLOPEN     0x0001u
#define MMUKO_SOM_ENTRY_BOOTSTRAP  0x0002u
#define MMUKO_SOM_ENTRY_STAGE2     0x0003u

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

typedef struct
#if defined(__GNUC__) || defined(__clang__)
__attribute__((packed))
#endif
MMUKO_SomHeader {
    uint8_t  magic[4];              /* 0x00: ASCII "MSOM"                        */
    uint16_t version_major;         /* 0x04: format major version                 */
    uint16_t version_minor;         /* 0x06: format minor version                 */
    uint32_t header_size;           /* 0x08: bytes from start of file to payload  */
    uint32_t total_size;            /* 0x0C: full container size in bytes         */
    uint32_t payload_offset;        /* 0x10: first payload byte                   */
    uint32_t payload_size;          /* 0x14: payload bytes after header           */
    uint32_t flags;                 /* 0x18: MMUKO_SOM_FLAG_* bitmask             */
    uint16_t target_machine;        /* 0x1C: ISA / machine identifier             */
    uint16_t payload_format;        /* 0x1E: ELF_SO / PE_DLL / RAW_BIN            */
    uint16_t entry_kind;            /* 0x20: how the stage-2 loader dispatches    */
    uint16_t reserved0;             /* 0x22: reserved, must be zero               */
    uint64_t preferred_load_addr;   /* 0x24: optional load address hint           */
    uint32_t required_alignment;    /* 0x2C: payload alignment requirement        */
    uint32_t checksum_crc32;        /* 0x30: CRC32 of payload only                */
    uint8_t  abi_tag[8];            /* 0x34: ABI tag, currently "NSIGII\0\0"     */
    uint32_t reserved1;             /* 0x3C: reserved, must be zero               */
} MMUKO_SomHeader;

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

_Static_assert(sizeof(MMUKO_SomHeader) == MMUKO_SOM_HEADER_SIZE,
               "MMUKO_SomHeader must remain 64 bytes");

#endif /* MMUKO_SOM_H */
