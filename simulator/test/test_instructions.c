#include <stdio.h>
#include <assert.h>
#include "../src/cpu.h"
#include "../src/memory.h"

// Вспомогательная функция для выполнения одной инструкции
void execute_instruction(PDP11 *cpu, uint16_t instr) {
    mem_write_word(cpu, cpu->reg[PC], instr);
    cpu_step(cpu);
}

void test_mov() {
    PDP11 cpu;
    cpu_init(&cpu);
    
    printf("Тест MOV...\n");
    
    // MOV #123, R0
    cpu.reg[PC] = 01000;
    execute_instruction(&cpu, 0012700); // MOV
    mem_write_word(&cpu, cpu.reg[PC], 0123);
    cpu.reg[PC] += 2;
    cpu_step(&cpu); // выполнить MOV
    
    assert(cpu.reg[R0] == 0123);
    printf("  MOV #123, R0: OK (R0=%o)\n", cpu.reg[R0]);
    
    // MOV R0, R1
    execute_instruction(&cpu, 0010001); // MOV R0, R1
    assert(cpu.reg[R1] == 0123);
    printf("  MOV R0, R1: OK (R1=%o)\n", cpu.reg[R1]);
}

void test_add() {
    PDP11 cpu;
    cpu_init(&cpu);
    
    printf("Тест ADD...\n");
    
    // MOV #100, R0
    cpu.reg[PC] = 01000;
    execute_instruction(&cpu, 0012700);
    mem_write_word(&cpu, cpu.reg[PC], 0100);
    cpu.reg[PC] += 2;
    cpu_step(&cpu);
    
    // ADD #30, R0
    execute_instruction(&cpu, 0062700);
    mem_write_word(&cpu, cpu.reg[PC], 0030);
    cpu.reg[PC] += 2;
    cpu_step(&cpu);
    
    assert(cpu.reg[R0] == 0130);
    printf("  ADD #30, R0: OK (R0=%o)\n", cpu.reg[R0]);
}

void test_sub() {
    PDP11 cpu;
    cpu_init(&cpu);
    
    printf("Тест SUB...\n");
    
    // MOV #100, R0
    cpu.reg[PC] = 01000;
    execute_instruction(&cpu, 0012700);
    mem_write_word(&cpu, cpu.reg[PC], 0100);
    cpu.reg[PC] += 2;
    cpu_step(&cpu);
    
    // SUB #30, R0
    execute_instruction(&cpu, 0162700);
    mem_write_word(&cpu, cpu.reg[PC], 0030);
    cpu.reg[PC] += 2;
    cpu_step(&cpu);
    
    assert(cpu.reg[R0] == 0050);
    printf("  SUB #30, R0: OK (R0=%o)\n", cpu.reg[R0]);
}

void test_flags() {
    PDP11 cpu;
    cpu_init(&cpu);
    
    printf("Тест флагов...\n");
    
    // ADD с переполнением: 177777 + 1 = 0, C=1, Z=1, V=1
    cpu.reg[PC] = 01000;
    cpu.reg[R0] = 0177777;
    
    execute_instruction(&cpu, 0062700); // ADD #1, R0
    mem_write_word(&cpu, cpu.reg[PC], 0000001);
    cpu.reg[PC] += 2;
    cpu_step(&cpu);
    
    assert(cpu.reg[R0] == 0);
    assert(cpu_get_flag(&cpu, PSW_C) == 1);
    assert(cpu_get_flag(&cpu, PSW_Z) == 1);
    printf("  ADD с переполнением: C=%d Z=%d OK\n", 
           cpu_get_flag(&cpu, PSW_C), cpu_get_flag(&cpu, PSW_Z));
}

void test_cmp() {
    PDP11 cpu;
    cpu_init(&cpu);
    
    printf("Тест CMP...\n");
    
    // MOV #5, R0
    cpu.reg[PC] = 01000;
    execute_instruction(&cpu, 0012700);
    mem_write_word(&cpu, cpu.reg[PC], 0000005);
    cpu.reg[PC] += 2;
    cpu_step(&cpu);
    
    // CMP #5, R0 (должно быть Z=1)
    execute_instruction(&cpu, 0022700);
    mem_write_word(&cpu, cpu.reg[PC], 0000005);
    cpu.reg[PC] += 2;
    cpu_step(&cpu);
    
    assert(cpu_get_flag(&cpu, PSW_Z) == 1);
    printf("  CMP равных: Z=1 OK\n");
}

void test_sob() {
    PDP11 cpu;
    cpu_init(&cpu);
    
    printf("Тест SOB...\n");
    
    // MOV #10, R3 (8 в десятичной)
    cpu.reg[PC] = 01000;
    execute_instruction(&cpu, 0012703);
    mem_write_word(&cpu, cpu.reg[PC], 0000010);
    cpu.reg[PC] += 2;
    cpu_step(&cpu);
    
    // SOB R3, метка (просто проверяем декремент)
    uint16_t sob_instr = 0077300; // SOB R3, +0 (не прыгаем)
    execute_instruction(&cpu, sob_instr);
    
    assert(cpu.reg[R3] == 0000007);
    printf("  SOB декремент: OK (R3=%o)\n", cpu.reg[R3]);
}

int main() {
    printf("\n=== ТЕСТЫ ИНСТРУКЦИЙ ===\n\n");
    
    test_mov();
    test_add();
    test_sub();
    test_flags();
    test_cmp();
    test_sob();
    
    printf("\nВсе тесты инструкций пройдены!\n");
    return 0;
}