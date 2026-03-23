; boot.asm - MMUKO-OS minimal stage-1 BIOS loader
; Loads the stage-2 loader from fixed reserved sectors and transfers control.
; Build: nasm -f bin boot.asm -o build/boot.bin

BITS 16
ORG 0x7C00

%define STAGE2_SEGMENT      0x0800
%define STAGE2_OFFSET       0x0000
%define STAGE2_START_LBA    1
%define STAGE2_SECTOR_COUNT 16
%define SECTORS_PER_TRACK   18
%define HEAD_COUNT          2

jmp short start
nop

boot_drive db 0
status_msg db 'MMUKO stage1', 13, 10, 0
error_msg  db 'Stage2 load failed', 13, 10, 0
sectors_per_track    dw SECTORS_PER_TRACK
sectors_per_cylinder dw SECTORS_PER_TRACK * HEAD_COUNT

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, status_msg
    call print_string

    mov ax, STAGE2_SEGMENT
    mov es, ax
    xor bx, bx
    mov cx, STAGE2_START_LBA
    mov si, STAGE2_SECTOR_COUNT
.load_loop:
    push cx
    push bx
    call lba_to_chs
    mov ah, 0x02
    mov al, 0x01
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    pop bx
    pop cx
    add bx, 512
    inc cx
    dec si
    jnz .load_loop

    mov dl, [boot_drive]
    jmp STAGE2_SEGMENT:STAGE2_OFFSET

disk_error:
    pop bx
    pop cx
    mov si, error_msg
    call print_string
.hang:
    hlt
    jmp .hang

; Input: CX = LBA.
; Output for int13h AH=02: CH=cylinder, CL=sector, DH=head.
lba_to_chs:
    mov ax, cx
    xor dx, dx
    div word [sectors_per_cylinder]
    push ax                 ; cylinder
    mov ax, dx              ; remainder within cylinder
    xor dx, dx
    div word [sectors_per_track]
    mov dh, al              ; head
    mov cl, dl
    inc cl                  ; sector is 1-based
    pop ax
    mov ch, al
    ret

print_char:
    mov ah, 0x0E
    xor bh, bh
    mov bl, 0x07
    int 0x10
    ret

print_string:
.next:
    lodsb
    test al, al
    jz .done
    call print_char
    jmp .next
.done:
    ret

times 510-($-$$) db 0
dw 0xAA55
