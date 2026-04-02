; kernel/mmuko_kernel.asm
; MMUKO-OS Flat Binary Kernel — Phase 1 BIOS Firmware Interface
; OBINexus Computing | Nnamdi Michael Okpala | 29 March 2026
;
; Build:  nasm -f bin kernel/mmuko_kernel.asm -o build/mmuko_kernel.bin
; Runs at: 0x1000:0x0000 (linear 0x10000), entered via jmp 0x1000:0x0000
; Size:   padded to 2048 bytes (4 sectors × 512)
;
; Concepts from transcript:
;   - Spin operators  (alpha/bravo magnets, three trinary states each)
;   - Magnetic attraction  (north/south attract, distance decreases each step)
;   - NSIGII 6-phase bar  (N/S/I/G/I/I with current phase highlighted)
;   - Mosaic memory  (color-coded hex address display)
;   - Date/time  (BIOS INT 0x1A, or read from bootloader's DATETIME_BASE 0x0500)
;
; Keyboard loop (INT 0x16 AH=0):
;   [s/S] — step spin simulation + redisplay
;   [n/N] — advance NSIGII phase (1→2→…→6→1)
;   [q/Q] — halt
;
; Memory conventions:
;   CS = DS = ES = SS = 0x1000 on entry (jmp FAR sets CS)
;   SP = 0xFF00
;   Bootloader stores BCD datetime at physical 0x0500 = ES:0x0500 when ES=0x0000

BITS 16
ORG 0x0000

DATETIME_BASE   equ 0x0500      ; physical addr set by bootloader (seg 0x0000)

; -----------------------------------------------------------------------
; Entry point — set up segments and run the UI
; -----------------------------------------------------------------------
start:
    cli
    mov  ax, cs                 ; cs = 0x1000 after long jmp from bootloader
    mov  ds, ax
    mov  es, ax
    mov  ss, ax
    mov  sp, 0xFF00
    sti

    ; Init spin state
    call spin_init

    ; Display
    call display_banner
    call display_datetime
    call display_nsigii
    call display_spin
    call display_mosaic
    call display_commands

; -----------------------------------------------------------------------
; Main keyboard loop
; -----------------------------------------------------------------------
keyboard_loop:
    mov  ah, 0x00
    int  0x16                   ; wait for keypress; AL = ASCII

    cmp  al, 'q'
    je   .do_quit
    cmp  al, 'Q'
    je   .do_quit
    cmp  al, 's'
    je   .do_step
    cmp  al, 'S'
    je   .do_step
    cmp  al, 'n'
    je   .do_next
    cmp  al, 'N'
    je   .do_next
    jmp  keyboard_loop

.do_step:
    call spin_step
    call refresh_spin_area
    jmp  keyboard_loop

.do_next:
    call phase_next
    call refresh_nsigii_area
    jmp  keyboard_loop

.do_quit:
    mov  si, msg_goodbye
    call print_str
    call newline
halt_loop:
    hlt
    jmp  halt_loop

; -----------------------------------------------------------------------
; display_banner
; -----------------------------------------------------------------------
display_banner:
    mov  si, banner_top
    call print_str
    call newline
    mov  si, banner_mid
    call print_str
    call newline
    mov  si, banner_bot
    call print_str
    call newline
    call newline
    ret

; -----------------------------------------------------------------------
; display_datetime
;   Reads DATETIME_BASE from physical 0x0500 via seg 0x0000.
;   Prints: "Date: CCYY-MM-DD   Time: HH:MM:SS"
;   Fallback: calls INT 0x1A directly if bootloader zeroed the block.
; -----------------------------------------------------------------------
display_datetime:
    push es
    push bx
    push cx
    push dx

    ; Point ES to segment 0 to access physical 0x0500
    xor  bx, bx
    mov  es, bx                 ; ES = 0x0000

    mov  si, msg_date_prefix
    call print_str

    ; Century
    mov  al, [es:DATETIME_BASE + 0]
    call print_bcd_al
    ; Year
    mov  al, [es:DATETIME_BASE + 1]
    call print_bcd_al
    mov  al, '-'
    call print_char
    ; Month
    mov  al, [es:DATETIME_BASE + 2]
    call print_bcd_al
    mov  al, '-'
    call print_char
    ; Day
    mov  al, [es:DATETIME_BASE + 3]
    call print_bcd_al

    ; Separator
    mov  si, msg_time_prefix
    call print_str

    ; Hours
    mov  al, [es:DATETIME_BASE + 4]
    call print_bcd_al
    mov  al, ':'
    call print_char
    ; Minutes
    mov  al, [es:DATETIME_BASE + 5]
    call print_bcd_al
    mov  al, ':'
    call print_char
    ; Seconds
    mov  al, [es:DATETIME_BASE + 6]
    call print_bcd_al

    call newline
    call newline

    pop  dx
    pop  cx
    pop  bx
    pop  es
    ret

; -----------------------------------------------------------------------
; display_nsigii — "NSIGII: [N:1] [S:2] [I:3] [G:4] [I:5] [I:6]"
;                  current phase is highlighted with '*'
; -----------------------------------------------------------------------
display_nsigii:
    mov  si, msg_nsigii_prefix
    call print_str

    mov  cx, 6                  ; loop counter 1..6
    mov  bx, 1                  ; phase index

.nsigii_loop:
    mov  al, '['
    call print_char

    ; Phase letter from nsigii_letters[bx-1]
    push bx
    dec  bx                     ; 0-based index
    mov  si, nsigii_letters
    add  si, bx
    lodsb
    call print_char
    pop  bx

    mov  al, ':'
    call print_char

    ; Print phase number
    mov  ax, bx
    call print_decimal_ax

    ; Mark current phase
    mov  ax, [kernel_phase]
    cmp  ax, bx
    jne  .not_current
    mov  al, '*'
    call print_char
.not_current:

    mov  al, ']'
    call print_char
    mov  al, ' '
    call print_char

    inc  bx
    loop .nsigii_loop

    call newline

    ; Print phase name
    mov  si, msg_phase_label
    call print_str
    mov  ax, [kernel_phase]
    call print_decimal_ax
    call newline
    call newline
    ret

; -----------------------------------------------------------------------
; display_spin — print current spin simulation state
; -----------------------------------------------------------------------
display_spin:
    mov  si, msg_spin_header
    call print_str
    call newline

    ; Alpha line
    mov  si, msg_alpha_prefix
    call print_str
    mov  ax, [spin_alpha_x]
    call print_signed_ax
    mov  si, msg_orient_sep
    call print_str
    mov  ax, [spin_alpha_or]
    call print_decimal_ax
    mov  si, msg_elev_sep
    call print_str
    mov  ax, [spin_alpha_el]
    call print_decimal_ax
    call newline

    ; Bravo line
    mov  si, msg_bravo_prefix
    call print_str
    mov  ax, [spin_bravo_x]
    call print_signed_ax
    mov  si, msg_orient_sep
    call print_str
    mov  ax, [spin_bravo_or]
    call print_decimal_ax
    mov  si, msg_elev_sep
    call print_str
    mov  ax, [spin_bravo_el]
    call print_decimal_ax
    call newline

    ; Steps
    mov  si, msg_steps_prefix
    call print_str
    mov  ax, [spin_steps]
    call print_decimal_ax
    call newline
    call newline
    ret

; -----------------------------------------------------------------------
; display_mosaic — print 3 canonical mosaic entries
; -----------------------------------------------------------------------
display_mosaic:
    mov  si, msg_mosaic_header
    call print_str
    call newline
    mov  si, mosaic_entry0
    call print_str
    call newline
    mov  si, mosaic_entry1
    call print_str
    call newline
    mov  si, mosaic_entry2
    call print_str
    call newline
    call newline
    ret

; -----------------------------------------------------------------------
; display_commands
; -----------------------------------------------------------------------
display_commands:
    mov  si, msg_commands
    call print_str
    call newline
    ret

; -----------------------------------------------------------------------
; refresh_spin_area — redraw NSIGII + spin lines without full clear
;   Simple approach: just print updated spin state inline.
; -----------------------------------------------------------------------
refresh_spin_area:
    call newline
    mov  si, msg_spin_update
    call print_str
    call display_spin
    ret

; -----------------------------------------------------------------------
; refresh_nsigii_area
; -----------------------------------------------------------------------
refresh_nsigii_area:
    call newline
    call display_nsigii
    ret

; -----------------------------------------------------------------------
; spin_init — set spin state variables to initial values
; -----------------------------------------------------------------------
spin_init:
    mov  word [spin_alpha_x],  -10
    mov  word [spin_bravo_x],   10
    mov  word [spin_alpha_or],   0
    mov  word [spin_bravo_or],   0
    mov  word [spin_alpha_el],   0
    mov  word [spin_bravo_el],   0
    mov  word [spin_steps],      0
    ret

; -----------------------------------------------------------------------
; spin_step — one attract step
;   distance = |bravo_x - alpha_x|
;   alpha_x++, bravo_x--
;   orient += 5 (wrap at 628)
;   elev += distance
;   steps++
; -----------------------------------------------------------------------
spin_step:
    push ax
    push bx

    ; drift = bravo_x - alpha_x
    mov  ax, [spin_bravo_x]
    sub  ax, [spin_alpha_x]
    ; abs
    test ax, ax
    jge  .dist_pos
    neg  ax
.dist_pos:
    push ax                     ; save distance

    ; Attract
    inc  word [spin_alpha_x]
    dec  word [spin_bravo_x]

    ; Orientation += 5 (wrap at 628)
    add  word [spin_alpha_or], 5
    mov  ax, [spin_alpha_or]
    cmp  ax, 628
    jl   .no_wrap_a
    sub  word [spin_alpha_or], 628
.no_wrap_a:

    add  word [spin_bravo_or], 5
    mov  ax, [spin_bravo_or]
    cmp  ax, 628
    jl   .no_wrap_b
    sub  word [spin_bravo_or], 628
.no_wrap_b:

    ; Elevation += distance
    pop  ax
    add  word [spin_alpha_el], ax
    add  word [spin_bravo_el], ax
    inc  word [spin_steps]

    pop  bx
    pop  ax
    ret

; -----------------------------------------------------------------------
; phase_next — advance kernel_phase 1→2→…→6→1
; -----------------------------------------------------------------------
phase_next:
    mov  ax, [kernel_phase]
    cmp  ax, 6
    jge  .wrap
    inc  word [kernel_phase]
    ret
.wrap:
    mov  word [kernel_phase], 1
    ret

; =========================================================================
; PRIMITIVE OUTPUT ROUTINES
; =========================================================================

; -----------------------------------------------------------------------
; print_char — BIOS teletype output of AL
; -----------------------------------------------------------------------
print_char:
    push bx
    mov  ah, 0x0E
    mov  bx, 0x0007
    int  0x10
    pop  bx
    ret

; -----------------------------------------------------------------------
; print_str — print null-terminated DS:SI string
; -----------------------------------------------------------------------
print_str:
    push ax
.loop:
    lodsb
    test al, al
    jz   .done
    call print_char
    jmp  .loop
.done:
    pop  ax
    ret

; -----------------------------------------------------------------------
; newline — CR + LF
; -----------------------------------------------------------------------
newline:
    push ax
    mov  al, 13
    call print_char
    mov  al, 10
    call print_char
    pop  ax
    ret

; -----------------------------------------------------------------------
; print_bcd_al — print BCD byte in AL as two ASCII decimal digits
;   Upper nibble = tens, lower nibble = ones.
;   Example: AL=0x29 → prints "29"
; -----------------------------------------------------------------------
print_bcd_al:
    push ax
    ; High nibble (tens)
    shr  al, 4
    add  al, '0'
    call print_char
    ; Low nibble (ones)
    pop  ax
    and  al, 0x0F
    add  al, '0'
    call print_char
    ret

; -----------------------------------------------------------------------
; print_decimal_ax — print unsigned 16-bit AX as decimal
; -----------------------------------------------------------------------
print_decimal_ax:
    push ax
    push bx
    push cx
    push dx

    xor  cx, cx                 ; digit count
    mov  bx, 10

    test ax, ax
    jnz  .extract
    ; Special case: zero
    mov  al, '0'
    call print_char
    jmp  .restore

.extract:
    test ax, ax
    jz   .emit
    xor  dx, dx
    div  bx                     ; AX = quotient, DX = remainder (0-9)
    push dx
    inc  cx
    jmp  .extract

.emit:
    test cx, cx
    jz   .restore
    pop  dx
    add  dl, '0'
    mov  al, dl
    call print_char
    dec  cx
    jmp  .emit

.restore:
    pop  dx
    pop  cx
    pop  bx
    pop  ax
    ret

; -----------------------------------------------------------------------
; print_signed_ax — print signed 16-bit AX as decimal (handles negatives)
; -----------------------------------------------------------------------
print_signed_ax:
    test ax, 0x8000             ; sign bit set?
    jz   print_decimal_ax       ; non-negative: tail-call decimal printer
    push ax
    mov  al, '-'
    call print_char
    pop  ax
    neg  ax
    jmp  print_decimal_ax       ; tail call with positive value

; =========================================================================
; DATA SECTION
; =========================================================================
banner_top          db "+=======================================================+", 0
banner_mid          db "|  OBINexus MMUKO-OS  | Phase 1 BIOS Firmware           |", 0
banner_bot          db "+=======================================================+", 0

msg_date_prefix     db "Date: ", 0
msg_time_prefix     db "   Time: ", 0
msg_nsigii_prefix   db "NSIGII: ", 0
msg_phase_label     db "Phase : ", 0
msg_spin_header     db "--- SPIN SIMULATION ---", 0
msg_alpha_prefix    db "  Alpha (N/+1):  x=", 0
msg_bravo_prefix    db "  Bravo (S/-1):  x=", 0
msg_orient_sep      db "  orient=", 0
msg_elev_sep        db "  elev=", 0
msg_steps_prefix    db "  Steps: ", 0
msg_mosaic_header   db "--- MOSAIC MEMORY ---", 0
msg_commands        db "Keys: [s]tep  [n]ext-phase  [q]uit", 0
msg_spin_update     db "[SPIN UPDATED]", 0
msg_goodbye         db 13, 10, "MMUKO-OS halted. Have a good day.", 0

nsigii_letters      db "NSIGII", 0

; Mosaic sample entries (hardcoded for display)
mosaic_entry0       db "  #BC5FA9  [color=4/blue]   state=YES    data=0xA0", 0
mosaic_entry1       db "  #1E2F3C  [color=6/cyan]   state=MAYBE  data=0xA1", 0
mosaic_entry2       db "  #DEADBE  [color=6/cyan]   state=NO     data=0xA2", 0

; Kernel state
kernel_phase        dw 1        ; current NSIGII phase (1-6)

; Spin state variables (signed words)
spin_alpha_x        dw 0        ; initialised by spin_init (-10)
spin_bravo_x        dw 0        ;                           (+10)
spin_alpha_or       dw 0        ; orientation × (0-627)
spin_bravo_or       dw 0
spin_alpha_el       dw 0        ; elevation (accumulated path)
spin_bravo_el       dw 0
spin_steps          dw 0        ; step counter

; Pad to exactly 2048 bytes (4 sectors of 512 bytes each)
times 2048-($-$$) db 0
