; baSic_ - boot/boot.asm
; Copyright (C) 2026 Dhrubo
; GPL v2 — see LICENSE

[BITS 16]
[ORG 0x7C00]

KERNEL_LOAD_ADDR  equ 0x8000   ; physical address to load stage2+kernel

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00          ; stack grows down from MBR
    sti

    mov [boot_drive], dl    ; BIOS passes boot drive in DL

    mov si, msg_loading
    call bios_print

    call load_kernel

    mov si, msg_ok
    call bios_print

    ; hand off to stage2
    jmp KERNEL_LOAD_ADDR

; load_kernel: loads KERNEL_SECTORS sectors starting at sector 2 into
;              ES:BX = 0x0000:0x8000 using INT 13h CHS read
load_kernel:
    mov si, dap1
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    mov si, dap2
    mov ah, 0x42
    mov dl, [boot_drive]
    int 0x13
    jc disk_error
    ret

dap1:
    db 0x10
    db 0x00
    dw 64                   ; first 64 sectors
    dw KERNEL_LOAD_ADDR
    dw 0x0000
    dq 1                    ; LBA 1

dap2:
    db 0x10
    db 0x00
    dw 64
    dw 0x0000       ; offset
    dw 0x1000       ; segment → physical 0x10000
    dq 65   

disk_error:
    mov si, msg_disk_err
    call bios_print
    cli
    hlt

bios_print:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    xor bh, bh
    int 0x10
    jmp bios_print
.done:
    ret

msg_loading  db 'baSic_: loading kernel...', 0x0D, 0x0A, 0
msg_ok       db 'OK. Entering stage2.', 0x0D, 0x0A, 0
msg_disk_err db '[ERROR] Disk read failed!', 0x0D, 0x0A, 0
boot_drive   db 0

; Pad to exactly 512 bytes and place boot signature
times 510 - ($ - $$) db 0
dw 0xAA55