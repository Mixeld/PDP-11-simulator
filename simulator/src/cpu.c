#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "memory.h"
#include "instructions.h"
#include "debug.h"

void cpu_init(PDP11 *cpu)
{
    cpu_reset(cpu);
}

void cpu_reset(PDP11 *cpu)
{
    memset(cpu->reg, 0, sizeof(cpu->reg));
    memset(cpu->memory, 0, MEM_SIZE);
    cpu->psw = 0;
    cpu->running = 0;
    cpu->cycles = 0;
}

void cpu_set_flag(PDP11 *cpu, uint16_t flag)
{
    cpu->psw |= flag;
}

void cpu_clear_flag(PDP11 *cpu, uint16_t flag)
{
    cpu->psw &= ~flag;
}

int cpu_get_flag(PDP11 *cpu, uint16_t flag)
{
    return (cpu->psw & flag) ? 1 : 0;
}

void cpu_update_nz(PDP11 *cpu, uint16_t result)
{
    if (result == 0)
        cpu_set_flag(cpu, PSW_Z);
    else
        cpu_clear_flag(cpu, PSW_Z);

    if (result & 0x8000)
        cpu_set_flag(cpu, PSW_N);
    else
        cpu_clear_flag(cpu, PSW_N);
}

void cpu_step(PDP11 *cpu)
{
    uint16_t instr = mem_read_word(cpu, cpu->reg[PC]);
    cpu->reg[PC] += 2;
    execute(cpu, instr);
    cpu->cycles++;
}

void cpu_run(PDP11 *cpu)
{
    cpu->running = 1;

    while (cpu->running) {
        debug_trace(cpu, cpu->reg[PC]);
        cpu_step(cpu);

        if (cpu->cycles > 100000) {
            printf("Превышен лимит циклов\n");
            cpu->running = 0;
        }
    }

    printf("\n=== Остановка после %llu инструкций ===\n",
           (unsigned long long)cpu->cycles);
    debug_dump_regs(cpu);
}

void cpu_update_nz_byte (PDP11 *cpu, uint16_t result){
    if (result == 0)
        cpu_set_flag(cpu, PSW_Z);
    else 
        cpu_clear_flag(cpu, PSW_Z);

    if (result & 0x80)
        cpu_set_flag(cpu, PSW_N);
    else
        cpu_clear_flag(cpu, PSW_N);
}