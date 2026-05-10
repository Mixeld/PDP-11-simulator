#ifndef LOADER_H
#define LOADER_H

#include "types.h"

int  loader_load_words(PDP11 *cpu, uint16_t start,
                       const uint16_t *words, int count);
void loader_set_start(PDP11 *cpu, uint16_t start_addr);

#endif