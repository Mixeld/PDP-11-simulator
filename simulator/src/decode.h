#ifndef DECODE_H
#define DECODE_H

#include "types.h"

#define INSTR_SRC_MODE(i)       (((i) >> 9) & 7)
#define INSTR_SRC_REG(i)        (((i) >> 6) & 7)
#define INSTR_DST_MODE(i)       (((i) >> 3) & 7)
#define INSTR_DST_REG(i)        ((i) & 7)
#define INSTR_BRANCH_OFFSET(i)  ((int8_t)((i) & 0xFF))

uint16_t fetch_operand(PDP11 *cpu, int mode, int reg);
uint16_t resolve_dst_addr(PDP11 *cpu, int mode, int reg);
uint16_t resolve_byte_addr(PDP11 *cpu, int mode, int reg);
void     store_operand(PDP11 *cpu, int mode, int reg, uint16_t val);

#endif