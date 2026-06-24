#include <stdio.h>
#include "memory.h"
#include"devices/terminal.h"

uint8_t mem_read_byte(PDP11 *cpu, uint16_t addr) {
    if (terminal_is_io_address(addr)) {
        uint16_t word_val = 0;
        switch (addr & ~1) {
        case TKS: word_val = terminal_read_tks(); break;
        case TKB: word_val = terminal_read_tkb(); break;
        case TPS: word_val = terminal_read_tps(); break;
        case TPB: word_val = 0; break;
        }
        return (addr & 1) ? (word_val >> 8) : (word_val & 0xFF);
    }

    return cpu -> memory[addr];
}

uint16_t mem_read_word(PDP11 *cpu, uint16_t addr) {
    switch (addr) {
        case TKS: return terminal_read_tks();
        case TKB: return terminal_read_tkb();
        case TPS: return terminal_read_tps();
        case TPB: return 0;  
    }
    uint16_t lo = cpu->memory[addr];
    uint16_t hi = cpu->memory[addr + 1];
    return (hi << 8) | lo;
}

void mem_write_byte(PDP11 *cpu, uint16_t addr, uint8_t val) {

    if(terminal_is_io_address(addr)){
        if ((addr & ~1) == TPB) {
            terminal_write_tpb(val);
        }

        return;

    }

    cpu->memory[addr] = val;
}

void mem_write_word(PDP11 *cpu, uint16_t addr, uint16_t val) {

    if (addr == TPB) {
        terminal_write_tpb(val);
        return;
    }
    if (addr == TKS || addr == TKB || addr == TPS){
        return;
    }
    
    cpu->memory[addr]     = val & 0xFF;
    cpu->memory[addr + 1] = (val >> 8) & 0xFF;
}