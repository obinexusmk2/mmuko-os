; MMUKO stage-1 boot sector with shared boot-contract serialization.

BITS 16
ORG 0x7C00

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
          dd 0x4E534947
          db "MMUKO-RING "
          db "FAT12   "

T_NO  equ 0x00
T_MB  equ 0xFF
O_PASS equ 0xAA
O_HOLD equ 0xBB
O_ALRT equ 0xCC

CB   equ 0x0500
CMAG equ 0x4D42
CVER equ 0x0001
CSIZ equ 52
COF_FLAGS    equ 0x06
COF_XFER     equ 0x08
COF_OUTCOME  equ 0x09
COF_PHASE    equ 0x0A
COF_KEYLEN   equ 0x10
COF_KEYCAP   equ 0x11
COF_KEYSCAN  equ 0x12
COF_KEYFLAGS equ 0x13
COF_KEYBUF   equ 0x14

BF_KEY_REQ equ 0x0001
BF_KEY_OK  equ 0x0002
BF_C_READY equ 0x0004
BF_STAGE2  equ 0x0008
BF_NSIGII  equ 0x0020

TX_STAGE1 equ 1
TX_KEYS   equ 2
TX_STAGE2 equ 3
TX_C      equ 4

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov ax, 0x07C0
    mov ds, ax
    mov [drv], dl

    call clear_screen
    call init_contract
    mov si, msg_boot
    call pr
    call read_keyboard

    mov word [es:CB + COF_FLAGS], BF_KEY_REQ | BF_C_READY | BF_STAGE2
    cmp byte [es:CB + COF_KEYLEN], 0
    je .phase_n
    or word [es:CB + COF_FLAGS], BF_KEY_OK
.phase_n:
    mov byte [es:CB + COF_XFER], TX_STAGE2
    mov byte [es:CB + COF_PHASE], 1
    int 0x11
    test ax, ax
    jz fail

    mov byte [es:CB + COF_PHASE], 2
    int 0x12
    mov byte [es:CB + COF_PHASE], 3
    mov byte [es:CB + COF_PHASE], 4
    mov byte [es:CB + COF_PHASE], 5
    mov byte [es:CB + COF_PHASE], 6

pass:
    mov byte [oc], O_PASS
    mov byte [es:CB + COF_OUTCOME], O_PASS
    or word [es:CB + COF_FLAGS], BF_NSIGII
    mov byte [es:CB + COF_XFER], TX_C
    mov si, msg_pass
    call pr
    jmp hang

fail:
    mov byte [t1], T_NO
    mov byte [oc], O_ALRT
    mov byte [es:CB + COF_OUTCOME], O_ALRT
    or word [es:CB + COF_FLAGS], BF_NSIGII
    mov si, msg_fail
    call pr
    jmp hang

clear_screen:
    mov ax, 0x0600
    mov bh, 0x07
    xor cx, cx
    mov dx, 0x184F
    int 0x10
    xor bh, bh
    mov ah, 0x02
    xor dx, dx
    int 0x10
    ret

init_contract:
    mov word [es:CB + 0x00], CMAG
    mov word [es:CB + 0x02], CVER
    mov word [es:CB + 0x04], CSIZ
    mov word [es:CB + COF_FLAGS], BF_KEY_REQ
    mov byte [es:CB + COF_XFER], TX_STAGE1
    mov byte [es:CB + COF_OUTCOME], O_HOLD
    mov byte [es:CB + COF_PHASE], 0
    mov byte [es:CB + COF_KEYLEN], 0
    mov byte [es:CB + COF_KEYCAP], 32
    mov byte [es:CB + COF_KEYSCAN], 0
    mov byte [es:CB + COF_KEYFLAGS], 0
    ret

read_keyboard:
    xor bx, bx
.read:
    xor ah, ah
    int 0x16
    cmp al, 13
    je .done
    cmp bx, 32
    jae .read
    mov [es:CB + COF_KEYSCAN], ah
    mov [es:CB + COF_KEYBUF + bx], al
    inc bx
    mov [es:CB + COF_KEYLEN], bl
    mov byte [es:CB + COF_KEYFLAGS], 1
    mov ah, 0x0E
    xor bh, bh
    mov bl, 0x0F
    int 0x10
    jmp .read
.done:
    mov byte [es:CB + COF_XFER], TX_KEYS
    ret

pr:
    lodsb
    test al, al
    jz .ret
    mov ah, 0x0E
    xor bh, bh
    mov bl, 0x0F
    int 0x10
    jmp pr
.ret:
    ret

hang:
    hlt
    jmp hang

drv db 0
t1  db T_MB
oc  db O_HOLD
msg_boot db "MMUKO KEY>",0
msg_pass db 13,10,"PASS",13,10,0
msg_fail db 13,10,"ALERT",13,10,0

times 510-($-$$) db 0
dw 0xAA55
