; test3_sob.asm - проверка SOB цикла (сумма 1..8 = 36)
    .ORG 01000
    .EVEN

START:
    MOV #10, R3       ; счетчик 8 (10 восьм.)
    CLR R4            ; сумма = 0
    
LOOP:
    ADD R3, R4        ; sum += R3
    DEC R3            ; R3--
    BNE LOOP          ; if R3 != 0 goto LOOP
    
    MOV R4, R0        ; результат в R0
    HALT
    
    .END START