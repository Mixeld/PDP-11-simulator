 #include <stdio.h>
#include "loader.h"
#include "memory.h"

int loader_load_words(PDP11 *cpu, uint16_t start,const uint16_t *words, int count)
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

static uint8_t *read_file_to_buf(const char *filename, long *out_size){
    FILE *f = fopen (filename, "rb");
    if (!f){
        printf("Ошибка открытия файла %s", filename);
        return NULL;
    }

    fseek (f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0){
        printf ("Пустой файла или нет доступа %s", filename);
        fclose(f);
        return NULL;
    }

    if (size > MEM_SIZE){
        printf("Слишком большой файл.");
        fclose(f);
        return NULL;
    }

    uint8_t *buf = (uint8_t *)malloc(size);

    if(!buf){
        printf("Не удалось выделить память");
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, (size_t)size, f);
    fclose (f);

    if ((long)read != size){
        printf("Прочитано %zu из %ld байт", read, size);
        free(buf);
        return NULL;
    }

    *out_size = size;

    return buf;
}


static uint8_t tape_cheksum( uint16_t block_len, uint16_t addr, const uint8_t *data, int data_len){
    uint8_t sum = 0;
    sum += (uint8_t)(block_len & 0xFF);
    sum += (uint8_t)((block_len >> 8) & 0xFF);
    sum += (uint8_t)(addr & 0xFF);
    sum += (uint8_t)((addr >> 8) & 0xFF);
    
    for (int i = 0; i < data_len; i++){
        sum += data[i];
    }

    return(uint8_t)((~sum + 1) & 0xFF);
}


int loader_load_raw (PDP11 *cpu, const char *filename, uint16_t load_addr){
    long size;
    uint8_t *buf = read_file_to_buf (filename, &size);
    if (!buf) return -1;

    if ((long)load_addr + size > MEM_SIZE){
        printf("Данные выходят за пределы памяти");
        free(buf);
        return -1;
    }

    memcpy (&cpu -> memory[load_addr], buf, size);
    free (buf);

    loader_set_start(cpu, load_addr);

    printf("Raw загрузил %ld байт по адресу %06o\n", size, load_addr);
    return 0;
}

static int save_raw(PDP11 *cpu, const char *filename, uint16_t load_addr, uint16_t lenght){
    FILE *f = fopen (filename, "wb");
    if (!f) {
        printf ("Не удалось создать файл\n");
        return -1;
    }

    fwrite(&cpu->memory[load_addr],1, lenght, f);
    fclose (f);  
    printf("Raw сохранён");
    return 0;
}


int loader_load_header(PDP11 *cpu, const char *filename) {
    long size;
    uint8_t *buf = read_file_to_buf(filename, &size);
    if (!buf) return -1;

    if (size < 6) {
        printf("Ошибка: файл слишком маленький (%ld байт)\n", size);
        free(buf);
        return -1;
    }

    uint16_t load_addr  = (uint16_t)(buf[0] | (buf[1] << 8));
    uint16_t start_addr = (uint16_t)(buf[2] | (buf[3] << 8));
    uint16_t data_size  = (uint16_t)(buf[4] | (buf[5] << 8));

    if (data_size != size - 6) {
        printf("Предупреждение: размер в заголовке (%d) "
               "не совпадает с файлом (%ld)\n",
               data_size, size - 6);
        data_size = (uint16_t)(size - 6);
    }

    if ((long)load_addr + data_size > MEM_SIZE) {
        printf("Ошибка: данные выходят за пределы памяти\n");
        free(buf);
        return -1;
    }

    memcpy(&cpu->memory[load_addr], buf + 6, data_size);
    free(buf);

    cpu->reg[PC] = start_addr;
    cpu->reg[SP] = 0xFFFE;

    printf("Header: загружено %d байт по адресу %06o, старт %06o\n",
           data_size, load_addr, start_addr);
    return 0;
}


