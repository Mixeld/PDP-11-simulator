#include <stdio.h>
#include "decode.h"
#include "memory.h"

static uint16_t resolve_addr(PDP11 *cpu, int mode, int reg)
{
    uint16_t addr = 0;
    uint16_t index;

    switch (mode) {
    case 0:
        break;
    case 1:
        addr = cpu->reg[reg];
        break;
    case 2:
        addr = cpu->reg[reg];
        cpu->reg[reg] += 2;
        break;
    case 3:
        addr = cpu->reg[reg];
        cpu->reg[reg] += 2;
        addr = mem_read_word(cpu, addr);
        break;
    case 4:
        cpu->reg[reg] -= 2;
        addr = cpu->reg[reg];
        break;
    case 5:
        cpu->reg[reg] -= 2;
        addr = cpu->reg[reg];
        addr = mem_read_word(cpu, addr);
        break;
    case 6:
        index = mem_read_word(cpu, cpu->reg[PC]);
        cpu->reg[PC] += 2;
        addr = cpu->reg[reg] + index;
        break;
    case 7:
        index = mem_read_word(cpu, cpu->reg[PC]);
        cpu->reg[PC] += 2;
        addr = cpu->reg[reg] + index;
        addr = mem_read_word(cpu, addr);
        break;
    }

    return addr;
}

uint16_t fetch_operand(PDP11 *cpu, int mode, int reg)
{
    if (mode == 0)
        return cpu->reg[reg];

    uint16_t addr = resolve_addr(cpu, mode, reg);
    return mem_read_word(cpu, addr);
}

uint16_t resolve_dst_addr(PDP11 *cpu, int mode, int reg)
{
    return resolve_addr(cpu, mode, reg);
}

void store_operand(PDP11 *cpu, int mode, int reg, uint16_t val)
{
    if (mode == 0) {
        cpu->reg[reg] = val;
        return;
    }
    uint16_t addr = resolve_addr(cpu, mode, reg);
    mem_write_word(cpu, addr, val);
}