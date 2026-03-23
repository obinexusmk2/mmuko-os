; MMUKO stage-2 loader.
; Loads a protected-mode runtime image from fixed reserved sectors,
; validates its header, switches to protected mode, and jumps to the entrypoint.

BITS 16
ORG 0x0000

%define RUNTIME_START_LBA    17
%define RUNTIME_SECTOR_COUNT 32
%define RUNTIME_LOAD_SEGMENT 0x1000
%define RUNTIME_LOAD_ADDR    0x00010000
%define SECTORS_PER_TRACK    18
%define HEAD_COUNT           2
%define RUNTIME_MAGIC        0x4D4D4B52
%define RUNTIME_SIGNATURE    0x4E534947

start:
    cli
    xor ax, ax
    mov ds, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl
    mov si, msg_stage2
    call print_string

    mov ax, RUNTIME_LOAD_SEGMENT
    mov es, ax
    xor bx, bx
    mov cx, RUNTIME_START_LBA
    mov si, RUNTIME_SECTOR_COUNT
.load_runtime:
    push cx
    push bx
    call lba_to_chs
    mov ah, 0x02
    mov al, 0x01
    mov dl, [boot_drive]
    int 0x13
    jc load_error
    pop bx
    pop cx
    add bx, 512
    inc cx
    dec si
    jnz .load_runtime

    mov ax, RUNTIME_LOAD_SEGMENT
    mov ds, ax
    xor si, si

    mov eax, [si + 0]
    cmp eax, RUNTIME_MAGIC
    jne bad_header
    mov eax, [si + 4]
    cmp eax, RUNTIME_SIGNATURE
    jne bad_header
    mov eax, [si + 20]
    cmp eax, RUNTIME_LOAD_ADDR
    jne bad_header
    mov eax, [si + 12]
    cmp eax, (RUNTIME_SECTOR_COUNT * 512)
    ja bad_header
    mov eax, [si + 20]
    add eax, [si + 16]
    mov [protected_entry], eax

    cli
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

load_error:
    pop bx
    pop cx
    mov si, msg_load_error
    call print_string
    jmp halt

bad_header:
    mov si, msg_bad_header
    call print_string
    jmp halt

halt:
    cli
.hang:
    hlt
    jmp .hang

print_char:
    mov ah, 0x0E
    xor bh, bh
    mov bl, 0x0A
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

lba_to_chs:
    mov ax, cx
    xor dx, dx
    div word [sectors_per_cylinder]
    push ax
    mov ax, dx
    xor dx, dx
    div word [sectors_per_track]
    mov dh, al
    mov cl, dl
    inc cl
    pop ax
    mov ch, al
    ret

boot_drive db 0
msg_stage2 db 'MMUKO stage2', 13, 10, 0
msg_load_error db 'Runtime load failed', 13, 10, 0
msg_bad_header db 'Runtime header invalid', 13, 10, 0
sectors_per_track dw SECTORS_PER_TRACK
sectors_per_cylinder dw SECTORS_PER_TRACK * HEAD_COUNT
protected_entry dd 0

align 8
gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

BITS 32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x0009FC00
    mov eax, [protected_entry]
    jmp eax
