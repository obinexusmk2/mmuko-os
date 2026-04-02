; -----------------------------------------------------------------------------
; Generated file. Do not edit by hand.
; Authoritative input: MMUKO-OS.txt
; Primary pseudocode: pseudocode/mmuko-boot.psc
; Supporting pseudocode count: 26
; Parsed ENUM types: MMUKO_BOOT_OUTCOME, MMUKO_BOOT_PHASE
; Parsed STRUCT types: MMUKO_BOOT_HANDOFF
; Boot contract: MMKO magic, 6 phases, outcome PASS=0xAA
; -----------------------------------------------------------------------------
; Key generated phases:
;   PHASE 0 - Vacuum Medium Initialization
;   PHASE 1 - Cubit Ring Initialization
;   PHASE 2 - Compass Alignment
;   PHASE 3 - Superposition Entanglement
;   PHASE 4 - Middle Alignment
;   PHASE 5 - Nonlinear Index Resolution
;   PHASE 6 - Rotation Verification

BITS 16
ORG  0x7C00

jmp short start
nop
db "MMUKOGEN"
dw 512
db 1
dw 1
db 2
dw 224
dw 2880
db 0xF0
dw 9
dw 18
dw 2
dd 0
dd 0
db 0
db 0
db 0x29
dd 0x4D4D554B
db "MMUKO-GEN  "
db "FAT12   "

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Save boot drive number
    mov [boot_drive], dl

    ; Print boot banner
    mov si, boot_banner
    call print_string

    ; Load stage-2 from disk (sectors 1..16) into 0x0000:0x8000
    mov ax, 0x0000
    mov es, ax
    mov bx, 0x8000          ; load address

load_stage2:
    mov ah, 0x02            ; BIOS read sectors
    mov al, 16              ; sector count
    mov ch, 0               ; cylinder 0
    mov cl, 2               ; sector 2 (1-based, sector 1 = boot)
    mov dh, 0               ; head 0
    mov dl, [boot_drive]
    int 0x13
    jc  disk_error

    mov si, boot_stage2_ok
    call print_string

    ; Jump to stage-2
    jmp 0x0000:0x8000

disk_error:
    mov si, boot_disk_err
    call print_string

halt_forever:
    hlt
    jmp halt_forever

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0F
    int 0x10
    jmp print_string
.done:
    ret

boot_drive   db 0
boot_banner  db 13,10, "MMUKO-OS stage-1", 13,10, 0
boot_stage2_ok db "Stage-2 loaded OK", 13,10, 0
boot_disk_err  db "Disk error - halting", 13,10, 0

times 510-($-$$) db 0
dw 0xAA55
