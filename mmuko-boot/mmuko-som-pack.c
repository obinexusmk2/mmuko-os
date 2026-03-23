#include "mmuko_som.h"

#include <errno.h>
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

static long file_size(FILE *fp) {
    if (fseek(fp, 0, SEEK_END) != 0) {
        return -1;
    }
    long size = ftell(fp);
    if (size < 0) {
        return -1;
    }
    rewind(fp);
    return size;
}

static uint16_t parse_payload_format(const char *arg) {
    if (strcmp(arg, "elf-so") == 0) return MMUKO_SOM_PAYLOAD_ELF_SO;
    if (strcmp(arg, "pe-dll") == 0) return MMUKO_SOM_PAYLOAD_PE_DLL;
    if (strcmp(arg, "raw-bin") == 0) return MMUKO_SOM_PAYLOAD_RAW_BIN;
    return 0;
}

static const char *payload_name(uint16_t format) {
    switch (format) {
        case MMUKO_SOM_PAYLOAD_ELF_SO: return "ELF shared object";
        case MMUKO_SOM_PAYLOAD_PE_DLL: return "PE/COFF DLL";
        case MMUKO_SOM_PAYLOAD_RAW_BIN: return "raw binary";
        default: return "unknown";
    }
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <input> <output.som> <elf-so|pe-dll|raw-bin>\n", argv[0]);
        return 1;
    }

    uint16_t payload_format = parse_payload_format(argv[3]);
    if (payload_format == 0) {
        fprintf(stderr, "error: unsupported payload format '%s'\n", argv[3]);
        return 1;
    }

    FILE *in = fopen(argv[1], "rb");
    if (!in) {
        fprintf(stderr, "error: cannot open %s: %s\n", argv[1], strerror(errno));
        return 1;
    }

    long payload_size_long = file_size(in);
    if (payload_size_long < 0) {
        fprintf(stderr, "error: cannot determine size of %s\n", argv[1]);
        fclose(in);
        return 1;
    }

    uint32_t payload_size = (uint32_t) payload_size_long;
    uint8_t *payload = (uint8_t *) malloc(payload_size ? payload_size : 1u);
    if (!payload) {
        fprintf(stderr, "error: out of memory allocating %u bytes\n", payload_size);
        fclose(in);
        return 1;
    }

    if (payload_size && fread(payload, 1, payload_size, in) != payload_size) {
        fprintf(stderr, "error: short read from %s\n", argv[1]);
        free(payload);
        fclose(in);
        return 1;
    }
    fclose(in);

    MMUKO_SomHeader header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, MMUKO_SOM_MAGIC, 4);
    header.version_major = MMUKO_SOM_VERSION_MAJOR;
    header.version_minor = MMUKO_SOM_VERSION_MINOR;
    header.header_size = MMUKO_SOM_HEADER_SIZE;
    header.total_size = MMUKO_SOM_HEADER_SIZE + payload_size;
    header.payload_offset = MMUKO_SOM_HEADER_SIZE;
    header.payload_size = payload_size;
    header.flags = MMUKO_SOM_FLAG_NONE;
    header.target_machine = MMUKO_SOM_MACHINE_X86_64;
    header.payload_format = payload_format;
    header.entry_kind = MMUKO_SOM_ENTRY_DLOPEN;
    header.preferred_load_addr = 0;
    header.required_alignment = 4096u;
    header.checksum_crc32 = crc32_update(0u, payload, payload_size);
    memcpy(header.abi_tag, MMUKO_SOM_ABI_TAG, sizeof(header.abi_tag));

    FILE *out = fopen(argv[2], "wb");
    if (!out) {
        fprintf(stderr, "error: cannot open %s: %s\n", argv[2], strerror(errno));
        free(payload);
        return 1;
    }

    if (fwrite(&header, 1, sizeof(header), out) != sizeof(header) ||
        (payload_size && fwrite(payload, 1, payload_size, out) != payload_size)) {
        fprintf(stderr, "error: failed to write %s\n", argv[2]);
        fclose(out);
        free(payload);
        return 1;
    }

    fclose(out);
    free(payload);

    printf("packed %s -> %s\n", argv[1], argv[2]);
    printf("  payload: %s (%u bytes)\n", payload_name(payload_format), payload_size);
    printf("  header : %u bytes\n", header.header_size);
    printf("  crc32  : 0x%08X\n", header.checksum_crc32);
    return 0;
}
