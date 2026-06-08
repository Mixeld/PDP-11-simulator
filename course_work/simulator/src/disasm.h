#ifndef PDP11_DISASM_H
#define PDP11_DISASM_H

#include "types.h"

int disassemble(PDP11 *cpu, uint16_t addr, char *buf, int bufsize);

#endif