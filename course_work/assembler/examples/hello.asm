; hello.asm - вывод строки на консоль (через эмулятор)
    .ORG 01000
    .EVEN

START:
    MOV #MSG, R0
    JSR PC, PRINT
    HALT

PRINT:
    MOVB (R0)+, R1
    BEQ EXIT
    MOV #1, R2      ; stdout
    EMT 020         ; системный вызов вывода
    BR PRINT
EXIT:
    RTS PC

MSG:
    .ASCII "Hello, World!"
    .BYTE 0

    .END START