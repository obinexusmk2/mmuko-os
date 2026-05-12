; -----------------------------------------------------------------------------
; Generated file. Do not edit by hand.
; Authoritative input: MMUKO-OS.txt
; Primary pseudocode: mmuko-boot/pseudocode/mmuko-boot.psc
; Supporting pseudocode count: 1
; Parsed ENUM types: MMUKO_BOOT_OUTCOME, MMUKO_BOOT_PHASE
; Parsed STRUCT types: MMUKO_BOOT_HANDOFF
; Boot contract: MMUKO magic, 6 phases, outcome PASS=0xAA
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

; Raw fixed-sector MMUKO boot layout. This reserved metadata is not a
; BIOS Parameter Block and intentionally carries no filesystem label.
mmuko_layout_magic  db "MMUKORAW"
mmuko_stage2_lba    dw 1
mmuko_stage2_count  dw 16
mmuko_runtime_lba   dw 17
mmuko_runtime_count dw 32
mmuko_reserved      times 8 db 0

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
