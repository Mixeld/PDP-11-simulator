#ifndef PDP11_DEBUG_H
#define PDP11_DEBUG_H

#include "types.h"

void debug_dump_regs(PDP11 *cpu);
void debug_dump_mem(PDP11 *cpu, uint16_t start, int count);
void debug_trace(PDP11 *cpu, uint16_t pc);

#endif