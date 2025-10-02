# boot.s, basically its for like setting up the Cpu mode (real mode -> protected mode)
 

    .section .data
argc:       .long 0
argv:       .long 0
fout:       .long 0
txtsiz:     .long 0
datsiz:     .long 0
bsssiz:     .long 0

symtab:     .space 4096        # dummy symbol table buffer
esymp:      .long 0

outfile:    .asciz "l.out"
aout:       .asciz "a.out"

    .section .text
    .globl _start
_start:
    # stack: [argc][argv...]
    popl %eax
    movl %eax, argc
    movl %esp, argv

    cmpl $1, argc
    jg first_pass
    movl $1, %ebx        # exit code
    movl $1, %eax        # sys_exit
    int  $0x80


first_pass:
    call nxtarg
    testl %eax, %eax
    jz second_pass
    call load1
    jmp first_pass

second_pass:
    movl $outfile, %ebx
    movl $0666, %ecx         # mode
    movl $8, %eax            # sys_creat
    int  $0x80
    testl %eax, %eax
    js exit_fail
    movl %eax, fout

    # compute BSS origin: r3 = txtsiz + datsiz
    movl txtsiz, %ecx
    movl datsiz, %edx
    addl %edx, %ecx

    # clear common size accumulator
    xorl %esi, %esi

resolve_commons:
    movl symtab, %edi
.loop_comm:
    cmpl esymp, %edi
    jae commons_done
    cmpl $0x40, 10(%edi)     # undefined symbol?
    jne next_sym
    movl 12(%edi), %ebx
    testl %ebx, %ebx
    jz next_sym
    movl %esi, 12(%edi)      # assign common origin
    addl %ecx, 12(%edi)
    incl %ebx
    andl $~1, %ebx           # align even
    addl %ebx, %esi
    movb $0x47, 10(%edi)     # temp common type
next_sym:
    addl $14, %edi
    jmp .loop_comm
commons_done:

    # finalize bss size
    addl %esi, bsssiz

    # (rest of pass2: symbol relocation, file writes, etc.)

    jmp done

exit_fail:
    movl $1, %ebx
    movl $1, %eax
    int $0x80

done:
    movl $0, %ebx
    movl $1, %eax
    int $0x80

nxtarg:
    movl $0, %eax
    ret

load1:
    ret
