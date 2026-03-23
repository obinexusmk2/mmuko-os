BITS 16
ORG 0x0000

start:
    cli
    mov ax, cs
    mov ds, ax
    mov es, ax
    sti

    mov si, msg_kernel
    call print_string

halt:
    hlt
    jmp halt

print_string:
    pusha
.next:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0x00
    mov bl, 0x0A
    int 0x10
    jmp .next
.done:
    popa
    ret

msg_kernel db 13,10,'MMUKO kernel reached.',13,10
           db 'BOOT.CFG and NSIGII.TXT were loaded from FAT16.',13,10
           db 'BIOS disk image pipeline is live.',13,10,0