static int save_header(PDP11 *cpu, const char *filename, uint16_t load_addr, uint16_t start_addr, uint16_t length ){
    FILE *f = fopen (filename, "wb");

    if (!f) {
        printf ("Не удалось создать файл\n");
        return -1;
    }

    // Подготовка заголовка 
    fwrite(&load_addr, 2, 1, f);
    fwrite(&start_addr, 2, 1, f);
    fwrite(&length, 2, 1,f);

    //Пишем данные 
    fwrite(&cpu -> memory[load_addr], 1, length, f);
    fclose(f);

    printf ("Header сохранён \n");

    return 0;
}

int loader_load_tape(PDP11 *cpu, const char *filename)
{
    long file_size;
    uint8_t *buf = read_file_to_buf(filename, &file_size);
    if (!buf) return -1;

    long pos        = 0;
    int  blocks     = 0;
    int  total      = 0;
    uint16_t start  = 0;

    printf("Tape: загрузка '%s'\n", filename);

    while (pos < file_size) {
        while (pos < file_size && buf[pos] == 0x00)
            pos++;

        if (pos >= file_size) break;

        if (buf[pos] != 0x01) {
            printf("  Предупреждение: неожиданный байт %02x "
                   "на смещении %ld\n", buf[pos], pos);
            pos++;
            continue;
        }

        if (pos + 6 > file_size) {
            printf("  Ошибка: неполный заголовок блока\n");
            break;
        }

        uint16_t block_len = (uint16_t)(buf[pos + 2] | (buf[pos + 3] << 8));
        uint16_t addr      = (uint16_t)(buf[pos + 4] | (buf[pos + 5] << 8));

        int data_len = (int)block_len - 6;

        if (data_len <= 0) {
            start = addr;
            printf("  Стартовый адрес: %06o\n", start);
            break;
        }

        if (pos + 7 + data_len > file_size) {
            printf("  Ошибка: блок выходит за пределы файла\n");
            break;
        }

        if ((long)addr + data_len <= MEM_SIZE) {
            memcpy(&cpu->memory[addr], buf + pos + 7, data_len);
            printf("  Блок %d: %d байт по адресу %06o\n",
                   blocks, data_len, addr);
            total += data_len;
            blocks++;
        } else {
            printf("  Ошибка: блок выходит за пределы памяти\n");
        }

        pos += 7 + data_len;
    }

    free(buf);

    cpu->reg[PC] = start;
    cpu->reg[SP] = 0xFFFE;

    printf("Tape: загружено %d блоков, %d байт, старт %06o\n",
           blocks, total, start);
    return 0;
}

static int save_tape(PDP11 *cpu, const char *filename, uint16_t load_addr, uint16_t start_addr, uint16_t length)
{
    FILE *f = fopen(filename, "wb");
    if (!f) {
        printf("Ошибка: не удалось создать файл '%s'\n", filename);
        return -1;
    }

    const uint8_t *data = &cpu->memory[load_addr];

    uint16_t block_len = length + 6;
    uint8_t  csum = tape_cheksum(block_len, load_addr, data, length);

    uint8_t marker[2] = {0x01, 0x00};
    fwrite(marker,      1, 2, f);
    fwrite(&block_len,  2, 1, f);
    fwrite(&load_addr,  2, 1, f);
    fwrite(&csum,       1, 1, f);
    fwrite(data,        1, length, f);

    uint16_t start_block_len = 6;
    uint8_t  start_csum = tape_cheksum(start_block_len, start_addr, NULL, 0);

    fwrite(marker,           1, 2, f);
    fwrite(&start_block_len, 2, 1, f);
    fwrite(&start_addr,      2, 1, f);
    fwrite(&start_csum,      1, 1, f);

    fclose(f);

    printf("Tape: сохранено %d байт в '%s'\n", length, filename);
    return 0;
}

int loader_save (PDP11 *cpu, const char *filename, uint16_t load_addr, uint16_t start_addr, uint16_t length, int format){
    if ((long)load_addr + length > MEM_SIZE){
        printf("Данные выходят за пределы памяти");
        return -1;
    }

    switch (format)
    {
    case 0:
        return save_raw (cpu, filename, load_addr, length);
    case 1:
        return save_header(cpu, filename, load_addr, start_addr, length);
    case 2:
        return save_tape (cpu, filename, load_addr, start_addr, length);
    default:
        printf("Неизвестный формат %d\n", format);
        return -1;
    }

}
