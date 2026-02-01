bits 32

section .multiboot
align 4
multiboot_header:
    dd 0x1BADB002          ; Magic
    dd 0x03                ; Flags
    dd -(0x1BADB002 + 0x03) ; Checksum

section .text
global _start
_start:
    ; Setup stack
    mov esp, stack_top
    
    ; Clear BSS
    extern bss_start
    extern bss_end
    mov edi, bss_start
    mov ecx, bss_end
    sub ecx, bss_start
    xor eax, eax
    rep stosb
    
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