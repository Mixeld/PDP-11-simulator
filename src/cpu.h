#ifndef CPU_H
#define CPU_H

#include "types.h"

void cpu_init(PDP11 *cpu);
void cpu_reset(PDP11 *cpu);
void cpu_run(PDP11 *cpu);
void cpu_step(PDP11 *cpu);

void cpu_set_flag(PDP11 *cpu, uint16_t flag);
void cpu_clear_flag(PDP11 *cpu, uint16_t flag);
int  cpu_get_flag(PDP11 *cpu, uint16_t flag);
void cpu_update_nz(PDP11 *cpu, uint16_t result);

#endif