; test4_jsr.asm - проверка JSR/RTS подпрограмм
    .ORG 01000
    .EVEN

START:
    MOV #12, R0       ; 10 в десятичной
    JSR PC, DOUBLE
    HALT

DOUBLE:
    ASL R0            ; умножение на 2
    RTS PC
    
    .END START