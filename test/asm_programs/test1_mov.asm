; test1_mov.asm - проверка MOV и регистров
    .ORG 01000
    .EVEN

START:
    MOV #123, R0
    MOV #456, R1
    MOV R0, R2
    MOV R1, R3
    MOV R2, R4
    MOV R3, R5
    
    HALT
    
    .END START