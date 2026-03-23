; kernel/runtime.asm - MMUKO-OS firmware runtime entry
; Reads the NSIGII contract/state prepared by stage-2 and serves as the
; first real runtime after BIOS handoff.

BITS 16
%include "boot/contract.inc"
ORG KERNEL_LOAD_OFFSET

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9800
    sti

    mov si, msg_runtime
    call print_string

    mov al, [CONTRACT_BASE + CONTRACT_BOOT_DRIVE_OFF]
    mov si, msg_drive
    call print_string
    call print_hex8
    call newline

    mov al, [CONTRACT_BASE + CONTRACT_MEMBRANE_OFF]
    mov si, msg_membrane
    call print_string
    call print_hex8
    call newline

    mov ax, [CONTRACT_BASE + CONTRACT_MEM_KB_OFF]
    mov si, msg_memory
    call print_string
    call print_hex16
    call newline

    mov si, msg_ready
    call print_string
.halt:
    hlt
    jmp .halt

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x000A
    int 0x10
    jmp print_string
.done:
    ret

newline:
    mov al, 13
    mov ah, 0x0E
    mov bx, 0x000A
    int 0x10
    mov al, 10
    int 0x10
    ret

print_hex16:
    push ax
    xchg al, ah
    call print_hex8
    pop ax
    call print_hex8
    ret

print_hex8:
    push ax
    mov ah, al
    shr al, 4
    call nibble
    mov al, ah
    and al, 0x0F
    call nibble
    pop ax
    ret

nibble:
    and al, 0x0F
    cmp al, 10
    jb .digit
    add al, 'A' - 10
    jmp .emit
.digit:
    add al, '0'
.emit:
    mov ah, 0x0E
    mov bx, 0x000A
    int 0x10
    ret

msg_runtime  db 13,10, "[runtime] firmware entry", 13,10, 0
msg_drive    db "boot drive dl=0x", 0
msg_membrane db "membrane=0x", 0
msg_memory   db "bios mem kb=0x", 0
msg_ready    db "runtime ready; handoff contract preserved", 13,10, 0
