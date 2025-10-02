; what it does? nothing !! just an assembler driver routine (error, betwen, putw, putc)


section .data
errflg:      db 0                  ; error flag
curarg:      dd 0                  ; pointer to current argument (string)
line:        dd 0                  ; current line number
msg_newline: db '\n', 0
obuf:        times 512 db 0        ; output buffer
obufp:       dd obuf               ; current buffer pointer
pof:         dd 1                  ; default fd = stdout
ifflg:       dd 0                  ; formatting flag

section .text
global error, betwen, putw, putc


; error: increment error flag, print current argument + line number

error:
    inc byte [errflg]

    ; save registers (simulate PDP-11 pushes)
    push eax
    push ebx
    push ecx
    push edx

    mov eax, [curarg]
    test eax, eax
    jz .skip_arg

    ; clear curarg so it won’t be reused
    mov dword [curarg], 0

    ; write current arg string
    mov ebx, [pof]         ; fd
    mov ecx, eax           ; buffer ptr
    mov edx, 32            ; assume <=32 chars (placeholder)
    mov eax, 4             ; sys_write
    int 0x80

.skip_arg:
    ; convert line number to ASCII decimal
    mov eax, [line]
    mov ecx, 10
    xor edx, edx
    mov esi, esp           ; temp buf on stack
    add esi, -16           ; reserve 16 bytes
    mov edi, esi

.conv_loop:
    xor edx, edx
    div ecx                ; eax/10 -> quotient, remainder in edx
    add dl, '0'
    dec edi
    mov [edi], dl
    test eax, eax
    jnz .conv_loop

    ;
