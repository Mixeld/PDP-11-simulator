#ifndef PDP11_MEMORY_H
#define PDP11_MEMORY_H

#include <stdint.h>
#include "types.h"

uint8_t  mem_read_byte(PDP11 *cpu, uint16_t addr);
uint16_t mem_read_word(PDP11 *cpu, uint16_t addr);
void     mem_write_byte(PDP11 *cpu, uint16_t addr, uint8_t val);
void     mem_write_word(PDP11 *cpu, uint16_t addr, uint16_t val);

#endif