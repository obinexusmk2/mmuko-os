; boot.asm - MMUKO-OS ring boot sector with shared boot contract handoff
; Build: nasm -f bin boot.asm -o boot.bin

BITS 16
ORG  0x7C00

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

T_YES    equ 0x01
T_NO     equ 0x00
T_MB     equ 0xFF
ST_HOLD  equ 0x00
ST_PASS  equ 0x01
ST_ALRT  equ 0x02
ST_FAULT equ 0x03

CT_ADDR   equ 0x0600
CT_SIZE   equ 52
CT_O_CUR  equ 14
CT_O_LAST equ 16
CT_O_FLG  equ 18
CT_O_T1   equ 20
CT_O_T2   equ 21
CT_O_THR  equ 24
CT_O_ST   equ 48
CT_O_RS   equ 49
CT_O_CK   equ 50

PH_PREP   equ 0
PH_N      equ 1
PH_S      equ 2
PH_IID    equ 3
PH_G      equ 4
PH_IPR    equ 5
PH_IIN    equ 6
PH_HO     equ 7
PH_DONE   equ 8

FL_NEEDS  equ 0x0001
FL_ROTOK  equ 0x0004

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov byte [t1], T_MB
    mov byte [t2], T_MB
    mov word [threshold], 0
    mov word [phase_last], PH_PREP
    mov word [phase_cur], PH_N
    mov byte [status], ST_HOLD
    mov byte [reason], 0x10
    mov word [flags], FL_NEEDS

    int 0x11
    test ax, ax
    jz .alert
    mov byte [t1], T_YES
    mov word [phase_last], PH_N
    mov word [phase_cur], PH_S

    int 0x12
    mov word [threshold], 120
    mov byte [t2], T_YES
    mov word [phase_last], PH_S
    mov word [phase_cur], PH_IID

    mov word [threshold], 240
    mov word [phase_last], PH_G
    mov word [phase_cur], PH_IIN
    or  word [flags], FL_ROTOK
    mov byte [status], ST_PASS
    mov byte [reason], 0x7F
    mov word [phase_last], PH_HO
    mov word [phase_cur], PH_DONE
    jmp short .write

.alert:
    mov byte [t1], T_NO
    mov byte [status], ST_ALRT
    mov byte [reason], 0x21
    mov word [phase_last], PH_N
    mov word [phase_cur], PH_HO

.write:
    call write_contract
.hang:
    hlt
    jmp short .hang

write_contract:
    push ds
    push es
    push si
    push di
    push cx
    push ax
    push bx

    push cs
    pop ds
    xor ax, ax
    mov es, ax
    mov si, contract_template
    mov di, CT_ADDR
    mov cx, CT_SIZE
    rep movsb

    mov ax, [phase_cur]
    mov [es:CT_ADDR + CT_O_CUR], ax
    mov ax, [phase_last]
    mov [es:CT_ADDR + CT_O_LAST], ax
    mov ax, [flags]
    mov [es:CT_ADDR + CT_O_FLG], ax
    mov al, [t1]
    mov [es:CT_ADDR + CT_O_T1], al
    mov al, [t2]
    mov [es:CT_ADDR + CT_O_T2], al
    mov ax, [threshold]
    mov [es:CT_ADDR + CT_O_THR], ax
    mov al, [status]
    mov [es:CT_ADDR + CT_O_ST], al
    mov al, [reason]
    mov [es:CT_ADDR + CT_O_RS], al
    mov byte [es:CT_ADDR + CT_O_CK], 0

    xor bx, bx
    xor si, si
.ck:
    cmp si, CT_O_CK
    je .next
    mov al, [es:CT_ADDR + si]
    add bl, al
.next:
    inc si
    cmp si, CT_SIZE
    jb .ck
    mov [es:CT_ADDR + CT_O_CK], bl

    pop bx
    pop ax
    pop cx
    pop di
    pop si
    pop es
    pop ds
    ret

phase_cur dw 0
phase_last dw 0
flags dw 0
threshold dw 0
t1 db 0
t2 db 0
status db 0
reason db 0

contract_template:
    dd 0x4D425443
    dd 0x4E534947
    dd 0x00000001
    dw 0x0001
    dw PH_PREP
    dw PH_PREP
    dw FL_NEEDS
    db T_MB
    db T_MB
    db 0
    db 0
    dw 240
    dw 1
    dd 0x00000600
    dw CT_SIZE
    dw 0x7C00
    dw CT_SIZE
    dw 0
    dd 0
    dd 0
    db ST_HOLD
    db 0x10
    db 0
    db CT_SIZE

times 510-($-$$) db 0
dw 0xAA55
