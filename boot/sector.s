.global fstat_wrapper
.section .text

fstat_wrapper:
    ; int fstat(int fd, struct stat *buf)
    mov     $5, %rax          
    mov     %rdi, %rdi        ; fd
    mov     %rsi, %rsi        ; buf pointer
    syscall
    test    %rax, %rax        ; check return value
    jge     .ok               ; success → >=0
    mov     $1, %rax          ; return 1 on failure
    ret
.ok:
    xor     %rax, %rax        ; return 0 on success
    ret
