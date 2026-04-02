; boot_sector.asm - MMUKO-OS 512-byte Boot Sector
; M for Mike, U for Uniform, K for Kilo, O for Oscar
; Interdependency Tree Hierarchy Boot System
;
; Assemble: nasm -f bin boot_sector.asm -o mmuko-os.img

BITS 16
ORG 0x7C00

; ============================================================================
; RIFT HEADER (8 bytes)
; ============================================================================
rift_header:
    db 'NXOB'           ; Magic: OBINEXUS signature
    db 0x01             ; Version: 1
    db 0x00             ; Reserved
    db 0xFE             ; Checksum: XOR of header = 0xFE
    db 0x01             ; Flags: Boot flags

; ============================================================================
; BOOT CODE ENTRY POINT
; ============================================================================
start:
    cli                 ; Disable interrupts
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00      ; Stack at 0x7C00 (grows down)
    sti

    ; Print boot message
    mov si, msg_boot
    call print_string

    ; ========================================================
    ; MUCO BOOT SEQUENCE
    ; ========================================================
    
    ; Phase 1: SPARSE - Initialize 8 qubits to NORTH
    mov si, msg_sparse
    call print_string
    
    ; Initialize qubit array (BX = qubit counter)
    xor bx, bx
    mov cx, 8           ; 8 qubits
.init_loop:
    ; Set qubit to NORTH (0) with half-spin (1)
    mov byte [qubits + bx], 0x01  ; state=SPARSE, half_spin=1
    inc bx
    loop .init_loop
    
    ; Allocate North/East qubits (0, 1, 2)
    mov byte [qubits + 0], 0x01   ; NORTH
    mov byte [qubits + 1], 0x11   ; NORTHEAST (dir=1, half=1)
    mov byte [qubits + 2], 0x21   ; EAST (dir=2, half=1)
    
    ; Phase 2: REMEMBER - Verify conjugate pairs
    mov si, msg_remember
    call print_string
    
    ; Verify North-South pair (qubits 0 and 4)
    mov al, [qubits + 0]
    and al, 0x0F        ; Get state
    cmp al, 0x01        ; Check if allocated
    jne verify_fail
    
    ; Allocate South/West qubits (4, 5, 6)
    mov byte [qubits + 4], 0x41   ; SOUTH (dir=4, half=1)
    mov byte [qubits + 5], 0x51   ; SOUTHWEST (dir=5, half=1)
    mov byte [qubits + 6], 0x61   ; WEST (dir=6, half=1)
    
    ; Phase 3: ACTIVE - All qubits active
    mov si, msg_active
    call print_string
    
    ; Allocate remaining qubits (3, 7)
    mov byte [qubits + 3], 0x31   ; SOUTHEAST (dir=3, half=1)
    mov byte [qubits + 7], 0x71   ; NORTHWEST (dir=7, half=1)
    
    ; Set all to ACTIVE state (high nibble = 1)
    mov cx, 8
    mov bx, 0
.set_active:
    mov al, [qubits + bx]
    or al, 0x10         ; Set ACTIVE bit
    mov [qubits + bx], al
    inc bx
    loop .set_active
    
    ; Phase 4: VERIFY - NSIGII check
    mov si, msg_verify
    call print_string
    
    ; Count verified qubits (ACTIVE state with half_spin)
    xor bx, bx          ; Verified count
    mov cx, 8
    mov si, qubits
.count_loop:
    lodsb
    and al, 0x11        ; Check ACTIVE and half_spin bits
    cmp al, 0x11
    jne .not_verified
    inc bx
.not_verified:
    loop .count_loop
    
    ; NSIGII Verification: 6+ qubits = YES
    cmp bx, 6
    jge verify_success
    
    ; 3- qubits = NO
    cmp bx, 3
    jl verify_fail
    
    ; 4-5 qubits = MAYBE (treat as fail for boot)
    jmp verify_fail

; ============================================================================
; VERIFICATION RESULTS
; ============================================================================
verify_success:
    mov si, msg_success
    call print_string
    
    ; Output NSIGII_YES to debug port
    mov al, NSIGII_YES
    out 0x80, al
    
    ; Halt with success
    hlt
    jmp $               ; Safety loop

verify_fail:
    mov si, msg_fail
    call print_string
    
    ; Output NSIGII_NO to debug port
    mov al, NSIGII_NO
    out 0x80, al
    
    ; Halt with failure
    hlt
    jmp $

; ============================================================================
; UTILITY FUNCTIONS
; ============================================================================

; Print string at DS:SI
print_string:
    push ax
    push bx
    mov ah, 0x0E        ; BIOS teletype function
    mov bh, 0x00        ; Page 0
    mov bl, 0x07        ; Light gray color
.loop:
    lodsb               ; Load byte from SI
    test al, al
    jz .done
    int 0x10            ; Print character
    jmp .loop
.done:
    pop bx
    pop ax
    ret

; ============================================================================
; DATA SECTION
; ============================================================================

; NSIGII Constants
NSIGII_YES      equ 0x55
NSIGII_NO       equ 0xAA
NSIGII_MAYBE    equ 0x00

; Boot messages
msg_boot:       db '=== MMUKO-OS RINGBOOT ===', 0x0D, 0x0A
                db 'OBINEXUS NSIGII Verify', 0x0D, 0x0A
                db 0x0A, 0
msg_sparse:     db '[Phase 1] SPARSE', 0x0D, 0x0A, 0
msg_remember:   db '[Phase 2] REMEMBER', 0x0D, 0x0A, 0
msg_active:     db '[Phase 3] ACTIVE', 0x0D, 0x0A, 0
msg_verify:     db '[Phase 4] VERIFY', 0x0D, 0x0A, 0
msg_success:    db 0x0A, 'NSIGII_VERIFIED', 0x0D, 0x0A
                db 'BOOT_SUCCESS', 0x0D, 0x0A, 0
msg_fail:       db 0x0A, 'NSIGII_FAILED', 0x0D, 0x0A
                db 'BOOT_FAIL', 0x0D, 0x0A, 0

; Qubit array (8 bytes)
; Format: [ACTIVE:1][DIR:3][HALF:1][RES:3]
; Stored at end of boot sector
qubits:         times 8 db 0x00

; ============================================================================
; PADDING AND BOOT SIGNATURE
; ============================================================================

; Pad to 510 bytes
times 510-($-$$) db 0x00

; Boot signature
dw 0xAA55
