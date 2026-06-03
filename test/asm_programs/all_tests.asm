; Мега-тест: проверяет всё и сразу
    .ORG 01000
    .EVEN

START:
    ; Тест 1: MOV
    MOV #123, R0
    MOV #456, R1
    MOV R0, R2
    
    ; Тест 2: ADD/SUB
    MOV #100, R3
    ADD #30, R3
    SUB #20, R3
    
    ; Тест 3: Стек
    MOV #100000, SP
    MOV R0, -(SP)
    MOV R1, -(SP)
    MOV (SP)+, R4
    MOV (SP)+, R5
    
    ; Тест 4: JSR
    JSR PC, DOUBLE
    
    ; Тест 5: Цикл
    MOV #10, R3
    CLR R2
LOOP:
    ADD R3, R2
    DEC R3
    BNE LOOP
    
    ; Результат в R0
    MOV R2, R0
    HALT

DOUBLE:
    ASL R0
    RTS PC

    .END START