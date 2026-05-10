#include <stdio.h>
#include "loader.h"
#include "memory.h"

int loader_load_words(PDP11 *cpu, uint16_t start,
                      const uint16_t *words, int count)
{
    for (int i = 0; i < count; i++) {
        uint16_t addr = start + i * 2;
        if (addr + 1 >= MEM_SIZE) {
            printf("Ошибка загрузки: адрес %06o за пределами памяти\n", addr);
            return -1;
        }
        mem_write_word(cpu, addr, words[i]);
    }
    return 0;
}

void loader_set_start(PDP11 *cpu, uint16_t start_addr)
{
    cpu->reg[PC] = start_addr;
    cpu->reg[SP] = 0xFFFE;
}