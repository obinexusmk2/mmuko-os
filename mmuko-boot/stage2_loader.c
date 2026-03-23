#include "mmuko_som.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t crc32_update(uint32_t crc, const uint8_t *buf, size_t len) {
    crc = ~crc;
    for (size_t i = 0; i < len; ++i) {
        crc ^= buf[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (-(int32_t)(crc & 1u)));
        }
    }
    return ~crc;
}

static const char *payload_name(uint16_t format) {
    switch (format) {
        case MMUKO_SOM_PAYLOAD_ELF_SO: return "ELF shared object";
        case MMUKO_SOM_PAYLOAD_PE_DLL: return "PE/COFF DLL";
        case MMUKO_SOM_PAYLOAD_RAW_BIN: return "raw binary";
        default: return "unknown";
    }
}

static int validate_header(const MMUKO_SomHeader *header, long file_size) {
    if (memcmp(header->magic, MMUKO_SOM_MAGIC, 4) != 0) {
        fprintf(stderr, "stage2: bad magic, expected MSOM\n");
        return 1;
    }
    if (header->header_size != MMUKO_SOM_HEADER_SIZE || header->payload_offset < header->header_size) {
        fprintf(stderr, "stage2: unsupported header size (%u) or payload offset (%u)\n",
                header->header_size, header->payload_offset);
        return 1;
    }
    if ((uint64_t) header->payload_offset + header->payload_size != header->total_size ||
        header->total_size != (uint32_t) file_size) {
        fprintf(stderr, "stage2: size mismatch (header=%u, file=%ld)\n", header->total_size, file_size);
        return 1;
    }
    if (header->required_alignment == 0u || (header->required_alignment & (header->required_alignment - 1u)) != 0u) {
        fprintf(stderr, "stage2: invalid alignment %u\n", header->required_alignment);
        return 1;
    }
    if (header->reserved0 != 0u || header->reserved1 != 0u) {
        fprintf(stderr, "stage2: reserved fields must be zero\n");
        return 1;
    }
    if (memcmp(header->abi_tag, MMUKO_SOM_ABI_TAG, sizeof(header->abi_tag)) != 0) {
        fprintf(stderr, "stage2: unsupported ABI tag\n");
        return 1;
    }
    if (header->payload_format != MMUKO_SOM_PAYLOAD_ELF_SO &&
        header->payload_format != MMUKO_SOM_PAYLOAD_PE_DLL &&
        header->payload_format != MMUKO_SOM_PAYLOAD_RAW_BIN) {
        fprintf(stderr, "stage2: unsupported payload format %u\n", header->payload_format);
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <artifact.som>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "stage2: cannot open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "stage2: cannot seek %s\n", argv[1]);
        fclose(fp);
        return 1;
    }
    long file_size = ftell(fp);
    rewind(fp);

    MMUKO_SomHeader header;
    if (fread(&header, 1, sizeof(header), fp) != sizeof(header)) {
        fprintf(stderr, "stage2: failed to read header from %s\n", argv[1]);
        fclose(fp);
        return 1;
    }

    if (validate_header(&header, file_size) != 0) {
        fclose(fp);
        return 1;
    }

    uint8_t *payload = (uint8_t *) malloc(header.payload_size ? header.payload_size : 1u);
    if (!payload) {
        fprintf(stderr, "stage2: out of memory allocating %u bytes\n", header.payload_size);
        fclose(fp);
        return 1;
    }

    if (fseek(fp, (long) header.payload_offset, SEEK_SET) != 0 ||
        (header.payload_size && fread(payload, 1, header.payload_size, fp) != header.payload_size)) {
        fprintf(stderr, "stage2: failed to read payload from %s\n", argv[1]);
        free(payload);
        fclose(fp);
        return 1;
    }
    fclose(fp);

    uint32_t crc = crc32_update(0u, payload, header.payload_size);
    if (crc != header.checksum_crc32) {
        fprintf(stderr, "stage2: crc32 mismatch (calc=0x%08X header=0x%08X)\n", crc, header.checksum_crc32);
        free(payload);
        return 1;
    }

    printf("stage2: validated %s\n", argv[1]);
    printf("  format    : %s\n", payload_name(header.payload_format));
    printf("  bytes     : %u payload / %u total\n", header.payload_size, header.total_size);
    printf("  align     : %u\n", header.required_alignment);
    printf("  entry kind: %u\n", header.entry_kind);
    printf("  load addr : 0x%" PRIX64 "\n", header.preferred_load_addr);
    printf("  crc32     : 0x%08X\n", header.checksum_crc32);
    printf("  loader expectation: map payload on a %u-byte boundary, then dispatch according to payload_format.\n",
           header.required_alignment);

    free(payload);
    return 0;
}
