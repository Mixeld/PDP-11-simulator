#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *name;
    const char *asm_code;
    uint16_t expected_r0;
} TestCase;

TestCase tests[] = {
    {
        "MOV",
        "    .ORG 01000\nSTART:\n    MOV #123, R0\n    HALT\n    .END START\n",
        0123
    },
    {
        "ADD",
        "    .ORG 01000\nSTART:\n    MOV #100, R0\n    ADD #30, R0\n    HALT\n    .END START\n",
        0130
    },
    {
        "SUB",
        "    .ORG 01000\nSTART:\n    MOV #100, R0\n    SUB #30, R0\n    HALT\n    .END START\n",
        0070
    },
    {
        "ASL",
        "    .ORG 01000\nSTART:\n    MOV #10, R0\n    ASL R0\n    HALT\n    .END START\n",
        0020
    },
    {
        "ASR",
        "    .ORG 01000\nSTART:\n    MOV #20, R0\n    ASR R0\n    HALT\n    .END START\n",
        0010
    },
    {
        "INC/DEC",
        "    .ORG 01000\nSTART:\n    MOV #10, R0\n    INC R0\n    DEC R0\n    HALT\n    .END START\n",
        0010
    },
    {NULL, NULL, 0}
};

int main() {
    int passed = 0, failed = 0;
    
    printf("\n========================================\n");
    printf("     ПОЛНАЯ ВАЛИДАЦИЯ СИМУЛЯТОРА\n");
    printf("========================================\n");
    
    for (int i = 0; tests[i].name != NULL; i++) {
        printf("\n[%s]\n", tests[i].name);
        
        // Запись тестовой программы
        FILE *f = fopen("temp.asm", "w");
        fprintf(f, "%s", tests[i].asm_code);
        fclose(f);
        
        // Ассемблирование
        if (system("../assembler/asm temp.asm -o temp > /dev/null 2>&1") != 0) {
            printf("  FAILED: ассемблирование\n");
            failed++;
            continue;
        }
        
        // Запуск симулятора
        char cmd[256];
        snprintf(cmd, sizeof(cmd), "../simulator/pdp11 temp.lda > output.txt 2>&1");
        system(cmd);
        
        // Проверка R0
        FILE *out = fopen("output.txt", "r");
        char line[256];
        int found = 0;
        uint16_t r0 = 0;
        
        while (fgets(line, sizeof(line), out)) {
            if (sscanf(line, "RESULT_R0=%ho", &r0) == 1) {
                found = 1;
                break;
            }
            // старый формат
            if (sscanf(line, "R0: %ho", &r0) == 1) {
                found = 1;
                break;
            }
        }
        fclose(out);
        
        if (found && r0 == tests[i].expected_r0) {
            printf("  PASSED: R0=%o (ожидалось %o)\n", r0, tests[i].expected_r0);
            passed++;
        } else {
            printf("  FAILED: R0=%o (ожидалось %o)\n", r0, tests[i].expected_r0);
            failed++;
        }
    }
    
    // Очистка
    system("rm -f temp.asm temp.lda temp.lst output.txt");
    
    printf("\n========================================\n");
    printf("ИТОГО: пройдено %d, провалено %d\n", passed, failed);
    printf("========================================\n");
    
    return failed;
}