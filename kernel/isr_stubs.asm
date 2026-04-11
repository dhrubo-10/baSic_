; baSic_ - kernel/isr.asm
; Copyright (C) 2026 Shahriar Dhrubo 
; GPL v2  see LICENSE
; ISR stubs for CPU exceptions 0-31
; each stub pushes int_no + err_code, saves state, calls isr_handler()

[BITS 32]

extern isr_handler      ; defined in isr.c

section .text

; -- macro: exception WITH no error code (CPU does not push one)
; we push a dummy 0 so the stack frame is always uniform
%macro ISR_NOERR 1
global isr%1
isr%1:
    cli
    push dword 0        ; dummy error code
    push dword %1       ; interrupt number
    jmp isr_common
%endmacro

; -- macro: exception WITH error code (CPU already pushed it)
%macro ISR_ERR 1
global isr%1
isr%1:
    cli
    push dword %1       ; interrupt number (error code already on stack)
    jmp isr_common
%endmacro

; CPU exceptions 0-31
ISR_NOERR 0    ; divide by zero
ISR_NOERR 1    ; debug
ISR_NOERR 2    ; NMI
ISR_NOERR 3    ; breakpoint
ISR_NOERR 4    ; overflow
ISR_NOERR 5    ; bound range exceeded
ISR_NOERR 6    ; invalid opcode
ISR_NOERR 7    ; device not available
ISR_ERR   8    ; double fault          (has error code)
ISR_NOERR 9    ; coprocessor segment overrun
ISR_ERR   10   ; invalid TSS           (has error code)
ISR_ERR   11   ; segment not present   (has error code)
ISR_ERR   12   ; stack-segment fault   (has error code)
ISR_ERR   13   ; general protection    (has error code)
ISR_ERR   14   ; page fault            (has error code)
ISR_NOERR 15   ; reserved
ISR_NOERR 16   ; x87 FPU fault
ISR_ERR   17   ; alignment check       (has error code)
ISR_NOERR 18   ; machine check
ISR_NOERR 19   ; SIMD FP exception
ISR_NOERR 20   ; virtualization exception
ISR_NOERR 21   ; reserved
ISR_NOERR 22   ; reserved
ISR_NOERR 23   ; reserved
ISR_NOERR 24   ; reserved
ISR_NOERR 25   ; reserved
ISR_NOERR 26   ; reserved
ISR_NOERR 27   ; reserved
ISR_NOERR 28   ; reserved
ISR_NOERR 29   ; reserved
ISR_ERR   30   ; security exception    (has error code)
ISR_NOERR 31   ; reserved

; -- common stub: save full register state, call C handler, restore
isr_common:
    pusha                   ; push eax ecx edx ebx esp ebp esi edi

    ; call C handler with pointer to the register struct on the stack
    push esp
    call isr_handler
    add  esp, 4             ; clean up the pushed pointer

    popa
    add esp, 8              ; pop err_code and int_no
    sti
    iret