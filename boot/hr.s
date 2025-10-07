; Because I'm writing my OS from scratch -,- so i just have to manually reimplement all 
; routines (I/O, buffer mgmt, symbol handling etc etc, in x86 assembly to make them work on my machine.
; the prvs one sucks. found some error and has been fixed, it may still need more fixes!
; okay didnt work too. so rewritten -,-



; This file handles disk reads using BIOS INT 13h extensions.
; real mode, 16-bit, stack args
; DL = drive number (0x80 for Hdd). 


bits 16
org 0


SECTION .data
; ah yes... the "filesystem" — if you can even call it that.
; basically a glorified table of filenames, LBA, and sizes.

DIR_SECTOR      dw 100         ; where our fake directry lives!!!
N_ENTRIES       dw 16         

dap:
    db 16
    db 0
    dw 0          ; sectors (to fill at runtime)
    dw 0          
    dw 0         
    dq 0          ; 64-bit LBA, even tho ig ill only use 32 bits lol


SECTION .bss
align 16
sector_buf:       resb 512      ; single.. buffer for reading sectors

align 4
fd_table:         resb 12*16   


SECTION .text
global attach
global rdchk
global getwrd
global putwrd
global flush
global aopen
global disk_read_lba
global memcpy
global memcmp16
; basically a fancy BIOS “please read disk” beggar routine.
; caller sets DL to drive number.
disk_read_lba:
    push bp
    mov bp, sp

   
    mov word [dap+2], [bp+8]     
    mov word [dap+4], [bp+4]   
    mov word [dap+6], [bp+6]    
    mov word [dap+8], [bp+10]    
    mov word [dap+10], [bp+12]   
    mov dword [dap+12], 0        

   
    push ds
    pop es
    lea di, [dap]

    mov ah, 0x42
    int 0x13
    jc .disk_err

    xor ax, ax
    jmp .disk_ok

.disk_err:
    mov ax, 0xFFFF      ; -1, because everything’s broken
.disk_ok:
    pop bp
    ret


;=========================================================
memcpy:
    push bp
    mov bp, sp
    mov di, [bp+4]
    mov es, [bp+6]
    mov si, [bp+8]
    mov ds, [bp+10]
    mov cx, [bp+12]
    rep movsb
    pop bp
    ret



memcmp16:
    push bp
    mov bp, sp
    mov si, [bp+4]
    mov ds, [bp+6]
    mov di, [bp+8]
    mov es, [bp+10]
    mov cx, [bp+12]
    xor ax, ax
.cmp_loop:
    cmp cx, 0
    je .done
    mov al, [ds:si]
    mov ah, [es:di]
    cmp al, ah
    jne .diff
    inc si
    inc di
    dec cx
    jmp .cmp_loop
.diff:
    mov ax, 1
.done:
    pop bp
    ret

; Reads the directory sector n searches for a filename.

aopen:
    push bp
    mov bp, sp

    ; load dir sector
    mov word [dap+2], 1
    mov word [dap+4], sector_buf
    mov word [dap+6], cs
    mov dword [dap+8], DIR_SECTOR
    mov dword [dap+12], 0
    push ds
    pop es
    lea di, [dap]

    mov ah, 0x42
    int 0x13
    jc .aopen_err

    mov si, sector_buf
    xor bx, bx
    mov cx, [N_ENTRIES]

.aopen_loop:
    cmp bx, cx
    jge .notfound

    
    push word 11
    push word [bp+6]      
    push word [bp+4]     
    push word cs
    push word si
    call memcmp16
    add sp, 10

    cmp ax, 0
    je .found

    add si, 19    
    inc bx
    jmp .aopen_loop

.found:
    ; write entry to fd_table[bx]
    mov di, bx
    mov dx, 12
    mul dx
    add di, fd_table
    mov ax, bx
    jmp .done

.notfound:
    mov ax, 0xFFFF
    jmp .done

.aopen_err:
    mov ax, 0xFFFF
.done:
    pop bp
    ret

attach:
    push bp
    mov bp, sp

    mov bx, [bp+4]        ; fd
    cmp bx, [N_ENTRIES]
    jae .err             

    ; 14th hour straight into this and rn at this point im.. too tired to do real math.
    ; assume offset is 0 and just read the start sector.
    mov word [dap+2], [bp+14]
    mov word [dap+4], [bp+10]
    mov word [dap+6], [bp+12]
    mov word [dap+8], 0
    mov word [dap+10], 0
    mov dword [dap+12], 0

    push ds
    pop es
    lea di, [dap]
    mov ah, 0x42
    int 0x13
    jc .err

    mov ax, [bp+14]
    shl ax, 9          ; convert sectors → bytes (×512)
    jmp .done

.err:
    mov ax, 0xFFFF
.done:
    pop bp
    ret


rdchk:
    push bp
    mov bp, sp
    call attach
    cmp ax, 0xFFFF
    je .fail
    pop bp
    ret
.fail:
    mov ax, 0xFFFF
    pop bp
    ret

getwrd:
    push bp
    mov bp, sp
    mov ax, 0xFFFF     ; TODO: actually do it someday
    pop bp
    ret

putwrd:
    mov ax, 0xFFFF
    ret

flush:
    mov ax, 0xFFFF
    ret

; done...