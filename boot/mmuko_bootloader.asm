; boot/mmuko_bootloader.asm
; MMUKO-OS Flat Binary Bootloader
; OBINexus Computing | Nnamdi Michael Okpala | 29 March 2026
;
; Build:  nasm -f bin boot/mmuko_bootloader.asm -o build/mmuko_bootloader.bin
; Output: 512 bytes, 0xAA55 signature at bytes 510-511
;
; Responsibilities:
;   1. Read BIOS RTC date+time (INT 0x1A) into physical 0x0500 (7 BCD bytes)
;   2. Load 4 sectors (2048 bytes) from disk sector 2 into 0x1000:0x0000
;   3. Jump to 0x1000:0x0000 (flat binary kernel entry)
;
; BIOS INT 0x1A protocol:
;   AH=4 → Date:  CF=error | CH=century(BCD) CL=year(BCD) DH=month(BCD) DL=day(BCD)
;   AH=2 → Time:  CF=error | CH=hours(BCD)   CL=min(BCD)  DH=sec(BCD)
;
; Memory layout after boot:
;   0x0500  7 bytes: century year month day hour min sec (all BCD)
;   0x1000:0x0000  2048 bytes: kernel binary
;
; Segment model: flat 16-bit real mode, DS=ES=SS=0x0000

BITS 16
ORG 0x7C00

DATETIME_BASE   equ 0x0500      ; physical address for BCD datetime (7 bytes)
KERNEL_SEGMENT  equ 0x1000      ; load kernel to linear 0x10000
KERNEL_OFFSET   equ 0x0000
KERNEL_SECTORS  equ 4           ; 4 × 512 = 2048 bytes

; -----------------------------------------------------------------------
; Entry point
; -----------------------------------------------------------------------
start:
    cli
    xor  ax, ax
    mov  ds, ax
    mov  es, ax
    mov  ss, ax
    mov  sp, 0x7C00
    sti

    mov  [boot_drive], dl       ; BIOS passes drive number in DL

    call read_rtc_datetime
    call load_kernel

    ; Hand off to kernel at 0x1000:0x0000
    jmp  KERNEL_SEGMENT:KERNEL_OFFSET

; -----------------------------------------------------------------------
; read_rtc_datetime
;   Reads BIOS RTC into DATETIME_BASE (7 bytes BCD).
;   On RTC failure (CF set) zeros the block gracefully.
; -----------------------------------------------------------------------
read_rtc_datetime:
    ; --- Read date (AH=4) -------------------------------------------
    mov  ah, 0x04
    int  0x1A
    jc   .rtc_fail              ; carry set = clock not running / error

    mov  [DATETIME_BASE + 0], ch   ; century  (e.g. 0x20 for 20xx)
    mov  [DATETIME_BASE + 1], cl   ; year     (e.g. 0x26 for 2026)
    mov  [DATETIME_BASE + 2], dh   ; month    (01-12)
    mov  [DATETIME_BASE + 3], dl   ; day      (01-31)

    ; --- Read time (AH=2) -------------------------------------------
    mov  ah, 0x02
    int  0x1A
    jc   .rtc_fail

    mov  [DATETIME_BASE + 4], ch   ; hours    (00-23)
    mov  [DATETIME_BASE + 5], cl   ; minutes  (00-59)
    mov  [DATETIME_BASE + 6], dh   ; seconds  (00-59)
    ret

.rtc_fail:
    ; Zero the 7-byte block so the kernel sees 00/00/00 00:00:00
    xor  al, al
    mov  di, DATETIME_BASE
    mov  cx, 7
    rep  stosb
    ret

; -----------------------------------------------------------------------
; load_kernel
;   Loads KERNEL_SECTORS sectors starting at CHS (0,0,2) into
;   KERNEL_SEGMENT:KERNEL_OFFSET using BIOS INT 0x13 AH=0x02.
; -----------------------------------------------------------------------
load_kernel:
    mov  ax, KERNEL_SEGMENT
    mov  es, ax
    mov  bx, KERNEL_OFFSET

    mov  ah, 0x02               ; BIOS: read sectors
    mov  al, KERNEL_SECTORS     ; number of sectors to read
    xor  ch, ch                 ; cylinder 0
    mov  cl, 2                  ; starting sector 2 (1-based, sectors 2–5)
    xor  dh, dh                 ; head 0
    mov  dl, [boot_drive]
    int  0x13
    jc   .disk_error
    ret

.disk_error:
    mov  si, msg_disk_err
    call print_str
.hang:
    hlt
    jmp  .hang

; -----------------------------------------------------------------------
; print_str — BIOS teletype: print null-terminated DS:SI string
; -----------------------------------------------------------------------
print_str:
    lodsb
    test al, al
    jz   .done
    mov  ah, 0x0E
    mov  bx, 0x0007
    int  0x10
    jmp  print_str
.done:
    ret

; -----------------------------------------------------------------------
; Data
; -----------------------------------------------------------------------
boot_drive    db 0
msg_disk_err  db "MMUKO-BOOT: disk read error", 13, 10, 0

; Pad to 512 bytes with boot signature
times 510-($-$$) db 0
dw 0xAA55
