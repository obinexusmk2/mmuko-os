BITS 16
ORG 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, partition_table
    mov cx, 4
.find_active:
    cmp byte [si], 0x80
    je .load_vbr
    add si, 16
    loop .find_active

    mov si, partition_table

.load_vbr:
    mov eax, dword [si + 8]
    mov dword [dap_lba], eax
    mov dword [dap_lba + 4], 0
    mov word [dap_offset], 0x7C00
    mov word [dap_segment], 0x0000
    mov word [dap_count], 1

    mov dl, [boot_drive]
    mov si, disk_address_packet
    mov ah, 0x42
    int 0x13
    jc disk_error

    jmp 0x0000:0x7C00

disk_error:
    mov si, msg_error
    call print_string
.hang:
    hlt
    jmp .hang

print_string:
    pusha
.next:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0C
    int 0x10
    jmp .next
.done:
    popa
    ret

boot_drive db 0
msg_error db 'MBR read error', 0

disk_address_packet:
    db 0x10
    db 0x00

dap_count:
    dw 0

dap_offset:
    dw 0

dap_segment:
    dw 0

dap_lba:
    dq 0

times 446-($-$$) db 0
partition_table:
    times 64 db 0

dw 0xAA55
