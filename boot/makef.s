    .section .rodata
msg_q:      .asciz "?\n"          ; error message string
a_tmp1:     .asciz "a.tmp1\0"     ; temp file 1
a_tmp2:     .asciz "a.tmp2\0"     ; temp file 2
a_tmp3:     .asciz "a.tmp3\0"     ; temp file 3
outfile:    .asciz "l.out\0"      ; output file
aoutname:   .asciz "a.out\0"      ; final a.out
exec_path:  .asciz "/etc/as2\0"   ; external assembler path
arg_g:      .asciz "-g\0"         ; assembler option

    .section .data
outbuf:     .space 512            ; buffer (512 bytes)
pof:        .long 0               ; placeholder fd
fbfil:      .long 0               ; placeholder fd
errflg:     .byte 0               ; error flag
txtsiz:     .long 0               ; text size placeholder
symend:     .long 0               ; symbol end pointer
usymtab:    .space 256            ; user symbol table
rlist:      .long 0

    .section .text
    .globl _start
_start:
    jmp go                        ; jump into program body

go:
    ; write outbuf → fd in pof
    movl    $4, %eax              ; sys_write
    movl    pof, %ebx             ; fd
    movl    $outbuf, %ecx         ; buffer
    movl    $512, %edx            ; size
    int     $0x80

    ; close pof
    movl    $6, %eax
    movl    pof, %ebx
    int     $0x80

    ; close fbfil
    movl    $6, %eax
    movl    fbfil, %ebx
    int     $0x80

    ; check errflg
    movb    errflg, %al
    testb   %al, %al
    jne     aexit                 ; if nonzero, exit

    ; creat a.tmp3
    movl    $8, %eax              ; sys_creat
    movl    $a_tmp3, %ebx
    movl    $0666, %ecx
    int     $0x80
    cmpl    $0, %eax
    jl      aexit
    movl    %eax, %ebx            ; save fd

    
    movl    txtsiz, %ecx
    incl    (%ecx)

    movl    $4, %eax              ; sys_write
    movl    %ebx, %ebx
    movl    $txtsiz, %ecx
    movl    $6, %edx
    int     $0x80

    ; write user symbol table (usymtab..symend)
    movl    symend, %esi
    subl    $usymtab, %esi
    movl    $4, %eax
    movl    %ebx, %ebx
    movl    $usymtab, %ecx
    movl    %esi, %edx
    int     $0x80

    ; close temp file
    movl    $6, %eax
    movl    %ebx, %ebx
    int     $0x80

    ; execve("/etc/as2", ["-g"], NULL)
    pushl   $0                    ; argv[2] = NULL
    pushl   $arg_g                ; argv[1] = "-g"
    pushl   $exec_path            ; argv[0] = path
    movl    %esp, %ecx            ; argv pointer

    movl    $exec_path, %ebx      ; filename
    movl    %ecx, %ecx            ; argv
    xorl    %edx, %edx            ; envp = NULL

    movl    $11, %eax             ; sys_execve
    int     $0x80


aexit:
    movl    $10, %eax             ; sys_unlink
    movl    $a_tmp1, %ebx
    int     $0x80

    movl    $10, %eax
    movl    $a_tmp2, %ebx
    int     $0x80

    movl    $10, %eax
    movl    $a_tmp3, %ebx
    int     $0x80

    movl    $1, %eax              ; sys_exit
    xorl    %ebx, %ebx
    int     $0x80

filerr:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp), %edi         ; filename ptr
    xorl    %ecx, %ecx
.fcount:
    cmpb    $0, (%edi,%ecx,1)     ; strlen loop
    je      .fcount_done
    incl    %ecx
    jmp     .fcount
.fcount_done:
    movl    $4, %eax              ; sys_write
    movl    $1, %ebx              ; stdout
    movl    $msg_q, %ecx
    movl    $2, %edx
    int     $0x80

    movl    $4, %eax              ; sys_write filename
    movl    $1, %ebx
    movl    8(%ebp), %ecx
    movl    %ecx, %ecx
    movl    %ecx, %ecx
    movl    %ecx, %ecx
    movl    %ecx, %edx            ; length already in ecx (reused)
    int     $0x80

    movl    %ebp, %esp
    popl    %ebp
    ret

; fcreat: wrapper around creat() with simple retry/error reporting


fcreat:
    pushl   %ebp
    movl    %esp, %ebp
    movl    8(%ebp), %edi
1:
    movl    $8, %eax              ; sys_creat
    movl    %edi, %ebx
    movl    $0666, %ecx
    int     $0x80
    jge     .ok
    pushl   %edi                  ; on error → filerr
    call    filerr
    addl    $4, %esp
    movl    $1, %eax
    popl    %ebp
    ret
.ok:
    movl    %eax, %eax
    popl    %ebp
    ret



nxtarg:
    xorl    %eax, %eax
    ret

load1:
    ret

load2:
    ret

txtlod:
    ret
