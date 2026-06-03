#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "========================================="
echo "ЗАПУСК ТЕСТОВ"
echo "========================================="

if [ ! -f "../assembler/asm" ]; then
    echo -e "${RED}ОШИБКА: Ассемблер не найден${NC}"
    exit 1
fi

if [ ! -f "../simulator/pdp11" ]; then
    echo -e "${RED}ОШИБКА: Симулятор не найден${NC}"
    exit 1
fi

PASSED=0
FAILED=0

for asm in asm_programs/test*.asm; do
    if [ ! -f "$asm" ]; then
        continue
    fi
    
    NAME=$(basename "$asm" .asm)
    echo ""
    echo -e "${BLUE}[$NAME]${NC}"
    
    ../assembler/asm "$asm" -o "$NAME" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo -e "  ${RED}FAILED: Ассемблирование${NC}"
        FAILED=$((FAILED+1))
        continue
    fi
    
    ../simulator/pdp11 "$NAME.lda" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "  ${RED}FAILED: Выполнение${NC}"
        FAILED=$((FAILED+1))
        continue
    fi
    
    echo -e "  ${GREEN}PASSED${NC}"
    PASSED=$((PASSED+1))
done

echo ""
echo "========================================="
echo "Пройдено: $PASSED"
echo "Провалено: $FAILED"
echo "========================================="

rm -f *.lda *.lst

exit $FAILED