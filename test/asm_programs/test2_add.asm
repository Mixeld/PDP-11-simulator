; test2_add.asm - проверка ADD и SUB
    .ORG 01000
    .EVEN

START:
    MOV #100, R0
    MOV #30, R1
    ADD R1, R0        ; R0 = 130
    
    SUB #20, R0       ; R0 = 110
    
    MOV #50, R2
    ADD R0, R2        ; R2 = 160
    
    MOV R2, R0        ; результат в R0
    
    HALT
    
    .END START