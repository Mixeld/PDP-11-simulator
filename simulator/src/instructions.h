#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "types.h"

void execute(PDP11 *cpu, uint16_t instr);

#endif


/*
Уровень 1 (простые): ...
  □ NOP .
  □ COM .
  □ BIT .
  □ BIC .
  □ BIS .
  □ SWAB .
  □ JMP .

Уровень 2 (сдвиги): ......
  □ ASL - сдвиг влево
  □ ASR - сдвиг врпаво  0123456
  □ ROL - 
  □ ROR

Уровень 3 (переходы): ...
  □ BGE .
  □ BLT .
  □ BGT .
  □ BLE .
  □ BHI .
  □ BLOS .
  □ BVC .
  □ BVS .
  □ BCC . 
  □ BCS .

Уровень 4 (арифметика с переносом):
  □ ADC .
  □ SBC .
  □ SOB .

Уровень 5 (управление флагами): ...
  □ CLC, CLV, CLZ, CLN, CCC
  □ SEC, SEV, SEZ, SEN, SCC

Уровень 6 (прерывания):
  □ TRAP .
  □ EMT .
  □ RTI 




N — по результату         ✓ (делает cpu_update_nz)
Z — по результату         ✓ (делает cpu_update_nz)
V — всегда 0              ✗ ← ты забыл это
C — НЕ МЕНЯТЬ             ✗ ← ты сбрасываешь, а не надо
*/