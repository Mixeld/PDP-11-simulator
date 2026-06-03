#include <stdio.h>
#include "debug.h"
#include "memory.h"
#include "disasm.h"

void debug_dump_regs(PDP11 *cpu)
{
    printf("=== Регистры ===\n");
    for (int i = 0; i < 8; i++) {
        const char *name;
        switch (i) {
        case 6:  name = "SP"; break;
        case 7:  name = "PC"; break;
        default: name = "  "; break;
        }
        printf("  R%d (%s): %06o (%5u)\n",
               i, name, cpu->reg[i], cpu->reg[i]);
    }
    printf("  PSW: %06o  [N=%d Z=%d V=%d C=%d]\n",
           cpu->psw,
           (cpu->psw >> 3) & 1,
           (cpu->psw >> 2) & 1,
           (cpu->psw >> 1) & 1,
           cpu->psw & 1);
    printf("  Cycles: %llu\n", (unsigned long long)cpu->cycles);
}

void debug_dump_mem(PDP11 *cpu, uint16_t start, int count)
{
    printf("=== Память [%06o..%06o] ===\n",
           start, (uint16_t)(start + count * 2 - 2));
    for (int i = 0; i < count; i++) {
        uint16_t addr = start + i * 2;
        uint16_t val = mem_read_word(cpu, addr);
        printf("  %06o: %06o\n", addr, val);
    }
}

void debug_trace(PDP11 *cpu, uint16_t pc)
{
    char buf[64];
    disassemble(cpu, pc, buf, sizeof(buf));
    printf("[%06o] %-25s R0=%06o R1=%06o R2=%06o R3=%06o R4=%06o SP=%06o\n",
           pc, buf,
           cpu->reg[0], cpu->reg[1], cpu->reg[2],
           cpu->reg[3], cpu->reg[4], cpu->reg[SP]);
}