; test7_all.asm - все инструкции вместе
    .ORG 01000
    .EVEN

START:
    ; MOV
    MOV #123, R0
    MOV R0, R1
    
    ; ADD/SUB
    ADD #100, R0
    SUB #50, R0
    
    ; INC/DEC
    INC R1
    DEC R1
    
    ; CLR
    CLR R2
    
    ; TST
    TST R0
    
    ; COM
    COM R3
    
    ; NEG
    NEG R4
    
    ; Сдвиги
    ASL R0
    ASR R0
    ROL R0
    ROR R0
    
    ; Стек
    MOV #100000, SP
    MOV R0, -(SP)
    MOV (SP)+, R5
    
    ; Подпрограмма
    JSR PC, SUBR
    
    HALT

SUBR:
    ADD #10, R0
    RTS PC

    .END START