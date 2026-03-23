BITS 16
ORG 0x0000

ROOT_BUFFER_SEG   equ 0x2400
FAT_BUFFER_SEG    equ 0x2000
KERNEL_LOAD_SEG   equ 0x1000
CONFIG_LOAD_SEG   equ 0x2600
ASSET_LOAD_SEG    equ 0x2800

start:
    cli
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0xFFFE
    sti

    mov [boot_drive], dl

    mov si, msg_stage2
    call print_string

    call init_filesystem
    call load_fat
    call load_root_directory

    mov si, kernel_name
    call find_root_entry
    jc file_error
    mov [kernel_cluster], ax
    mov ax, KERNEL_LOAD_SEG
    mov es, ax
    mov ax, [kernel_cluster]
    call load_file_chain

    mov si, config_name
    call find_root_entry
    jc file_error
    mov [config_cluster], ax
    mov ax, CONFIG_LOAD_SEG
    mov es, ax
    mov ax, [config_cluster]
    call load_file_chain

    mov si, asset_name
    call find_root_entry
    jc file_error
    mov [asset_cluster], ax
    mov ax, ASSET_LOAD_SEG
    mov es, ax
    mov ax, [asset_cluster]
    call load_file_chain

    mov si, msg_kernel
    call print_string
    mov dl, [boot_drive]
    jmp KERNEL_LOAD_SEG:0x0000

file_error:
    mov si, msg_missing
    call print_string
    jmp halt_forever

disk_error:
    mov si, msg_disk_error
    call print_string
    jmp halt_forever

halt_forever:
    hlt
    jmp halt_forever

init_filesystem:
    push es
    xor ax, ax
    mov es, ax

    mov bx, 0x7C00 + 13
    mov al, [es:bx]
    mov [sectors_per_cluster], al

    mov bx, 0x7C00 + 14
    mov ax, [es:bx]
    mov [reserved_sectors], ax

    mov bx, 0x7C00 + 16
    mov al, [es:bx]
    mov [fat_count], al

    mov bx, 0x7C00 + 17
    mov ax, [es:bx]
    mov [root_entries], ax

    mov bx, 0x7C00 + 22
    mov ax, [es:bx]
    mov [sectors_per_fat], ax

    mov bx, 0x7C00 + 28
    mov eax, [es:bx]
    mov [hidden_lba], eax

    mov ax, [root_entries]
    add ax, 15
    shr ax, 4
    mov [root_dir_sectors], ax

    xor eax, eax
    mov ax, [reserved_sectors]
    xor bx, bx
    mov bl, [fat_count]
    mov dx, [sectors_per_fat]
    imul bx, dx
    add ax, bx
    add eax, [hidden_lba]
    mov [first_root_lba], eax

    xor eax, eax
    mov ax, [root_dir_sectors]
    add eax, [first_root_lba]
    mov [first_data_lba], eax

    pop es
    ret

load_fat:
    mov eax, [hidden_lba]
    xor ebx, ebx
    mov bx, [reserved_sectors]
    add eax, ebx
    mov cx, [sectors_per_fat]
    mov ax, FAT_BUFFER_SEG
    mov es, ax
    xor bx, bx
    call read_sectors
    ret

load_root_directory:
    mov eax, [first_root_lba]
    mov cx, [root_dir_sectors]
    mov ax, ROOT_BUFFER_SEG
    mov es, ax
    xor bx, bx
    call read_sectors
    ret

find_root_entry:
    push bx
    push cx
    push dx
    push di
    push si

    mov ax, ROOT_BUFFER_SEG
    mov es, ax
    xor di, di
    mov cx, [root_entries]
.next_entry:
    mov al, [es:di]
    cmp al, 0x00
    je .not_found
    cmp al, 0xE5
    je .skip_entry

    push cx
    push di
    push si
    mov bx, 11
.compare:
    mov al, [si]
    cmp al, [es:di]
    jne .no_match
    inc si
    inc di
    dec bx
    jnz .compare

    pop si
    pop di
    pop cx
    mov ax, [es:di + 26]
    clc
    jmp .done

.no_match:
    pop si
    pop di
    pop cx
.skip_entry:
    add di, 32
    loop .next_entry

.not_found:
    stc
.done:
    pop si
    pop di
    pop dx
    pop cx
    pop bx
    ret

load_file_chain:
    push bx
    push cx
    push dx
    push si

    mov [current_cluster], ax
.next_cluster:
    mov ax, [current_cluster]
    cmp ax, 0x0002
    jb .done
    cmp ax, 0xFFF8
    jae .done

    call cluster_to_lba
    xor bx, bx
    xor cx, cx
    mov cl, [sectors_per_cluster]
    call read_sectors

    xor bx, bx
    mov bl, [sectors_per_cluster]
    shl bx, 5
    mov ax, es
    add ax, bx
    mov es, ax

    mov bx, [current_cluster]
    shl bx, 1
    mov ax, FAT_BUFFER_SEG
    mov ds, ax
    mov ax, [bx]
    mov dx, cs
    mov ds, dx
    mov [current_cluster], ax
    jmp .next_cluster

.done:
    pop si
    pop dx
    pop cx
    pop bx
    ret

cluster_to_lba:
    push dx
    push bx

    sub ax, 2
    xor dx, dx
    xor bx, bx
    mov bl, [sectors_per_cluster]
    mul bx
    add ax, [first_data_lba]
    adc dx, [first_data_lba + 2]
    mov [temp_lba], ax
    mov [temp_lba + 2], dx
    mov eax, [temp_lba]

    pop bx
    pop dx
    ret

read_sectors:
    push ax
    push dx
    push si

    mov dword [dap_lba], eax
    mov dword [dap_lba + 4], 0
    mov [dap_count], cx
    mov [dap_offset], bx
    mov ax, es
    mov [dap_segment], ax

    mov dl, [boot_drive]
    mov si, disk_address_packet
    mov ah, 0x42
    int 0x13
    jc disk_error

    pop si
    pop dx
    pop ax
    ret

print_string:
    pusha
.next_char:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0F
    int 0x10
    jmp .next_char
.done:
    popa
    ret

boot_drive db 0
sectors_per_cluster db 0
fat_count db 0
reserved_sectors dw 0
root_entries dw 0
sectors_per_fat dw 0
root_dir_sectors dw 0
hidden_lba dd 0
first_root_lba dd 0
first_data_lba dd 0
temp_lba dd 0
current_cluster dw 0
kernel_cluster dw 0
config_cluster dw 0
asset_cluster dw 0

kernel_name db 'KERNEL  BIN'
config_name db 'BOOT    CFG'
asset_name  db 'NSIGII  TXT'

msg_stage2 db 13, 10, 'MMUKO FAT16 STAGE2', 13, 10, 0
msg_kernel db 'Kernel + config + NSIGII asset loaded', 13, 10, 0
msg_missing db 'Required file missing', 13, 10, 0
msg_disk_error db 'Disk read error', 13, 10, 0

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
