.global chmod_wrapper
.section .text

chmod_wrapper:
    ; Arguments: (const char *pathname, mode_t mode)
    mov     $90, %rax        ; __NR_chmod
    mov     %rdi, %rdi       ; pathname (1st arg)
    mov     %rsi, %rsi       ; mode (2nd arg)
    syscall                  ; make the syscall
    ret                      ; return with result in rax
