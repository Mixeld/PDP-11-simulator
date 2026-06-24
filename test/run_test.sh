#!/bin/bash

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo "========================================="
echo "ЗАПУСК ТЕСТОВ С ПРОВЕРКОЙ РЕЗУЛЬТАТОВ"
echo "========================================="

if [ ! -f "../assembler/asm" ]; then
    echo -e "${RED}ОШИБКА: Ассемблер не найден${NC}"
    echo "Сначала собери: cd ../assembler && make"
    exit 1
fi

if [ ! -f "../simulator/pdp11" ]; then
    echo -e "${RED}ОШИБКА: Симулятор не найден${NC}"
    echo "Сначала собери: cd ../simulator && make"
    exit 1
fi

PASSED=0
FAILED=0

# Функция для запуска теста и проверки результата
run_test() {
    local test_name=$1
    local expected_r0=$2
    local description=$3
    
    echo ""
    echo -e "${BLUE}[$test_name]${NC} - $description"
    
    # Ассемблирование
    ../assembler/asm "asm_programs/$test_name.asm" -o "$test_name" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "  ${RED}FAILED: Ассемблирование не удалось${NC}"
        FAILED=$((FAILED+1))
        return
    fi
    
    # Запуск симулятора и захват вывода
    output=$(../simulator/pdp11 "$test_name.lda" 2>&1)
    
    # Извлечение R0 из вывода (новый формат)
    r0=$(echo "$output" | grep "RESULT_R0=" | head -1 | sed 's/.*RESULT_R0=//' | tr -d ' ')
    
    # Если не нашли новый формат, пробуем старый
    if [ -z "$r0" ]; then
        r0=$(echo "$output" | grep "R0:" | head -1 | sed 's/.*R0: \([0-9]*\).*/\1/')
    fi
    
    # Очистка от ведущих нулей для сравнения
    r0_clean=$(echo "$r0" | sed 's/^0*//')
    expected_clean=$(echo "$expected_r0" | sed 's/^0*//')
    
    # Сравнение с ожидаемым
    if [ "$r0_clean" = "$expected_clean" ]; then
        echo -e "  ${GREEN}PASSED${NC} R0=$r0 (ожидалось $expected_r0)"
        PASSED=$((PASSED+1))
    else
        echo -e "  ${RED}FAILED${NC} R0=$r0 (ожидалось $expected_r0)"
        # Вывод отладочной информации
        echo "$output" | grep -E "(RESULT_|R[0-9]:)" | head -5
        FAILED=$((FAILED+1))
    fi
    
    # Очистка
    rm -f "$test_name.lda" "$test_name.lst"
}

# Запуск тестов с ожидаемыми значениями
run_test "test1_mov"   "123"     "MOV #123, R0 -> R0=123"
run_test "test2_add"   "110"     "100+30-20=110"
run_test "test3_sob"   "44"      "Сумма 1..8=36 (44 восьм.)"
run_test "test4_jsr"   "24"      "12*2=24"
run_test "test5_stack" "111"     "Push/pop -> R0=111"
run_test "test6_bubble" "1"      "Сортировка выполнена"
run_test "test8_cmp"   "77"      "CMP и переходы -> R0=77"

echo ""
echo "========================================="
echo -e "Пройдено: ${GREEN}$PASSED${NC}"
echo -e "Провалено: ${RED}$FAILED${NC}"
echo "========================================="

exit $FAILED