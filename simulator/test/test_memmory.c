#include <stdio.h>
#include <assert.h>
#include "../src/memory.h"
#include "../src/types.h"

void test_memory_basic() {
    PDP11 cpu;
    cpu_init(&cpu);
    
    printf("Тест памяти: базовые операции...\n");
    
    // Тест записи/чтения байта
    mem_write_byte(&cpu, 01000, 0x12);
    uint8_t b = mem_read_byte(&cpu, 01000);
    assert(b == 0x12);
    
    // Тест записи/чтения слова
    mem_write_word(&cpu, 01000, 0x1234);
    uint16_t w = mem_read_word(&cpu, 01000);
    assert(w == 0x1234);
    
    // Тест little-endian
    uint8_t lo = mem_read_byte(&cpu, 01000);
    uint8_t hi = mem_read_byte(&cpu, 01001);
    assert(lo == 0x34);
    assert(hi == 0x12);
    
    printf("  OK\n");
}

void test_memory_boundary() {
    PDP11 cpu;
    cpu_init(&cpu);
    
    printf("Тест памяти: граничные адреса...\n");
    
    // Последний байт
    mem_write_byte(&cpu, 0177776, 0xAB);
    assert(mem_read_byte(&cpu, 0177776) == 0xAB);
    
    // Предпоследнее слово
    mem_write_word(&cpu, 0177774, 0xCDEF);
    assert(mem_read_word(&cpu, 0177774) == 0xCDEF);
    
    printf("  OK\n");
}

void test_memory_overwrite() {
    PDP11 cpu;
    cpu_init(&cpu);
    
    printf("Тест памяти: перезапись...\n");
    
    mem_write_word(&cpu, 02000, 0x1111);
    mem_write_word(&cpu, 02000, 0x2222);
    assert(mem_read_word(&cpu, 02000) == 0x2222);
    
    printf("  OK\n");
}

int main() {
    printf("\n=== ТЕСТЫ ПАМЯТИ ===\n\n");
    
    test_memory_basic();
    test_memory_boundary();
    test_memory_overwrite();
    
    printf("\nВсе тесты памяти пройдены!\n");
    return 0;
}