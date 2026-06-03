#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Использование: ./quick_test.sh program.asm"
    exit 1
fi

ASM=$1
NAME=$(basename "$ASM" .asm)

echo "Тест: $ASM"

../assembler/asm "$ASM" -o "$NAME"
if [ $? -ne 0 ]; then
    echo "ОШИБКА: Ассемблирование"
    exit 1
fi

../simulator/pdp11 "$NAME.lda"

rm -f "$NAME.lda" "$NAME.lst"