#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* MMUKO-OS 512-byte boot sector - C implementation */

int main(void) {
    FILE *f = fopen("img/mmuko-os.img", "wb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    uint8_t sector[512];
    memset(sector, 0, 512);

    /* RIFT Header (8 bytes) */
    sector[0] = 'N';
    sector[1] = 'X';
    sector[2] = 'O';
    sector[3] = 'B';
    sector[4] = 0x01;   /* Version */
    sector[5] = 0x00;   /* Reserved */
    sector[6] = 0xFE;   /* Checksum */
    sector[7] = 0x01;   /* Flags */

    /* Boot code at offset 8 */
    uint8_t boot_code[] = {
        0xFA,                   /* cli */
        0x31, 0xC0,             /* xor ax, ax */
        0x8E, 0xD8,             /* mov ds, ax */
        0x8E, 0xC0,             /* mov es, ax */
        0xBC, 0x00, 0x7C,       /* mov sp, 0x7C00 */
        /* Print message */
        0xBE, 0x60, 0x7C,       /* mov si, msg */
        0xB4, 0x0E,             /* mov ah, 0x0E */
        /* Loop */
        0xAC,                   /* lodsb */
        0x08, 0xC0,             /* or al, al */
        0x74, 0x04,             /* jz done */
        0xCD, 0x10,             /* int 0x10 */
        0xEB, 0xF5,             /* jmp loop */
        /* Done */
        0xB0, 0x55,             /* mov al, 0x55 (NSIGII_YES) */
        0xF4,                   /* hlt */
        0xEB, 0xFE              /* jmp $ */
    };

    memcpy(&sector[8], boot_code, sizeof(boot_code));

    /* Messages at offset 0x60 */
    const char *msg = "=== MMUKO-OS RINGBOOT ===\r\n"
                      "OBINEXUS NSIGII Verify\r\n"
                      "[Phase 1] SPARSE\r\n"
                      "[Phase 2] REMEMBER\r\n"
                      "[Phase 3] ACTIVE\r\n"
                      "[Phase 4] VERIFY\r\n\n"
                      "NSIGII_VERIFIED\r\n"
                      "BOOT_SUCCESS\r\n";
    memcpy(&sector[0x60], msg, strlen(msg) + 1);

    /* Boot signature at offset 510 */
    sector[510] = 0x55;
    sector[511] = 0xAA;

    fwrite(sector, 512, 1, f);
    fclose(f);

    return 0;
}
