#include <stdio.h>
#include "disasm.h"
#include "memory.h"


/* ============================================================
 *  Вспомогательная функция: декодирование одного операнда
 * ============================================================
 *
 * mode — режим адресации (0-7)
 * reg  — номер регистра (0-7)
 * addr — адрес в памяти где может лежать дополнительное слово
 * buf  — куда записать текст
 *
 * Возвращает количество дополнительных слов (0 или 1)
 */

#include <stdio.h>
#include "disasm.h"
#include "memory.h"

/*
 * decode_operand — превращает числовой код операнда (mode + reg)
 * в читаемый текст.
 *
 * Параметры:
 *   cpu     — указатель на процессор (для чтения памяти)
 *   addr    — адрес где может лежать дополнительное слово
 *   mode    — режим адресации (0-7)
 *   reg     — номер регистра (0-7)
 *   buf     — буфер для текстового результата
 *   bufsize — размер буфера
 *
 * Возвращает: количество дополнительных слов (0 или 1)
 *
 * Примеры:
 *   mode=0, reg=1  →  "R1"
 *   mode=2, reg=7  →  "#10"     (непосредственное значение)
 *   mode=1, reg=0  →  "(R0)"
 *   mode=6, reg=0  →  "2(R0)"   (индексный режим)
 */
static int decode_operand(PDP11 *cpu, uint16_t addr, int mode, int reg,
                          char *buf, int bufsize)
{
    switch (mode) {

    /* Режим 0: Регистровый — значение прямо в регистре */
    case 0:
        if (reg == 6)
            snprintf(buf, bufsize, "SP");
        else if (reg == 7)
            snprintf(buf, bufsize, "PC");
        else
            snprintf(buf, bufsize, "R%d", reg);
        return 0;

    /* Режим 1: Косвенный — значение в памяти по адресу из регистра */
    case 1:
        if (reg == 6)
            snprintf(buf, bufsize, "(SP)");
        else if (reg == 7)
            snprintf(buf, bufsize, "(PC)");
        else
            snprintf(buf, bufsize, "(R%d)", reg);
        return 0;

    /* Режим 2: Автоинкремент — (Rn)+ или #N (когда reg=7) */
    case 2:
        if (reg == 7) {
            /* Непосредственное значение: число лежит после инструкции */
            uint16_t val = mem_read_word(cpu, addr);
            snprintf(buf, bufsize, "#%o", val);
        } else if (reg == 6) {
            snprintf(buf, bufsize, "(SP)+");
        } else {
            snprintf(buf, bufsize, "(R%d)+", reg);
        }
        return 1;

    /* Режим 3: Автоинкремент косвенный — @(Rn)+ или @#N */
    case 3:
        if (reg == 7) {
            uint16_t val = mem_read_word(cpu, addr);
            snprintf(buf, bufsize, "@#%o", val);
        } else {
            snprintf(buf, bufsize, "@(R%d)+", reg);
        }
        return 1;

    /* Режим 4: Автодекремент — -(Rn) */
    case 4:
        if (reg == 6)
            snprintf(buf, bufsize, "-(SP)");
        else
            snprintf(buf, bufsize, "-(R%d)", reg);
        return 0;

    /* Режим 5: Автодекремент косвенный — @-(Rn) */
    case 5:
        snprintf(buf, bufsize, "@-(R%d)", reg);
        return 0;

    /* Режим 6: Индексный — X(Rn) или относительный адрес (когда reg=7) */
    case 6:
        {
            uint16_t offset = mem_read_word(cpu, addr);
            if (reg == 7) {
                /* Относительная адресация через PC:
                 * целевой адрес = addr + 2 + offset
                 * (addr указывает на слово со смещением,
                 *  addr+2 — это PC после чтения смещения) */
                uint16_t target = addr + 2 + offset;
                snprintf(buf, bufsize, "%o", target);
            } else if (reg == 6) {
                snprintf(buf, bufsize, "%o(SP)", offset);
            } else {
                snprintf(buf, bufsize, "%o(R%d)", offset, reg);
            }
        }
        return 1;

    /* Режим 7: Индексный косвенный — @X(Rn) */
    case 7:
        {
            uint16_t offset = mem_read_word(cpu, addr);
            if (reg == 7) {
                uint16_t target = addr + 2 + offset;
                snprintf(buf, bufsize, "@%o", target);
            } else {
                snprintf(buf, bufsize, "@%o(R%d)", offset, reg);
            }
        }
        return 1;
    }

    snprintf(buf, bufsize, "???");
    return 0;
}

