; boot/stage1.asm - MMUKO-OS BIOS stage-1 boot sector
; Loads a fixed raw stage-2 payload into 0000:8000 and jumps to it.
; The handoff contract lives at 0000:0600.

BITS 16
ORG 0x7C00

%include "boot/contract.inc"

jmp short start
nop
          db "MMUKOOS "
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
          dd 0x3247534E
          db "MMUKO-RING "
          db "FAT12   "

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [boot_drive], dl
    call init_contract
    call load_stage2
    jmp STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET

init_contract:
    mov dword [CONTRACT_BASE + CONTRACT_SIG_OFF], CONTRACT_SIGNATURE
    mov word  [CONTRACT_BASE + CONTRACT_VER_OFF], CONTRACT_VERSION
    mov al, [boot_drive]
    mov byte  [CONTRACT_BASE + CONTRACT_BOOT_DRIVE_OFF], al
    mov word  [CONTRACT_BASE + CONTRACT_STAGE2_SEG_OFF], STAGE2_LOAD_SEGMENT
    mov word  [CONTRACT_BASE + CONTRACT_STAGE2_OFF_OFF], STAGE2_LOAD_OFFSET
    mov word  [CONTRACT_BASE + CONTRACT_STAGE2_SEC_OFF], STAGE2_TOTAL_SECTORS
    mov word  [CONTRACT_BASE + CONTRACT_KERNEL_SEG_OFF], KERNEL_LOAD_SEGMENT
    mov word  [CONTRACT_BASE + CONTRACT_KERNEL_OFF_OFF], KERNEL_LOAD_OFFSET
    mov word  [CONTRACT_BASE + CONTRACT_FLAGS_OFF], 0x0001
    mov word  [CONTRACT_BASE + CONTRACT_STATE_PTR_OFF], STATE_BLOCK_BASE
    mov word  [CONTRACT_BASE + CONTRACT_MEM_KB_OFF], 0
    mov byte  [CONTRACT_BASE + CONTRACT_LAST_ERR_OFF], 0
    mov byte  [CONTRACT_BASE + CONTRACT_MEMBRANE_OFF], O_HOLD
    mov byte  [CONTRACT_BASE + CONTRACT_TIER1_OFF], T_MB
    mov byte  [CONTRACT_BASE + CONTRACT_TIER2_OFF], T_MB
    mov word  [CONTRACT_BASE + CONTRACT_MEMMAP_PTR_OFF], MEMMAP_BASE
    mov byte  [CONTRACT_BASE + CONTRACT_MEMMAP_CNT_OFF], MEMMAP_COUNT
    ret

load_stage2:
    xor ax, ax
    mov es, ax
    mov bx, STAGE2_LOAD_OFFSET
    mov dl, [boot_drive]
    xor ah, ah
    int 0x13
    jc disk_error

    mov ah, 0x02
    mov al, STAGE2_TOTAL_SECTORS
    xor ch, ch
    mov cl, 0x02
    xor dh, dh
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    ret

disk_error:
    mov [CONTRACT_BASE + CONTRACT_LAST_ERR_OFF], ah
    mov byte [CONTRACT_BASE + CONTRACT_MEMBRANE_OFF], O_ALRT
    mov si, disk_error_msg
    call print_string
.hang:
    hlt
    jmp .hang

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp print_string
.done:
    ret

boot_drive db 0

disk_error_msg db "Stage1 disk read failed", 0

times 510-($-$$) db 0
dw 0xAA55
