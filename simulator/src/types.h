#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/* Индексы регистров */
#define R0  0
#define R1  1
#define R2  2
#define R3  3
#define R4  4
#define R5  5
#define SP  6
#define PC  7

/* Биты PSW */
#define PSW_C   (1 << 0)
#define PSW_V   (1 << 1)
#define PSW_Z   (1 << 2)
#define PSW_N   (1 << 3)

/* Размер памяти */
#define MEM_SIZE  65536

/* Состояние машины */
typedef struct {
    uint16_t reg[8];
    uint16_t psw;
    uint8_t  memory[MEM_SIZE];
    int      running;
    uint64_t cycles;
} PDP11;

#endif