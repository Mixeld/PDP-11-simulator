#include <stdio.h>
#include <string.h>
#include "types.h"
#include "cpu.h"
#include "loader.h"
#include "debug.h"
#include "devices/terminal.h"

int main(int argc, char *argv[]) {
    PDP11 cpu;
    cpu_init(&cpu);
    
    terminal_init();
    atexit(terminal_cleanup);

    const char *tape_file = NULL;
    int trace_mode = 0;
    int interactive_mode = 0;    
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0) {
            trace_mode = 1;
        } else if (strcmp(argv[i], "-i") == 0) {        /* НОВОЕ */
            interactive_mode = 1;                        /* НОВОЕ */
        } else if (argv[i][0] != '-') {
            tape_file = argv[i];
        }
    }
    
    if (!tape_file) {
        printf("Использование: %s [-t] [-i] <file.lda>\n", argv[0]);
        printf("  -t, --trace  включить трассировку выполнения\n");
        printf("  -i           интерактивный режим (без лимита циклов)\n");
        return 1;
    }
    
    if (loader_load_tape(&cpu, tape_file) != 0) {
        printf("Ошибка загрузки файла\n");
        return 1;
    }
    
    cpu.running = 1;
    long long steps = 0;
    const long long MAX_STEPS = 10000000LL;  
    
    while (cpu.running) {
        if (trace_mode) {
            debug_trace(&cpu, cpu.reg[PC]);
        }
        cpu_step(&cpu);
        steps++;
        
        if (!interactive_mode && steps >= MAX_STEPS) {
            printf("\n[Превышен лимит %lld инструкций]\n", MAX_STEPS);
            cpu.running = 0;
        }
    }
    
    debug_dump_regs(&cpu);
    
    printf("\n=== РЕЗУЛЬТАТ ВЫПОЛНЕНИЯ ===\n");
    printf("RESULT_R0=%06o\n", cpu.reg[R0]);
    printf("RESULT_R1=%06o\n", cpu.reg[R1]);
    printf("RESULT_R2=%06o\n", cpu.reg[R2]);
    printf("RESULT_R3=%06o\n", cpu.reg[R3]);
    printf("RESULT_R4=%06o\n", cpu.reg[R4]);
    printf("RESULT_R5=%06o\n", cpu.reg[R5]);
    printf("RESULT_SP=%06o\n", cpu.reg[SP]);
    printf("RESULT_PC=%06o\n", cpu.reg[PC]);
    printf("RESULT_N=%d\n", cpu_get_flag(&cpu, PSW_N));
    printf("RESULT_Z=%d\n", cpu_get_flag(&cpu, PSW_Z));
    printf("RESULT_V=%d\n", cpu_get_flag(&cpu, PSW_V));
    printf("RESULT_C=%d\n", cpu_get_flag(&cpu, PSW_C));
    printf("=== КОНЕЦ РЕЗУЛЬТАТА ===\n");
    
    return 0;
}