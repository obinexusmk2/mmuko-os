BITS 16
ORG 0x7C00

jmp short start
nop

oem_name                db 'MMUKOBIO'
bytes_per_sector        dw 512
sectors_per_cluster     db 4
reserved_sector_count   dw 16
fat_count               db 2
root_entry_count        dw 512
total_sectors_16        dw 0
media_descriptor        db 0xF8
sectors_per_fat_16      dw 0
sectors_per_track       dw 63
number_of_heads         dw 16
hidden_sectors          dd 0
total_sectors_32        dd 0
drive_number            db 0x80
reserved1               db 0
boot_signature          db 0x29
volume_id               dd 0x20260323
volume_label            db 'MMUKO BIOS '
filesystem_type         db 'FAT16   '
stage2_sector_count     dw 0
stage2_load_segment     dw 0x0800

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl

    mov si, msg_banner
    call print_string

    mov ax, [stage2_load_segment]
    mov es, ax
    xor bx, bx

    mov eax, [hidden_sectors]
    inc eax
    mov dword [dap_lba], eax
    mov dword [dap_lba + 4], 0
    mov ax, [stage2_sector_count]
    mov [dap_count], ax
    mov [dap_offset], bx
    mov ax, es
    mov [dap_segment], ax

    mov dl, [boot_drive]
    mov si, disk_address_packet
    mov ah, 0x42
    int 0x13
    jc disk_error

    mov dl, [boot_drive]
    push word 0x0000
    push word [stage2_load_segment]
    retf

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
    mov bl, 0x0F
    int 0x10
    jmp .next
.done:
    popa
    ret

boot_drive db 0
msg_banner db 13, 10, 'MMUKO BIOS VBR', 13, 10, 0
msg_error db 'VBR read error', 13, 10, 0

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

times 510-($-$$) db 0
dw 0xAA55
