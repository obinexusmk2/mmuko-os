#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "image_layout.h"

static void read_exact(const char *path, uint8_t **data, size_t *size) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "mkimage: unable to open %s: %s\n", path, strerror(errno));
        exit(1);
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "mkimage: unable to seek %s\n", path);
        exit(1);
    }

    long length = ftell(fp);
    if (length < 0) {
        fprintf(stderr, "mkimage: unable to measure %s\n", path);
        exit(1);
    }
    rewind(fp);

    *size = (size_t)length;
    *data = malloc(*size);
    if (!*data) {
        fprintf(stderr, "mkimage: out of memory while reading %s\n", path);
        exit(1);
    }

    if (fread(*data, 1, *size, fp) != *size) {
        fprintf(stderr, "mkimage: unable to read %s\n", path);
        exit(1);
    }
    fclose(fp);
}

static void ensure_limit(const char *label, size_t size, uint32_t sectors) {
    size_t limit = (size_t)sectors * MMUKO_SECTOR_SIZE;
    if (size > limit) {
        fprintf(stderr, "mkimage: %s is %zu bytes, exceeds %zu-byte allocation\n", label, size, limit);
        exit(1);
    }
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "usage: %s <boot.bin> <stage2.bin> <runtime.bin> <image>\n", argv[0]);
        return 1;
    }

    uint8_t *boot = NULL;
    uint8_t *stage2 = NULL;
    uint8_t *runtime = NULL;
    size_t boot_size = 0;
    size_t stage2_size = 0;
    size_t runtime_size = 0;

    read_exact(argv[1], &boot, &boot_size);
    read_exact(argv[2], &stage2, &stage2_size);
    read_exact(argv[3], &runtime, &runtime_size);

    if (boot_size != MMUKO_SECTOR_SIZE) {
        fprintf(stderr, "mkimage: boot sector must be exactly %u bytes (got %zu)\n", MMUKO_SECTOR_SIZE, boot_size);
        return 1;
    }
    ensure_limit("stage2", stage2_size, MMUKO_STAGE2_SECTOR_COUNT);
    ensure_limit("runtime", runtime_size, MMUKO_RUNTIME_SECTOR_COUNT);

    const mmuko_runtime_header_t *header = (const mmuko_runtime_header_t *)runtime;
    if (runtime_size < sizeof(*header) ||
        header->magic != MMUKO_RUNTIME_MAGIC ||
        header->signature != MMUKO_RUNTIME_SIGNATURE ||
        header->version != MMUKO_RUNTIME_VERSION ||
        header->load_address != MMUKO_RUNTIME_LOAD_ADDRESS ||
        header->image_size != runtime_size ||
        header->entry_offset < header->header_size ||
        header->entry_offset >= runtime_size) {
        fprintf(stderr, "mkimage: runtime header validation failed\n");
        return 1;
    }

    size_t image_size = (size_t)MMUKO_TOTAL_IMAGE_SECTORS * MMUKO_SECTOR_SIZE;
    uint8_t *image = calloc(1, image_size);
    if (!image) {
        fprintf(stderr, "mkimage: unable to allocate image buffer\n");
        return 1;
    }

    memcpy(image, boot, boot_size);
    memcpy(image + (size_t)MMUKO_STAGE2_START_LBA * MMUKO_SECTOR_SIZE, stage2, stage2_size);
    memcpy(image + (size_t)MMUKO_RUNTIME_START_LBA * MMUKO_SECTOR_SIZE, runtime, runtime_size);

    FILE *out = fopen(argv[4], "wb");
    if (!out) {
        fprintf(stderr, "mkimage: unable to create %s: %s\n", argv[4], strerror(errno));
        return 1;
    }
    if (fwrite(image, 1, image_size, out) != image_size) {
        fprintf(stderr, "mkimage: failed writing %s\n", argv[4]);
        return 1;
    }
    fclose(out);

    printf("[IMAGE] wrote %s (%zu bytes)\n", argv[4], image_size);
    printf("[IMAGE] stage1: LBA 0 (%zu bytes)\n", boot_size);
    printf("[IMAGE] stage2: LBA %u..%u (%zu bytes used of %u bytes)\n",
           MMUKO_STAGE2_START_LBA,
           MMUKO_STAGE2_START_LBA + MMUKO_STAGE2_SECTOR_COUNT - 1,
           stage2_size,
           MMUKO_STAGE2_SECTOR_COUNT * MMUKO_SECTOR_SIZE);
    printf("[IMAGE] runtime: LBA %u..%u (%zu bytes used of %u bytes)\n",
           MMUKO_RUNTIME_START_LBA,
           MMUKO_RUNTIME_START_LBA + MMUKO_RUNTIME_SECTOR_COUNT - 1,
           runtime_size,
           MMUKO_RUNTIME_SECTOR_COUNT * MMUKO_SECTOR_SIZE);

    free(boot);
    free(stage2);
    free(runtime);
    free(image);
    return 0;
}
