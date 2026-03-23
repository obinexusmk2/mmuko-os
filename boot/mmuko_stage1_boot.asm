; -----------------------------------------------------------------------------
; Generated file. Do not edit by hand.
; Authoritative input: MMUKO-OS.txt
; Primary pseudocode: mmuko-boot/pseudocode/mmuko-boot.psc
; Supporting pseudocode count: 14
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

    mov si, boot_banner
    call print_string
    mov si, boot_stage1
    call print_string
    mov si, boot_stage2
    call print_string
    mov si, boot_ready
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

boot_banner db 13,10, "MMUKO-OS generated stage-1", 13,10, 0
boot_stage1 db "Spec: MMUKO-OS.txt", 13,10, 0
boot_stage2 db "Stage-2 handoff: kernel/mmuko_stage2_loader.c", 13,10, 0
boot_ready  db "BOOTSTRAP_READY", 13,10, 0

times 510-($-$$) db 0
dw 0xAA55
