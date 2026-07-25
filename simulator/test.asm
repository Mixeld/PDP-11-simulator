wait1:
    BIT #200, @#177560
    BEQ wait1

    MOV @#177562, R0
    BIC #177400, R0

    MOV R0, @#177566
    
    SUB #'0, R0
    MOV R0, R2

wait2:
    BIT #200, @#177560
    BEQ wait2
    
    MOV @#177562, R0
    BIC #177400, R0
    MOV R0, @#177566
    SUB #'0, R0
    MOV R0, R3

    ADD R2, R3

    MOV R3, @#177566   

    SUB #'0, R0
    MOV R0, R3

    ADD R2, R3

    MOV #'=, R0
    MOV R0, @#177566

    ADD #'0, R3
    MOV R3, @#177566

    MOV #12., R0
    MOV R0, @#177566
    
    HALT

