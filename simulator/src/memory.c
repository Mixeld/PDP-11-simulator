#include <stdio.h>
#include "memory.h"

uint8_t mem_read_byte(PDP11 *cpu, uint16_t addr)
{
    return cpu->memory[addr];
}

uint16_t mem_read_word(PDP11 *cpu, uint16_t addr)
{
    uint16_t lo = cpu->memory[addr];
    uint16_t hi = cpu->memory[addr + 1];
    return (hi << 8) | lo;
}

void mem_write_byte(PDP11 *cpu, uint16_t addr, uint8_t val)
{
    cpu->memory[addr] = val;
}

void mem_write_word(PDP11 *cpu, uint16_t addr, uint16_t val)
{
    cpu->memory[addr]     = val & 0xFF;
    cpu->memory[addr + 1] = (val >> 8) & 0xFF;
}