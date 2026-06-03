; test8_cmp.asm - проверка CMP и переходов
    .ORG 01000
    .EVEN

START:
    MOV #5, R0
    MOV #3, R1
    CMP R0, R1
    BGE L1
    MOV #0, R2
    BR L2
L1:
    MOV #1, R2        ; R2 = 1, если 5 >= 3
    
L2:
    MOV #-2, R3
    TST R3
    BMI L3
    MOV #0, R4
    BR L4
L3:
    MOV #1, R4        ; R4 = 1, если -2 < 0
    
L4:
    MOV #10, R5
    MOV #10, R6
    CMP R5, R6
    BEQ L5
    MOV #0, R0
    HALT
L5:
    MOV #77, R0       ; R0 = 77, если равны
    
    HALT
    
    .END START