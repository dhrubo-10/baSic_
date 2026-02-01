bits 32

global _start
_start:
    ; Setup stack
    mov esp, stack_top
    
    ; Call kernel main
    extern kernel_main
    call kernel_main
    
    ; Halt if kernel returns
    cli
    hlt

section .bss
align 16
stack_bottom:
    resb 16384
stack_top: