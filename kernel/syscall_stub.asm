; baSic_ - kernel/syscall_stub.asm
; Copyright (C) 2026 Dhrubo
; GPL v2 — see LICENSE
;
; int 0x80 entry stub — saves state, calls syscall_handler(), restores

[BITS 32]

extern syscall_handler

section .text

global syscall_stub
syscall_stub:
    cli
    push dword 0        ; dummy error code
    push dword 0x80     ; int number

    pusha

    push esp
    call syscall_handler
    add  esp, 4

    popa
    add  esp, 8         ; pop int number + dummy error code
    sti
    iret