#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "loader.h"
#include "debug.h"

int main(void)
{
    PDP11 cpu;
    cpu_init(&cpu);

    /*
     * Тест BIC — три проверки:
     *
     * Тест 1: Сбросить бит 3 в числе 15
     *   MOV  #17, R0       ; R0 = 17₈ = 15₁₀ = 1111₂
     *   MOV  #10, R1       ; R1 = 10₈ = 8₁₀  = 1000₂ (маска бита 3)
     *   BIC  R1, R0        ; R0 = 1111 AND NOT(1000) = 1111 AND 0111 = 0111 = 7
     *
     * Тест 2: Сбросить младшие 4 бита
     *   MOV  #377, R2      ; R2 = 377₈ = 255₁₀ = 1111 1111₂
     *   MOV  #17, R3       ; R3 = 17₈ = 15₁₀   = 0000 1111₂
     *   BIC  R3, R2        ; R2 = 1111 1111 AND NOT(0000 1111)
     *                      ;    = 1111 1111 AND 1111 0000
     *                      ;    = 1111 0000 = 360₈ = 240₁₀
     *
     * Тест 3: Сбросить всё — результат 0 (проверка флага Z)
     *   MOV  #377, R4      ; R4 = 377₈ = 255₁₀
     *   MOV  #377, R5      ; R5 = 377₈ = 255₁₀
     *   BIC  R5, R4        ; R4 = 255 AND NOT(255) = 255 AND 0 = 0
     *                      ; Z должен стать 1
     *   HALT
     */

    uint16_t program[] = {
        /* Тест 1: сбросить бит 3 */
        0012700,        /* MOV #17, R0 */
        0000017,        /* число 17₈ = 15₁₀ */
        0012701,        /* MOV #10, R1 */
        0000010,        /* число 10₈ = 8₁₀ */
        0040100,        /* BIC R1, R0 */

        /* Тест 2: сбросить младшие 4 бита */
        0012702,        /* MOV #377, R2 */
        0000377,        /* число 377₈ = 255₁₀ */
        0012703,        /* MOV #17, R3 */
        0000017,        /* число 17₈ = 15₁₀ */
        0040302,        /* BIC R3, R2 */

        /* Тест 3: сбросить всё (результат = 0) */
        0012704,        /* MOV #377, R4 */
        0000377,        /* число 377₈ = 255₁₀ */
        0012705,        /* MOV #377, R5 */
        0000377,        /* число 377₈ = 255₁₀ */
        0040504,        /* BIC R5, R4 */

        0000000,        /* HALT */
    };

    int prog_size = sizeof(program) / sizeof(program[0]);

    uint16_t start = 01000;
    loader_load_words(&cpu, start, program, prog_size);
    loader_set_start(&cpu, start);

    printf("=== Начальное состояние ===\n");
    debug_dump_regs(&cpu);
    printf("\n=== Выполнение ===\n\n");

    cpu_run(&cpu);

    /* Проверяем результаты */
    printf("\n=== Проверка результатов ===\n\n");

    int errors = 0;

    /* Тест 1 */
    printf("Тест 1: BIC сброс бита 3\n");
    printf("  R0 = %06o (ожидается 000007)\n", cpu.reg[R0]);
    if (cpu.reg[R0] == 7) {
        printf("  PASSED\n\n");
    } else {
        printf("  FAILED!\n\n");
        errors++;
    }

    /* Тест 2 */
    printf("Тест 2: BIC сброс младших 4 бит\n");
    printf("  R2 = %06o (ожидается 000360)\n", cpu.reg[R2]);
    if (cpu.reg[R2] == 0360) {
        printf("  PASSED\n\n");
    } else {
        printf("  FAILED!\n\n");
        errors++;
    }

    /* Тест 3 */
    printf("Тест 3: BIC сброс всех бит (результат = 0)\n");
    printf("  R4 = %06o (ожидается 000000)\n", cpu.reg[R4]);
    printf("  Z  = %d    (ожидается 1)\n", cpu_get_flag(&cpu, PSW_Z));
    printf("  V  = %d    (ожидается 0)\n", cpu_get_flag(&cpu, PSW_V));
    if (cpu.reg[R4] == 0 &&
        cpu_get_flag(&cpu, PSW_Z) == 1 &&
        cpu_get_flag(&cpu, PSW_V) == 0) {
        printf("  PASSED\n\n");
    } else {
        printf("  FAILED!\n\n");
        errors++;
    }

    /* Проверяем что src не изменился */
    printf("Тест 4: src не изменился\n");
    printf("  R1 = %06o (ожидается 000010)\n", cpu.reg[R1]);
    printf("  R3 = %06o (ожидается 000017)\n", cpu.reg[R3]);
    printf("  R5 = %06o (ожидается 000377)\n", cpu.reg[R5]);
    if (cpu.reg[R1] == 010 &&
        cpu.reg[R3] == 017 &&
        cpu.reg[R5] == 0377) {
        printf("  PASSED\n\n");
    } else {
        printf("  FAILED!\n\n");
        errors++;
    }

    /* Итог */
    printf("=== Итого: ");
    if (errors == 0) {
        printf("Все тесты пройдены! ===\n");
    } else {
        printf("%d тестов провалено ===\n", errors);
    }

    return errors;
}