// Основная реализация дизасемблера1

int disassemble(PDP11 *cpu, uint16_t addr, char *buf, int bufsize)
{
    uint16_t instr = mem_read_word(cpu, addr);
    uint16_t opcode;
    int size = 2;       /* минимальный размер — одно слово (2 байта) */
    char src_buf[32];   /* текст источника */
    char dst_buf[32];   /* текст назначения */
    int extra;          /* сколько дополнительных слов прочитано */

    /* ================================================================
     *  Фиксированные коды — конкретные числа
     * ================================================================ */

    /* HALT = 000000 */
    if (instr == 0000000) {
        snprintf(buf, bufsize, "HALT");
        return 2;
    }

    /* RTI = 000002 */
    if (instr == 0000002) {
        snprintf(buf, bufsize, "RTI");
        return 2;
    }

    /* ================================================================
     *  Управление флагами и NOP: коды 000240 — 000277
     *  NOP = 000240 (ни один бит не выбран — ничего не происходит)
     *  CLC = 000241, SEC = 000261 и т.д.
     * ================================================================ */

    if (instr >= 0000240 && instr <= 0000277) {
        if (instr == 0000240)
            snprintf(buf, bufsize, "NOP");
        else if (instr == 0000257)
            snprintf(buf, bufsize, "CCC");
        else if (instr == 0000277)
            snprintf(buf, bufsize, "SCC");
        else if (instr == 0000241)
            snprintf(buf, bufsize, "CLC");
        else if (instr == 0000242)
            snprintf(buf, bufsize, "CLV");
        else if (instr == 0000244)
            snprintf(buf, bufsize, "CLZ");
        else if (instr == 0000250)
            snprintf(buf, bufsize, "CLN");
        else if (instr == 0000261)
            snprintf(buf, bufsize, "SEC");
        else if (instr == 0000262)
            snprintf(buf, bufsize, "SEV");
        else if (instr == 0000264)
            snprintf(buf, bufsize, "SEZ");
        else if (instr == 0000270)
            snprintf(buf, bufsize, "SEN");
        else
            snprintf(buf, bufsize, "FLAGS %06o", instr);
        return 2;
    }

    /* ================================================================
     *  RTS — возврат из подпрограммы
     *  Формат: 000200 + reg (3 бита)
     * ================================================================ */

    if ((instr & 0xFFF8) == 000200) {
        int reg = instr & 7;
        if (reg == 7)
            snprintf(buf, bufsize, "RTS PC");
        else
            snprintf(buf, bufsize, "RTS R%d", reg);
        return 2;
    }

    /* ================================================================
     *  Двухоперандные инструкции
     *  Формат: OP SS DD (биты 15-12 = opcode)
     *  Примеры: MOV R0, R1 / ADD #5, R2 / CMP (R0), R1
     * ================================================================ */

    opcode = (instr >> 12) & 0xF;
    {
        const char *name = NULL;
        switch (opcode) {
        case 001: name = "MOV"; break;
        case 002: name = "CMP"; break;
        case 003: name = "BIT"; break;
        case 004: name = "BIC"; break;
        case 005: name = "BIS"; break;
        case 006: name = "ADD"; break;
        case 016: name = "SUB"; break;
        }

        if (name) {
            /* Извлекаем поля источника и назначения */
            int sm = (instr >> 9) & 7;
            int sr = (instr >> 6) & 7;
            int dm = (instr >> 3) & 7;
            int dr = instr & 7;

            /* Декодируем источник (может прочитать доп. слово) */
            extra = decode_operand(cpu, addr + size, sm, sr, src_buf, 32);
            size += extra * 2;

            /* Декодируем назначение (может прочитать доп. слово) */
            extra = decode_operand(cpu, addr + size, dm, dr, dst_buf, 32);
            size += extra * 2;

            snprintf(buf, bufsize, "%s %s, %s", name, src_buf, dst_buf);
            return size;
        }
    }

    /* ================================================================
     *  Однооперандные инструкции
     *  Формат: OPOP DD (биты 15-6 = opcode)
     *  Примеры: CLR R0 / INC (R1) / ASL R2
     * ================================================================ */

    opcode = (instr >> 6) & 0x3FF;
    {
        const char *name = NULL;
        switch (opcode) {
        case 00001: name = "JMP";  break;
        case 00003: name = "SWAB"; break;
        case 00050: name = "CLR";  break;
        case 00051: name = "COM";  break;
        case 00052: name = "INC";  break;
        case 00053: name = "DEC";  break;
        case 00054: name = "NEG";  break;
        case 00055: name = "ADC";  break;
        case 00056: name = "SBC";  break;
        case 00057: name = "TST";  break;
        case 00060: name = "ROR";  break;
        case 00061: name = "ROL";  break;
        case 00062: name = "ASR";  break;
        case 00063: name = "ASL";  break;
        }

        if (name) {
            int dm = (instr >> 3) & 7;
            int dr = instr & 7;

            extra = decode_operand(cpu, addr + 2, dm, dr, dst_buf, 32);
            size += extra * 2;

            snprintf(buf, bufsize, "%s %s", name, dst_buf);
            return size;
        }
    }

    /* ================================================================
     *  SOB — Subtract One and Branch
     *  Формат: 077 RRR OOOOOO (биты 15-9 = 0077)
     *  Уменьшает регистр на 1, если не ноль — прыгает назад
     * ================================================================ */

    if (((instr >> 9) & 0177) == 0077) {
        int reg = (instr >> 6) & 7;
        int offset = instr & 077;
        /* SOB прыгает только назад: PC = PC - offset*2 */
        uint16_t target = addr + 2 - offset * 2;
        snprintf(buf, bufsize, "SOB R%d, %o", reg, target);
        return 2;
    }

    /* ================================================================
     *  JSR — вызов подпрограммы
     *  Формат: 004 RRR MMMDDD
     *  Проверяем: (instr & 0xFE00) == 0x0800
     * ================================================================ */

    if ((instr & 0xFE00) == 0x0800) {
        int linkr = (instr >> 6) & 7;
        int dm = (instr >> 3) & 7;
        int dr = instr & 7;

        extra = decode_operand(cpu, addr + 2, dm, dr, dst_buf, 32);
        size += extra * 2;

        if (linkr == 7)
            snprintf(buf, bufsize, "JSR PC, %s", dst_buf);
        else
            snprintf(buf, bufsize, "JSR R%d, %s", linkr, dst_buf);
        return size;
    }

    /* ================================================================
     *  Переходы (Branch)
     *  Формат: OOOOOOOO XXXXXXXX (биты 15-8 = opcode, биты 7-0 = смещение)
     *
     *  opcode вычисляется как instr >> 8
     *  Смещение — знаковое 8-битное число
     *  Целевой адрес = addr + 2 + смещение * 2
     *
     *  ВАЖНО: opcodes здесь должны совпадать с execute()!
     * ================================================================ */

    opcode = instr >> 8;
    {
        const char *name = NULL;
        switch (opcode) {
        /* Простые переходы */
        case 0001: name = "BR";   break;   /* безусловный */
        case 0002: name = "BNE";  break;   /* Z == 0 */
        case 0003: name = "BEQ";  break;   /* Z == 1 */

        /* Знаковые сравнения */
        case 0004: name = "BGE";  break;   /* N^V == 0 */
        case 0005: name = "BLT";  break;   /* N^V == 1 */
        case 0006: name = "BGT";  break;   /* N^V==0 && Z==0 */
        case 0007: name = "BLE";  break;   /* N^V==1 || Z==1 */

        /* Проверка знака */
        case 0200: name = "BPL";  break;   /* N == 0 */
        case 0201: name = "BMI";  break;   /* N == 1 */

        /* Беззнаковые сравнения */
        case 0202: name = "BHI";  break;   /* C==0 && Z==0 */
        case 0203: name = "BLOS"; break;   /* C==1 || Z==1 */

        /* Проверка переполнения */
        case 0204: name = "BVC";  break;   /* V == 0 */
        case 0205: name = "BVS";  break;   /* V == 1 */

        /* Проверка переноса */
        case 0206: name = "BCC";  break;   /* C == 0 */
        case 0207: name = "BCS";  break;   /* C == 1 */
        }

        if (name) {
            /* Вычисляем целевой адрес перехода */
            int8_t offset = (int8_t)(instr & 0xFF);
            uint16_t target = addr + 2 + offset * 2;
            snprintf(buf, bufsize, "%s %o", name, target);
            return 2;
        }

        /* EMT и TRAP — тоже в этом диапазоне opcodes */
        if (opcode == 0210) {
            snprintf(buf, bufsize, "EMT %o", instr & 0xFF);
            return 2;
        }

        if (opcode == 0211) {
            snprintf(buf, bufsize, "TRAP %o", instr & 0xFF);
            return 2;
        }
    }
    snprintf(buf, bufsize, "??? %06o", instr);
    return 2;
}