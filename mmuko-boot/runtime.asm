; Native MMUKO runtime image with explicit header for stage-2 validation.

BITS 32
ORG 0x00010000

%define RUNTIME_MAGIC     0x4D4D4B52
%define RUNTIME_SIGNATURE 0x4E534947

header:
    dd RUNTIME_MAGIC
    dd RUNTIME_SIGNATURE
    dw 1
    dw header_end - header
    dd runtime_end - header
    dd runtime_entry - header
    dd 0x00010000
    dd 0
header_end:

runtime_entry:
    mov esi, runtime_message
    mov edi, 0xB8000
    mov ah, 0x1F
.print:
    lodsb
    test al, al
    jz .done
    stosw
    jmp .print
.done:
    cli
.halt:
    hlt
    jmp .halt

runtime_message db 'MMUKO runtime entered via stage-2', 0
runtime_end:
