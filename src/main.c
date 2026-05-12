#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "loader.h"
#include "debug.h"

static int errors = 0;
static int tests = 0;

/* Вспомогательная функция для проверки */
static void check(const char *name, int condition)
{
    tests++;
    if (condition) {
        printf("  [PASS] %s\n", name);
    } else {
        printf("  [FAIL] %s\n", name);
        errors++;
    }
}

/* Вспомогательная функция: загрузить и выполнить программу */
static void run_program(PDP11 *cpu, uint16_t *program, int size)
{
    cpu_reset(cpu);
    uint16_t start = 01000;
    loader_load_words(cpu, start, program, size);
    loader_set_start(cpu, start);
    cpu->running = 1;
    while (cpu->running) {
        cpu_step(cpu);
        if (cpu->cycles > 10000) {
            printf("  [FAIL] Превышен лимит циклов\n");
            errors++;
            cpu->running = 0;
        }
    }
}

/* ============================================================
 *  Тесты двухоперандных команд
 * ============================================================ */

static void test_mov(PDP11 *cpu)
{
    printf("\n=== Тест MOV ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #100, R0 */
        0000100,
        0010001,    /* MOV R0, R1 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = 100", cpu->reg[R0] == 0100);
    check("R1 = R0",  cpu->reg[R1] == 0100);
}

static void test_add(PDP11 *cpu)
{
    printf("\n=== Тест ADD ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #30, R0 */
        0000030,
        0012701,    /* MOV #50, R1 */
        0000050,
        0060001,    /* ADD R0, R1 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 6);

    check("R1 = 30 + 50 = 100", cpu->reg[R1] == 0100);
    check("R0 не изменился", cpu->reg[R0] == 030);
}

static void test_sub(PDP11 *cpu)
{
    printf("\n=== Тест SUB ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #30, R0 */
        0000030,
        0012701,    /* MOV #100, R1 */
        0000100,
        0160001,    /* SUB R0, R1 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 6);

    check("R1 = 100 - 30 = 50", cpu->reg[R1] == 050);
}

static void test_cmp(PDP11 *cpu)
{
    printf("\n=== Тест CMP (равные) ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #42, R0 */
        0000042,
        0012701,    /* MOV #42, R1 */
        0000042,
        0020001,    /* CMP R0, R1 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 6);

    check("Z = 1 (равны)", cpu_get_flag(cpu, PSW_Z) == 1);
    check("R0 не изменился", cpu->reg[R0] == 042);
    check("R1 не изменился", cpu->reg[R1] == 042);
}

static void test_bit(PDP11 *cpu)
{
    printf("\n=== Тест BIT ===\n");

    /* Проверяем бит 3 в числе 15 (бит установлен) */
    uint16_t prog[] = {
        0012700,    /* MOV #17, R0 */
        0000017,
        0012701,    /* MOV #10, R1 */
        0000010,
        0030100,    /* BIT R1, R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 6);

    check("Z = 0 (бит установлен)", cpu_get_flag(cpu, PSW_Z) == 0);
    check("R0 не изменился", cpu->reg[R0] == 017);
    check("R1 не изменился", cpu->reg[R1] == 010);
}

static void test_bic(PDP11 *cpu)
{
    printf("\n=== Тест BIC ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #17, R0 */
        0000017,
        0012701,    /* MOV #10, R1 */
        0000010,
        0040100,    /* BIC R1, R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 6);

    check("R0 = 7 (бит 3 сброшен)", cpu->reg[R0] == 7);
    check("R1 не изменился", cpu->reg[R1] == 010);
}

static void test_bis(PDP11 *cpu)
{
    printf("\n=== Тест BIS ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #6, R0 */
        0000006,
        0012701,    /* MOV #10, R1 */
        0000010,
        0050100,    /* BIS R1, R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 6);

    check("R0 = 16 (бит 3 установлен)", cpu->reg[R0] == 016);
    check("R1 не изменился", cpu->reg[R1] == 010);
}

/* ============================================================
 *  Тесты однооперандных команд
 * ============================================================ */

static void test_clr(PDP11 *cpu)
{
    printf("\n=== Тест CLR ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #777, R0 */
        0000777,
        0005000,    /* CLR R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = 0", cpu->reg[R0] == 0);
    check("Z = 1", cpu_get_flag(cpu, PSW_Z) == 1);
}

static void test_inc(PDP11 *cpu)
{
    printf("\n=== Тест INC ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #5, R0 */
        0000005,
        0005200,    /* INC R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = 6", cpu->reg[R0] == 6);
}

static void test_dec(PDP11 *cpu)
{
    printf("\n=== Тест DEC ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #5, R0 */
        0000005,
        0005300,    /* DEC R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = 4", cpu->reg[R0] == 4);
}

static void test_neg(PDP11 *cpu)
{
    printf("\n=== Тест NEG ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #5, R0 */
        0000005,
        0005400,    /* NEG R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = -5 (177773)", cpu->reg[R0] == 0177773);
    check("N = 1", cpu_get_flag(cpu, PSW_N) == 1);
}

static void test_tst(PDP11 *cpu)
{
    printf("\n=== Тест TST ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #0, R0 */
        0000000,
        0005700,    /* TST R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("Z = 1 (ноль)", cpu_get_flag(cpu, PSW_Z) == 1);
    check("N = 0", cpu_get_flag(cpu, PSW_N) == 0);
}

static void test_com(PDP11 *cpu)
{
    printf("\n=== Тест COM ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #0, R0 */
        0000000,
        0005100,    /* COM R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = 177777", cpu->reg[R0] == 0177777);
    check("C = 1", cpu_get_flag(cpu, PSW_C) == 1);
    check("V = 0", cpu_get_flag(cpu, PSW_V) == 0);
}

static void test_adc(PDP11 *cpu)
{
    printf("\n=== Тест ADC (C=1) ===\n");

    /* Сначала установим C=1 через сложение с переполнением */
    uint16_t prog[] = {
        0012700,    /* MOV #177777, R0 */
        0177777,
        0012701,    /* MOV #1, R1 */
        0000001,
        0060001,    /* ADD R0, R1 → переполнение, C=1 */
        0012702,    /* MOV #5, R2 */
        0000005,
        0005502,    /* ADC R2 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 9);

    check("R2 = 6 (5 + C=1)", cpu->reg[R2] == 6);
}

static void test_swab(PDP11 *cpu)
{
    printf("\n=== Тест SWAB ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #1234, R0 (восьмеричное) */
        0001234,
        0000300,    /* SWAB R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    /* 001234₈ = 0x029C → SWAB → 0x9C02 = 116002₈ */
    uint16_t val = 01234;
    uint16_t expected = ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
    check("R0 байты поменялись", cpu->reg[R0] == expected);
}

/* ============================================================
 *  Тесты сдвигов
 * ============================================================ */

static void test_asl(PDP11 *cpu)
{
    printf("\n=== Тест ASL ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #5, R0 */
        0000005,
        0006300,    /* ASL R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = 12 (5*2=10=12₈)", cpu->reg[R0] == 012);
    check("C = 0", cpu_get_flag(cpu, PSW_C) == 0);
}

static void test_asl_carry(PDP11 *cpu)
{
    printf("\n=== Тест ASL с переносом ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #100000, R0 */
        0100000,
        0006300,    /* ASL R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = 0", cpu->reg[R0] == 0);
    check("C = 1 (бит 15 выдвинулся)", cpu_get_flag(cpu, PSW_C) == 1);
    check("Z = 1", cpu_get_flag(cpu, PSW_Z) == 1);
}

static void test_asr(PDP11 *cpu)
{
    printf("\n=== Тест ASR ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #12, R0 (10₁₀) */
        0000012,
        0006200,    /* ASR R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = 5 (10/2=5)", cpu->reg[R0] == 5);
    check("C = 0", cpu_get_flag(cpu, PSW_C) == 0);
}

static void test_asr_sign(PDP11 *cpu)
{
    printf("\n=== Тест ASR с отрицательным числом ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #177776, R0 (-2) */
        0177776,
        0006200,    /* ASR R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 4);

    check("R0 = 177777 (-1)", cpu->reg[R0] == 0177777);
    check("N = 1 (знак сохранился)", cpu_get_flag(cpu, PSW_N) == 1);
}

static void test_rol(PDP11 *cpu)
{
    printf("\n=== Тест ROL ===\n");

    /* Сначала сбросим C через CLR */
    uint16_t prog[] = {
        0005001,    /* CLR R1 (C=0) */
        0012700,    /* MOV #1, R0 */
        0000001,
        0006100,    /* ROL R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 5);

    check("R0 = 2 (1 сдвинут влево, C=0 вошёл)", cpu->reg[R0] == 2);
    check("C = 0", cpu_get_flag(cpu, PSW_C) == 0);
}

static void test_ror(PDP11 *cpu)
{
    printf("\n=== Тест ROR ===\n");

    uint16_t prog[] = {
        0005001,    /* CLR R1 (C=0) */
        0012700,    /* MOV #2, R0 */
        0000002,
        0006000,    /* ROR R0 */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 5);

    check("R0 = 1 (2 сдвинут вправо, C=0 вошёл)", cpu->reg[R0] == 1);
    check("C = 0", cpu_get_flag(cpu, PSW_C) == 0);
}

/* ============================================================
 *  Тесты переходов
 * ============================================================ */

static void test_br(PDP11 *cpu)
{
    printf("\n=== Тест BR ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #1, R0 */
        0000001,
        0000402,    /* BR +2 (перепрыгнуть 2 слова) */
        0012700,    /* MOV #77, R0 (не должно выполниться) */
        0000077,
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 6);

    check("R0 = 1 (MOV #77 перепрыгнут)", cpu->reg[R0] == 1);
}

static void test_bne(PDP11 *cpu)
{
    printf("\n=== Тест BNE (цикл) ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #3, R0 */
        0000003,
        0005001,    /* CLR R1 */
        0005201,    /* INC R1 */
        0005300,    /* DEC R0 */
        0001375,    /* BNE -3 (назад к INC) */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 7);

    check("R0 = 0", cpu->reg[R0] == 0);
    check("R1 = 3 (цикл 3 раза)", cpu->reg[R1] == 3);
}

static void test_beq(PDP11 *cpu)
{
    printf("\n=== Тест BEQ ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #0, R0 */
        0000000,
        0005700,    /* TST R0 (Z=1) */
        0001402,    /* BEQ +2 */
        0012701,    /* MOV #77, R1 (не должно) */
        0000077,
        0012701,    /* MOV #1, R1 (должно) */
        0000001,
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 9);

    check("R1 = 1 (BEQ сработал)", cpu->reg[R1] == 1);
}

static void test_bpl_bmi(PDP11 *cpu)
{
    printf("\n=== Тест BPL/BMI ===\n");

    /* Тест BPL: положительное число */
    uint16_t prog[] = {
        0012700,    /* MOV #5, R0 */
        0000005,
        0005700,    /* TST R0 */
        0100002,    /* BPL +2 */
        0012701,    /* MOV #0, R1 (не должно) */
        0000000,
        0012701,    /* MOV #1, R1 (должно) */
        0000001,
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 9);

    check("R1 = 1 (BPL сработал для положительного)", cpu->reg[R1] == 1);
}

static void test_bge_blt(PDP11 *cpu)
{
    printf("\n=== Тест BGE/BLT ===\n");

    /* CMP 5, 3 → 5-3=2 → N=0,V=0 → N^V=0 → BGE прыгает */
    uint16_t prog[] = {
        0012700,    /* MOV #5, R0 */
        0000005,
        0012701,    /* MOV #3, R1 */
        0000003,
        0020001,    /* CMP R0, R1 */
        0002002,    /* BGE +2 */
        0012702,    /* MOV #0, R2 (не должно) */
        0000000,
        0012702,    /* MOV #1, R2 (должно) */
        0000001,
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 11);

    check("R2 = 1 (BGE: 5 >= 3)", cpu->reg[R2] == 1);
}

static void test_bgt_ble(PDP11 *cpu)
{
    printf("\n=== Тест BGT (равные числа) ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #5, R0 */
        0000005,
        0012701,    /* MOV #5, R1 */
        0000005,
        0020001,    /* CMP R0, R1 */
        0003003,    /* BGT +3 (перепрыгнуть MOV #0 + HALT) */
        0012702,    /* MOV #0, R2 (BGT не прыгнул → выполнится) */
        0000000,
        0000000,    /* HALT */
        0012702,    /* MOV #1, R2 (не должно) */
        0000001,
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 12);

    check("R2 = 0 (BGT: 5 не > 5)", cpu->reg[R2] == 0);
}

static void test_bhi_blos(PDP11 *cpu)
{
    printf("\n=== Тест BHI ===\n");

    /* CMP 10, 5 → беззнаковое 10 > 5 → BHI прыгает */
    uint16_t prog[] = {
        0012700,    /* MOV #10, R0 */
        0000010,
        0012701,    /* MOV #5, R1 */
        0000005,
        0020001,    /* CMP R0, R1 */
        0101002,    /* BHI +2 */
        0012702,    /* MOV #0, R2 (не должно) */
        0000000,
        0012702,    /* MOV #1, R2 (должно) */
        0000001,
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 11);

    check("R2 = 1 (BHI: 10 > 5)", cpu->reg[R2] == 1);
}

static void test_bvc_bvs(PDP11 *cpu)
{
    printf("\n=== Тест BVC (нет переполнения) ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #5, R0 */
        0000005,
        0012701,    /* MOV #3, R1 */
        0000003,
        0060001,    /* ADD R0, R1 (нет переполнения) */
        0102002,    /* BVC +2 */
        0012702,    /* MOV #0, R2 (не должно) */
        0000000,
        0012702,    /* MOV #1, R2 (должно) */
        0000001,
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 11);

    check("R2 = 1 (BVC: нет переполнения)", cpu->reg[R2] == 1);
}

static void test_bcc_bcs(PDP11 *cpu)
{
    printf("\n=== Тест BCC (нет переноса) ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #5, R0 */
        0000005,
        0012701,    /* MOV #3, R1 */
        0000003,
        0060001,    /* ADD R0, R1 (нет переноса) */
        0103002,    /* BCC +2 */
        0012702,    /* MOV #0, R2 (не должно) */
        0000000,
        0012702,    /* MOV #1, R2 (должно) */
        0000001,
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 11);

    check("R2 = 1 (BCC: нет переноса)", cpu->reg[R2] == 1);
}

/* ============================================================
 *  Тесты JSR/RTS
 * ============================================================ */

static void test_jsr_rts(PDP11 *cpu)
{
    printf("\n=== Тест JSR/RTS ===\n");

    uint16_t prog[] = {
        0012701,    /* MOV #0, R1 */
        0000000,
        0004767,    /* JSR PC, +2 */
        0000002,
        0000000,    /* HALT */
        0012701,    /* sub: MOV #42, R1 */
        0000042,
        0000207,    /* RTS PC */
    };
    run_program(cpu, prog, 8);

    check("R1 = 42 (подпрограмма выполнилась)", cpu->reg[R1] == 042);
}

/* ============================================================
 *  Тесты JMP и NOP
 * ============================================================ */

static void test_jmp(PDP11 *cpu)
{
    printf("\n=== Тест JMP ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #1, R0 */
        0000001,
        0000167,    /* JMP +4 (перепрыгнуть через MOV #77) */
        0000004,
        0012700,    /* MOV #77, R0 (не должно выполниться) */
        0000077,
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 7);

    check("R0 = 1 (JMP перепрыгнул)", cpu->reg[R0] == 1);
}

static void test_nop(PDP11 *cpu)
{
    printf("\n=== Тест NOP ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #42, R0 */
        0000042,
        0000240,    /* NOP */
        0000240,    /* NOP */
        0000240,    /* NOP */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 6);

    check("R0 = 42 (NOP ничего не сломал)", cpu->reg[R0] == 042);
}

/* ============================================================
 *  Тест цикла (интеграционный)
 * ============================================================ */

static void test_sum_loop(PDP11 *cpu)
{
    printf("\n=== Тест: сумма 1..8 ===\n");

    uint16_t prog[] = {
        0012700,    /* MOV #10, R0 */
        0000010,
        0005001,    /* CLR R1 */
        0060001,    /* ADD R0, R1 */
        0005300,    /* DEC R0 */
        0001375,    /* BNE -3 (назад к ADD) */
        0000000,    /* HALT */
    };
    run_program(cpu, prog, 7);

    check("R0 = 0", cpu->reg[R0] == 0);
    check("R1 = 44 (36₁₀)", cpu->reg[R1] == 044);
    check("Z = 1 (R0 стал 0)", cpu_get_flag(cpu, PSW_Z) == 1);
}

/* ============================================================
 *  Запуск всех тестов
 * ============================================================ */
#include <stdio.h>
#include "types.h"
#include "cpu.h"
#include "loader.h"
#include "debug.h"
#include "memory.h"

int main(void)
{
    PDP11 cpu;
    cpu_init(&cpu);

    uint16_t program[] = {
        /* 00 */ 0012705,    /* MOV #array, R5 */
        /* 01 */ 0001054,    /* адрес array */

        /* outer: */
        /* 02 */ 0005004,    /* CLR R4 */
        /* 03 */ 0012703,    /* MOV #7, R3 */
        /* 04 */ 0000007,
        /* 05 */ 0010500,    /* MOV R5, R0 */

        /* inner: */
        /* 06 */ 0011001,    /* MOV (R0), R1 */
        /* 07 */ 0016002,    /* MOV 2(R0), R2 */
        /* 08 */ 0000002,
        /* 09 */ 0020102,    /* CMP R1, R2 */
        /* 10 */ 0003405,    /* BLE +5 (noswap) */

        /* swap: */
        /* 11 */ 0010210,    /* MOV R2, (R0) */
        /* 12 */ 0010160,    /* MOV R1, 2(R0) */
        /* 13 */ 0000002,
        /* 14 */ 0012704,    /* MOV #1, R4 */
        /* 15 */ 0000001,

        /* noswap: */
        /* 16 */ 0062700,    /* ADD #2, R0 */
        /* 17 */ 0000002,
        /* 18 */ 0077315,    /* SOB R3, 13 (inner) */

        /* 19 */ 0005704,    /* TST R4 */
        /* 20 */ 0001355,    /* BNE outer */
        /* 21 */ 0000000,    /* HALT */

        /* array: */
        /* 22 */ 0000007,    /* 7 */
        /* 23 */ 0000003,    /* 3 */
        /* 24 */ 0000011,    /* 9 */
        /* 25 */ 0000001,    /* 1 */
        /* 26 */ 0000005,    /* 5 */
        /* 27 */ 0000010,    /* 8 */
        /* 28 */ 0000002,    /* 2 */
        /* 29 */ 0000004,    /* 4 */
    };

    int prog_size = sizeof(program) / sizeof(program[0]);
    uint16_t start = 01000;

    loader_load_words(&cpu, start, program, prog_size);
    loader_set_start(&cpu, start);

    /* Массив до сортировки */
    printf("=== Bubble Sort на PDP-11 ===\n\n");
    printf("Массив до сортировки:\n");
    uint16_t array_addr = 01054;
    for (int i = 0; i < 8; i++) {
        printf("  [%d] = %d\n", i, mem_read_word(&cpu, array_addr + i * 2));
    }

    /* Выполнение с трассировкой */
    printf("\nВыполнение...\n\n");
    cpu.running = 1;
    while (cpu.running) {
        debug_trace(&cpu, cpu.reg[PC]);
        cpu_step(&cpu);
        if (cpu.cycles > 100000) {
            printf("Превышен лимит циклов!\n");
            cpu.running = 0;
        }
    }

    /* Массив после сортировки */
    printf("\nМассив после сортировки:\n");
    int sorted = 1;
    int expected[] = {1, 2, 3, 4, 5, 7, 8, 9};
    for (int i = 0; i < 8; i++) {
        uint16_t val = mem_read_word(&cpu, array_addr + i * 2);
        printf("  [%d] = %d", i, val);
        if (val == expected[i]) {
            printf("  ✓\n");
        } else {
            printf("  ✗ (ожидалось %d)\n", expected[i]);
            sorted = 0;
        }
    }

    printf("\nИнструкций выполнено: %llu\n",
           (unsigned long long)cpu.cycles);
    debug_dump_regs(&cpu);

    printf("\n========================================\n");
    if (sorted) {
        printf("   СОРТИРОВКА УСПЕШНА!\n");
    } else {
        printf("   ОШИБКА СОРТИРОВКИ!\n");
    }
    printf("========================================\n");

    return sorted ? 0 : 1;
}