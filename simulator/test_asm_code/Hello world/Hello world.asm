; Hello, World!

    MOV  #msg, R0

loop:
    MOVB (R0)+, R1
    BEQ  done
    MOV  R1, @#177566
    BR   loop

done:
    MOV  #12., R0     ; LF
    MOV  R0, @#177566
    HALT

msg: .ASCIZ "Hello, World!"
