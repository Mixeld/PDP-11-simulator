#!/bin/bash

echo "========================================="
echo "ЗАПУСК ПОЛНОЙ ВАЛИДАЦИИ СИМУЛЯТОРА"
echo "========================================="

# Компиляция тестового валидатора
gcc -o validate_sim validate_sim.c

if [ $? -ne 0 ]; then
    echo "ОШИБКА: Не удалось скомпилировать валидатор"
    exit 1
fi

# Запуск валидации
./validate_sim
RESULT=$?

rm -f validate_sim

exit $RESULT