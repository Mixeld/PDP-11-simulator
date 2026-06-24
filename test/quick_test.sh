#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

if [ $# -ne 1 ]; then
    echo "Использование: ./quick_test.sh program.asm"
    echo "Пример: ./quick_test.sh asm_programs/test1_mov.asm"
    exit 1
fi

ASM=$1
NAME=$(basename "$ASM" .asm)

echo "========================================="
echo "Быстрый тест: $ASM"
echo "========================================="

# Ассемблирование
../assembler/asm "$ASM" -o "$NAME"
if [ $? -ne 0 ]; then
    echo -e "${RED}ОШИБКА: Ассемблирование не удалось${NC}"
    exit 1
fi

# Запуск симулятора
../simulator/pdp11 "$NAME.lda"

# Очистка
rm -f "$NAME.lda" "$NAME.lst"

echo "========================================="