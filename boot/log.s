; code needs recheck, might give error if used on a diff machine thn mine.
; better comments in case "anyone -XD " wanna undersntand whats going on 

BITS 16
org 0x7C00              ; BIOS loads boot sector here

start:
    mov ax, 0x7C00      
    mov bx, ax

    ; Push some parameters on "stack"
    push word 3
    push word 0x1400
    push word 0x540
    push word -2000
    push word 5

wait_loop:
    in al, 0x60         ; read from keyboard port as example
    test al, al
    jns wait_loop        ; wait until sign bit clears (like bge)

    jmp 0x0000:0x5400    ; jump to loaded code (like jmp *$54000)

; Second snippet (polling device at I/O port 0x177350 equivalent)
device_loop:
    mov dx, 0x3F8        ; COM1 port as a stand-in
    xor ax, ax
    out dx, ax           ; clear
    mov ax, dx
    push ax
    push word 3

check_ready:
    in al, dx
    test al, al
    jns check_ready

    in ax, dx
    test ax, ax
    jnz device_loop

    mov byte [dx], 5
wait2:
    in al, dx
    test al, al
    jns wait2

    xor ax, ax
    jmp $

times 510-($-$$) db 0   ; fill sector
dw 0xAA55               ; boot signature
