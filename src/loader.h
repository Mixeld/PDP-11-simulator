#ifndef LOADER_H
#define LOADER_H

#include "types.h"

int  loader_load_words(PDP11 *cpu, uint16_t start, const uint16_t *words, int count);
void loader_set_start(PDP11 *cpu, uint16_t start_addr);

/*
 * Формат 1: Raw binary
 * Файл содержит просто байты без заголовка.
 * load_addr — куда загружать в памяти симулятора.
 */
int loader_load_raw(PDP11 *cpu, const char *filename, uint16_t load_addr);

/*
 * Формат 2: С заголовком
 * Заголовок (6 байт):
 *   Байты 0-1: адрес загрузки  (little-endian)
 *   Байты 2-3: стартовый адрес (little-endian)
 *   Байты 4-5: размер данных   (little-endian)
 * Далее идут сами данные.
 */
int loader_load_header(PDP11 *cpu, const char *filename);

/*
 * Формат 3: SIMH paper tape
 * Стандартный формат для эмуляторов PDP-11.
 * Файл состоит из блоков с заголовками и данными.
 */
int loader_load_tape(PDP11 *cpu, const char *filename);

/*
 * Сохранить диапазон памяти в файл
 * format: 0 = raw, 1 = с заголовком, 2 = tape
 */
int loader_save(PDP11 *cpu, const char *filename, uint16_t load_addr, uint16_t start_addr, uint16_t length, int format);

#endif