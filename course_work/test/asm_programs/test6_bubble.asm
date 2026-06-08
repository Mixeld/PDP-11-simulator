; test6_bubble.asm - пузырьковая сортировка
    .ORG 01000
    .EVEN

START:
    MOV #ARRAY, R5
    
OUTER:
    CLR R4
    MOV #10, R3       ; размер 8 элементов (10 восьм.)
    MOV R5, R0
    
INNER:
    MOV (R0), R1
    MOV 2(R0), R2
    CMP R1, R2
    BLE NOSWAP
    MOV R2, (R0)
    MOV R1, 2(R0)
    MOV #1, R4
NOSWAP:
    ADD #2, R0
    SOB R3, INNER
    TST R4
    BNE OUTER
    
    ; ВОТ ЭТИ СТРОКИ ДОБАВИТЬ:
    MOV #1, R0         ; результат в R0 (успешная сортировка)
    HALT

ARRAY:
    .WORD 7, 3, 9, 1, 5, 8, 2, 4
    
    .END START