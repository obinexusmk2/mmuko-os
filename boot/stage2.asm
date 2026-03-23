; boot/stage2.asm - MMUKO-OS BIOS stage-2 loader
; Runs at 0000:8000, initializes the NSIGII interface/state block,
; publishes a fixed memory map, and jumps into the runtime at 0000:8200.

BITS 16
%include "boot/contract.inc"
ORG STAGE2_LOAD_OFFSET

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x9000
    sti

    mov si, msg_stage2
    call print_string
    call init_state_block
    call publish_memory_map
    call evaluate_membrane

    mov si, msg_jump
    call print_string
    jmp KERNEL_LOAD_SEGMENT:KERNEL_LOAD_OFFSET

init_state_block:
    mov dword [STATE_BLOCK_BASE + STATE_SIG_OFF], STATE_SIGNATURE
    mov word  [STATE_BLOCK_BASE + STATE_VER_OFF], CONTRACT_VERSION

    mov al, [CONTRACT_BASE + CONTRACT_BOOT_DRIVE_OFF]
    mov [STATE_BLOCK_BASE + STATE_BOOT_DRIVE_OFF], al

    int 0x12
    mov [CONTRACT_BASE + CONTRACT_MEM_KB_OFF], ax
    mov [STATE_BLOCK_BASE + STATE_MEM_KB_OFF], ax

    mov byte [STATE_BLOCK_BASE + STATE_MEMBRANE_OFF], O_HOLD
    mov byte [STATE_BLOCK_BASE + STATE_TIER1_OFF], T_MB
    mov byte [STATE_BLOCK_BASE + STATE_TIER2_OFF], T_MB
    mov word [STATE_BLOCK_BASE + STATE_FLAGS_OFF], 0x0002
    mov word [STATE_BLOCK_BASE + STATE_MSG_OFF], msg_state_ready
    ret

publish_memory_map:
    mov di, MEMMAP_BASE

    mov word [di + 0], CONTRACT_BASE
    mov word [di + 2], 0x0040
    mov word [di + 4], 0x0001
    mov word [di + 6], 0x4E43      ; "CN"
    add di, MEMMAP_ENTRY_SIZE

    mov word [di + 0], STATE_BLOCK_BASE
    mov word [di + 2], 0x0040
    mov word [di + 4], 0x0001
    mov word [di + 6], 0x5354      ; "TS"
    add di, MEMMAP_ENTRY_SIZE

    mov word [di + 0], STAGE2_LOAD_OFFSET
    mov word [di + 2], 0x0200
    mov word [di + 4], 0x0002
    mov word [di + 6], 0x324C      ; "L2"
    add di, MEMMAP_ENTRY_SIZE

    mov word [di + 0], KERNEL_LOAD_OFFSET
    mov word [di + 2], 0x0600
    mov word [di + 4], 0x0003
    mov word [di + 6], 0x4B52      ; "RK"
    ret

evaluate_membrane:
    mov byte [CONTRACT_BASE + CONTRACT_TIER1_OFF], T_YES
    mov byte [STATE_BLOCK_BASE + STATE_TIER1_OFF], T_YES

    mov ax, [CONTRACT_BASE + CONTRACT_MEM_KB_OFF]
    cmp ax, 128
    jb .hold

    mov byte [CONTRACT_BASE + CONTRACT_TIER2_OFF], T_YES
    mov byte [STATE_BLOCK_BASE + STATE_TIER2_OFF], T_YES
    mov byte [CONTRACT_BASE + CONTRACT_MEMBRANE_OFF], O_PASS
    mov byte [STATE_BLOCK_BASE + STATE_MEMBRANE_OFF], O_PASS
    mov word [STATE_BLOCK_BASE + STATE_MSG_OFF], msg_pass
    mov si, msg_pass
    call print_string
    ret

.hold:
    mov byte [CONTRACT_BASE + CONTRACT_TIER2_OFF], T_MB
    mov byte [STATE_BLOCK_BASE + STATE_TIER2_OFF], T_MB
    mov byte [CONTRACT_BASE + CONTRACT_MEMBRANE_OFF], O_HOLD
    mov byte [STATE_BLOCK_BASE + STATE_MEMBRANE_OFF], O_HOLD
    mov word [STATE_BLOCK_BASE + STATE_MSG_OFF], msg_hold
    mov si, msg_hold
    call print_string
    ret

print_string:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x000F
    int 0x10
    jmp print_string
.done:
    ret

msg_stage2      db 13,10, "[stage2] NSIGII handoff", 13,10, 0
msg_state_ready db "[stage2] state block armed", 13,10, 0
msg_pass        db "[stage2] membrane PASS", 13,10, 0
msg_hold        db "[stage2] membrane HOLD", 13,10, 0
msg_jump        db "[stage2] jumping runtime", 13,10, 0

times 512-($-$$) db 0
