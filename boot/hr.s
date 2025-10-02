; Because I'm writing my OS from scratch -,- so i just have to manually reimplement all 
; routines (I/O, buffer mgmt, symbol handling etc etc, in x86 assembly to make them work on my machine.


.global attach
.global rdchk
.global getwrd
.global putwrd
.global flush
.global aopen

.section .text


; attach(fd, offset, buf, count)
; Seek to offset in file, then read count bytes into buf
; args: rdi=fd, rsi=offset, rdx=buf, rcx=count

attach:
    push   %rbp
    mov    %rsp, %rbp

    ; lseek(fd, offset, SEEK_SET)
    mov    $8, %rax        ; sys_lseek
    mov    %rdi, %rdi      ; fd
    mov    %rsi, %rsi      ; offset
    mov    $0, %rdx        ; SEEK_SET
    syscall

    ; read(fd, buf, count)
    mov    $0, %rax        ; sys_read
    mov    %rdi, %rdi      ; fd
    mov    %rdx, %rsi      ; buf
    mov    %rcx, %rdx      ; count
    syscall

    leave
    ret


; rdchk(fd, buf, nbytes)
; Safe read with error/EOF check
; args: rdi=fd, rsi=buf, rdx=nbytes

rdchk:
    mov    $0, %rax        ; sys_read
    syscall
    test   %rax, %rax
    jge    .ok
    mov    $-1, %rax       ; error -> return -1
    ret
.ok:
    ret


; getwrd(fd, dst)
; Read a 2-byte word into dst
; args: rdi=fd, rsi=dst

getwrd:
    mov    $0, %rax        ; sys_read
    mov    %rdi, %rdi      ; fd
    mov    %rsi, %rsi      ; dst
    mov    $2, %rdx        ; read 2 bytes
    syscall
    cmp    $2, %rax
    jl     .eof
    ret
.eof:
    mov    $-1, %rax       ; EOF/error
    ret


; putwrd(fd, word)
; Write a 2-byte word to file
; args: rdi=fd, rsi=word

putwrd:
    mov    $1, %rax        ; sys_write
    mov    %rdi, %rdi      ; fd
    lea    word_buf(%rip), %rsi
    movw   %si, (word_buf) ; store word
    mov    $2, %rdx
    syscall
    ret

.section .bss
.lcomm word_buf, 2         ; temporary storage for putwrd

.section .text


; flush(fd, buf, count)
; Write count bytes from buf to fd
; args: rdi=fd, rsi=buf, rdx=count

flush:
    mov    $1, %rax        ; sys_write
    syscall
    ret

; aopen(pathname, flags, mode)
; Open file
; args: rdi=pathname, rsi=flags, rdx=mode

aopen:
    mov    $2, %rax        ; sys_open
    syscall
    ret
