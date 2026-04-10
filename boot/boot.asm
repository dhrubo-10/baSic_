%if 0 
    Decided to scrap everything and writing it from scratch.
    Bcs of some complicated decisions made before on this project. 
    SO now writing it from scratch seems easier thn fixing those issues :")
%endif

[BITS 16]
[ORG 0x7C00]

KERNEL_LOAD_ADDR  equ 0x8000   ; physical address to load stage2+kernel
KERNEL_SECTORS    equ 64       ; number of sectors to load (64 * 512 = 32KB)

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
    mov ah, 0x02            ; BIOS: read sectors
    mov al, KERNEL_SECTORS  ; number of sectors
    mov ch, 0               ; cylinder 0
    mov cl, 2               ; start at sector 2 (sector 1 = this MBR)
    mov dh, 0               ; head 0
    mov dl, [boot_drive]
    mov bx, KERNEL_LOAD_ADDR
    int 0x13
    jc .disk_error
    ret

.disk_error:
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

msg_loading  db 'DhrubOS: loading kernel...', 0x0D, 0x0A, 0
msg_ok       db 'OK. Entering stage2.', 0x0D, 0x0A, 0
msg_disk_err db '[ERROR] Disk read failed!', 0x0D, 0x0A, 0
boot_drive   db 0

; Pad to exactly 512 bytes and place boot signature
times 510 - ($ - $$) db 0
dw 0xAA55