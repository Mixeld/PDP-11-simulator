; test5_stack.asm - проверка стека
    .ORG 01000
    .EVEN

START:
    MOV #100000, SP   ; установка вершины стека
    
    MOV #111, R0
    MOV R0, -(SP)     ; push R0
    
    MOV #222, R1
    MOV R1, -(SP)     ; push R1
    
    MOV #333, R2
    MOV R2, -(SP)     ; push R2
    
    MOV (SP)+, R5     ; pop -> R5 = 333
    MOV (SP)+, R4     ; pop -> R4 = 222
    MOV (SP)+, R3     ; pop -> R3 = 111
    
    MOV R3, R0        ; результат = 111
    HALT
    
    .END START