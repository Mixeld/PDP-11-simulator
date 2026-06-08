/*
 * Двухпроходной ассемблер для процессора PDP-11.
 *
 * Читает исходный файл на языке ассемблера PDP-11 (MACRO-11-совместимый синтаксис),
 * выполняет два прохода: первый собирает таблицу меток, второй генерирует машинный
 * код. На выходе создаёт два файла:
 *   - .lda  — двоичный образ в формате SIMH tape (загружается симулятором PDP-11)
 *   - .lst  — листинг с адресами, кодами и исходными строками
 *
 * Поддерживаемые возможности:
 *   - Все стандартные инструкции PDP-11 (одно- и двухоперандные, ветвления, EIS и др.)
 *   - Все режимы адресации PDP-11 (прямой, косвенный, авто-инкремент/декремент,
 *     индексный, непосредственный, абсолютный, PC-относительный)
 *   - Директивы: .ORG, .WORD, .BYTE, .BLKW, .BLKB, .ASCII, .ASCIZ, .EVEN, .ODD, .END
 *   - Локальные числовые метки (1$:), символьные литералы ('A'), escape-последовательности
 *   - Числовые литералы: восьмеричные (по умолчанию), 0x... (hex), 0b... (bin), N. (дес.)
 *   - Ключи командной строки: -o (выходной файл), -s (стартовый адрес), -v (подробный вывод)
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MEMORY_SIZE 65536

static uint8_t  memory[MEMORY_SIZE];
static uint8_t  memory_used[MEMORY_SIZE];
static uint16_t LC        = 0;
static uint32_t max_lc    = 0;
static int quiet = 0;     
static int verbose = 0;  

static int current_line   = 0;
static int error_count    = 0;
static int pass_number    = 0;



/* ══════════════════════════════════════════════════════════════════════════
 *  Утилиты
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * asm_error — выводит сообщение об ошибке с номером строки, текущим адресом
 * и номером прохода; увеличивает счётчик ошибок.
 */
static void asm_error(const char *msg) {
    fprintf(stderr, "ОШИБКА [Строка %d, Адрес %06o, Проход %d]: %s\n",
            current_line, LC, pass_number, msg);
    error_count++;
}

/*
 * portable_strcasecmp — регистронезависимое сравнение двух строк целиком.
 * Портируемая замена POSIX-функции strcasecmp.
 */
static int portable_strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int d = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (d != 0) return d;
        s1++; s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

/*
 * portable_strncasecmp — регистронезависимое сравнение не более n символов.
 * Портируемая замена POSIX-функции strncasecmp.
 */
static int portable_strncasecmp(const char *s1, const char *s2, size_t n) {
    while (n-- > 0) {
        if (!*s1 || !*s2)
            return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        int d = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (d != 0) return d;
        s1++; s2++;
    }
    return 0;
}

/*
 * safe_strcpy — копирует строку src в dest с гарантированным нуль-терминатором
 * и без выхода за границу буфера dest_size.
 */
static void safe_strcpy(char *dest, const char *src, size_t dest_size) {
    if (dest_size > 0 && src != NULL) {
        size_t slen = strlen(src);
        size_t n = (slen < dest_size - 1) ? slen : dest_size - 1;
        memcpy(dest, src, n);
        dest[n] = '\0';
    }
}

/*
 * write_byte — записывает один байт value по адресу address в массив memory,
 * помечает ячейку как занятую и обновляет max_lc.
 */
static void write_byte(uint32_t address, uint8_t value) {
    if (address >= MEMORY_SIZE) {
        asm_error("Выход за пределы памяти!");
        return;
    }
    memory[address] = value;
    memory_used[address] = 1;
    if (address > max_lc) max_lc = address;
}

/*
 * write_word — записывает 16-битное слово value по чётному адресу address
 * в формате little-endian (младший байт первым).
 */
static void write_word(uint32_t address, uint16_t value) {
    if (address & 1) {
        asm_error("Запись слова по нечётному адресу!");
        return;
    }
    write_byte(address,       (uint8_t)(value & 0xFF));
    write_byte((address + 1) & 0xFFFF, (uint8_t)((value >> 8) & 0xFF));
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Таблица символов
 * ══════════════════════════════════════════════════════════════════════════ */

#define MAX_SYMBOLS 2048
#define LABEL_LEN   64

typedef struct Symbol {
    char           name[LABEL_LEN];
    uint16_t       address;
    int            defined;
    struct Symbol *next;
} Symbol;

static Symbol *symbol_table = NULL;

/*
 * find_symbol — ищет символ по имени в односвязном списке symbol_table.
 * Возвращает указатель на Symbol или NULL, если символ не найден.
 */
static Symbol *find_symbol(const char *name) {
    for (Symbol *s = symbol_table; s != NULL; s = s->next)
        if (portable_strcasecmp(s->name, name) == 0)
            return s;
    return NULL;
}

/*
 * get_symbol_address — возвращает адрес символа по имени.
 * Если символ не найден или ещё не определён — возвращает 0xFFFF.
 */
static uint16_t get_symbol_address(const char *name) {
    Symbol *s = find_symbol(name);
    if (s && s->defined) return s->address;
    return 0xFFFF;
}

/*
 * add_symbol — добавляет новый символ в таблицу или разрешает forward-reference
 * (если символ уже присутствует, но не был определён). При дублировании
 * определённого символа вызывает asm_error.
 */
static void add_symbol(const char *name, uint16_t address) {
    Symbol *s = find_symbol(name);
    if (s) {
        if (s->defined) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Дублирующаяся метка: %s", name);
            asm_error(buf);
            return;
        }
        s->address = address;
        s->defined  = 1;
        return;
    }
    s = (Symbol *)malloc(sizeof(Symbol));
    if (!s) { asm_error("Нет памяти для таблицы символов"); return; }
    safe_strcpy(s->name, name, sizeof(s->name));
    s->address = address;
    s->defined  = 1;
    s->next     = symbol_table;
    symbol_table = s;
}

/*
 * free_symbol_table — освобождает всю память, занятую таблицей символов,
 * и обнуляет указатель symbol_table.
 */
static void free_symbol_table(void) {
    Symbol *cur = symbol_table;
    while (cur) {
        Symbol *nxt = cur->next;
        free(cur);
        cur = nxt;
    }
    symbol_table = NULL;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Таблица инструкций
 * ══════════════════════════════════════════════════════════════════════════ */

typedef enum {
    NO_OP,
    SINGLE_OP,
    DOUBLE_OP,
    BRANCH,
    JSR_OP,
    RTS_OP,
    SOB_OP,
    EIS_OP,
    XOR_OP,
    EMT_OP,
    MARK_OP,
    SPL_OP
} InstructionType;

typedef struct {
    const char     *mnemonic;
    uint16_t        base_opcode;
    InstructionType type;
} Instruction;

static const Instruction instruction_table[] = {
    { "HALT",  0000000, NO_OP },
    { "WAIT",  0000001, NO_OP },
    { "RTI",   0000002, NO_OP },
    { "BPT",   0000003, NO_OP },
    { "IOT",   0000004, NO_OP },
    { "RESET", 0000005, NO_OP },
    { "RTT",   0000006, NO_OP },
    { "NOP",   0000240, NO_OP },
    { "CLC",   0000241, NO_OP },
    { "CLV",   0000242, NO_OP },
    { "CLZ",   0000244, NO_OP },
    { "CLN",   0000250, NO_OP },
    { "CCC",   0000257, NO_OP },
    { "SEC",   0000261, NO_OP },
    { "SEV",   0000262, NO_OP },
    { "SEZ",   0000264, NO_OP },
    { "SEN",   0000270, NO_OP },
    { "SCC",   0000277, NO_OP },

    { "SPL",   0000230, SPL_OP },

    { "MARK",  0006400, MARK_OP },

    { "JMP",   0000100, SINGLE_OP },
    { "SWAB",  0000300, SINGLE_OP },
    { "CLR",   0005000, SINGLE_OP },
    { "COM",   0005100, SINGLE_OP },
    { "INC",   0005200, SINGLE_OP },
    { "DEC",   0005300, SINGLE_OP },
    { "NEG",   0005400, SINGLE_OP },
    { "ADC",   0005500, SINGLE_OP },
    { "SBC",   0005600, SINGLE_OP },
    { "TST",   0005700, SINGLE_OP },
    { "ROR",   0006000, SINGLE_OP },
    { "ROL",   0006100, SINGLE_OP },
    { "ASR",   0006200, SINGLE_OP },
    { "ASL",   0006300, SINGLE_OP },
    { "SXT",   0006700, SINGLE_OP },
    { "MFPI",  0006500, SINGLE_OP },
    { "MTPI",  0006600, SINGLE_OP },

    { "CLRB",  0105000, SINGLE_OP },
    { "COMB",  0105100, SINGLE_OP },
    { "INCB",  0105200, SINGLE_OP },
    { "DECB",  0105300, SINGLE_OP },
    { "NEGB",  0105400, SINGLE_OP },
    { "ADCB",  0105500, SINGLE_OP },
    { "SBCB",  0105600, SINGLE_OP },
    { "TSTB",  0105700, SINGLE_OP },
    { "RORB",  0106000, SINGLE_OP },
    { "ROLB",  0106100, SINGLE_OP },
    { "ASRB",  0106200, SINGLE_OP },
    { "ASLB",  0106300, SINGLE_OP },
    { "MFPD",  0106500, SINGLE_OP },
    { "MTPD",  0106600, SINGLE_OP },
    { "SXTB",  0106700, SINGLE_OP },

    { "MOV",   0010000, DOUBLE_OP },
    { "CMP",   0020000, DOUBLE_OP },
    { "BIT",   0030000, DOUBLE_OP },
    { "BIC",   0040000, DOUBLE_OP },
    { "BIS",   0050000, DOUBLE_OP },
    { "ADD",   0060000, DOUBLE_OP },
    { "SUB",   0160000, DOUBLE_OP },

    { "MOVB",  0110000, DOUBLE_OP },
    { "CMPB",  0120000, DOUBLE_OP },
    { "BITB",  0130000, DOUBLE_OP },
    { "BICB",  0140000, DOUBLE_OP },
    { "BISB",  0150000, DOUBLE_OP },

    { "ASHC",  0073000, EIS_OP },
    { "MUL",   0070000, EIS_OP },
    { "DIV",   0071000, EIS_OP },
    { "ASH",   0072000, EIS_OP },

    { "XOR",   0074000, XOR_OP },

    { "BR",    0000400, BRANCH },
    { "BNE",   0001000, BRANCH },
    { "BEQ",   0001400, BRANCH },
    { "BGE",   0002000, BRANCH },
    { "BLT",   0002400, BRANCH },
    { "BGT",   0003000, BRANCH },
    { "BLE",   0003400, BRANCH },
    { "BPL",   0100000, BRANCH },
    { "BMI",   0100400, BRANCH },
    { "BHI",   0101000, BRANCH },
    { "BLOS",  0101400, BRANCH },
    { "BVC",   0102000, BRANCH },
    { "BVS",   0102400, BRANCH },
    { "BCC",   0103000, BRANCH },
    { "BCS",   0103400, BRANCH },
    { "BHIS",  0103000, BRANCH },
    { "BLO",   0103400, BRANCH },

    { "JSR",   0004000, JSR_OP  },
    { "RTS",   0000200, RTS_OP  },
    { "SOB",   0077000, SOB_OP  },

    { "EMT",   0104000, EMT_OP  },
    { "TRAP",  0104400, EMT_OP  },

    { NULL, 0, NO_OP }
};

/*
 * get_instruction — ищет инструкцию по мнемонике в таблице instruction_table
 * (регистронезависимо). Возвращает указатель на Instruction или NULL.
 */
static const Instruction *get_instruction(const char *mnemonic) {
    for (int i = 0; instruction_table[i].mnemonic != NULL; i++)
        if (portable_strcasecmp(instruction_table[i].mnemonic, mnemonic) == 0)
            return &instruction_table[i];
    return NULL;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Лексика: trim, strip comments
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * trim — убирает ведущие и хвостовые пробелы из строки на месте.
 * Возвращает указатель на первый непробельный символ.
 */
static char *trim(char *str) {
    if (!str) return str;
    while (isspace((unsigned char)*str)) str++;
    if (!*str) return str;
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

/*
 * strip_comment — удаляет хвостовой комментарий (начинающийся с ';'),
 * игнорируя символ ';' внутри строковых литералов в двойных кавычках.
 */
static void strip_comment(char *line) {
    int in_q = 0;
    for (char *p = line; *p; p++) {
        if (*p == '"') in_q = !in_q;
        if (*p == ';' && !in_q) { *p = '\0'; return; }
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Парсинг чисел и выражений
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * eval_atom — вычисляет одиночный операнд выражения: текущий счётчик адреса '.'),
 * символьный литерал ('A'), числовой литерал (0x, 0b, N., восьмеричный) или
 * имя символа из таблицы. Возвращает 1 при успехе, 0 если символ не найден.
 */
static int eval_atom(const char *s, uint16_t *out, uint16_t lc_val) {
    while (isspace((unsigned char)*s)) s++;

    if (s[0] == '.' && s[1] == '\0') {
        *out = lc_val;
        return 1;
    }

    if (s[0] == '\'') {
        if (s[1] == '\\') {
            switch (s[2]) {
            case 'n': *out = '\n'; break;
            case 't': *out = '\t'; break;
            case 'r': *out = '\r'; break;
            case '0': *out = '\0'; break;
            case '\\': *out = '\\'; break;
            default:  *out = (uint8_t)s[2]; break;
            }
        } else {
            *out = (uint8_t)s[1];
        }
        return 1;
    }

    if (portable_strncasecmp(s, "0x", 2) == 0) {
        *out = (uint16_t)strtoul(s + 2, NULL, 16);
        return 1;
    }
    if (portable_strncasecmp(s, "0b", 2) == 0) {
        *out = (uint16_t)strtoul(s + 2, NULL, 2);
        return 1;
    }

    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '.') {
        char tmp[64];
        if (len < sizeof(tmp)) {
            memcpy(tmp, s, len - 1);
            tmp[len - 1] = '\0';
            *out = (uint16_t)strtoul(tmp, NULL, 10);
            return 1;
        }
    }

    const char *p = s;
    if (*p == '-' || *p == '+') p++;
    int has_dollar = (strchr(p, '$') != NULL);
    int is_digit_start = !has_dollar &&
                         (isdigit((unsigned char)*p) ||
                          (portable_strncasecmp(p, "0x", 2) == 0) ||
                          (portable_strncasecmp(p, "0b", 2) == 0));

    if (!is_digit_start) {
        const char *name = s;
        int neg = 0;
        if (*name == '-') { neg = 1; name++; }
        else if (*name == '+') { name++; }

        uint16_t addr = get_symbol_address(name);
        if (addr == 0xFFFF) return 0;
        *out = neg ? (uint16_t)(-(int16_t)addr) : addr;
        return 1;
    }

    if (*s == '-') {
        *out = (uint16_t)(-(int16_t)(uint16_t)strtoul(s + 1, NULL, 8));
    } else {
        *out = (uint16_t)strtoul(s, NULL, 8);
    }
    return 1;
}

/*
 * evaluate_expr — рекурсивно вычисляет арифметическое выражение, поддерживая
 * операторы +, -, *, /, &, |, ^ и унарный минус. Разбор идёт слева направо
 * без учёта приоритетов (как в MACRO-11). Через ok_out сигнализирует,
 * удалось ли разрешить все символы.
 */
static uint16_t evaluate_expr(const char *expr_in, uint16_t lc_val, int *ok_out) {
    char buf[128];
    safe_strcpy(buf, expr_in, sizeof(buf));
    char *expr = trim(buf);

    if (ok_out) *ok_out = 1;
    if (!expr || !*expr) return 0;

    static const char *ops = "+-*/&|^";
    char *op_ptr = NULL;
    char  op     = 0;
    int paren = 0;

    for (size_t i = 1; i < strlen(expr); i++) {
        char c = expr[i];
        if (c == '(') paren++;
        else if (c == ')') paren--;
        if (paren == 0 && strchr(ops, c)) {
            op_ptr = &expr[i];
            op = c;
            break;
        }
    }

    if (op_ptr) {
        *op_ptr = '\0';
        int ok1 = 1, ok2 = 1;
        uint16_t v1 = evaluate_expr(trim(expr),          lc_val, &ok1);
        uint16_t v2 = evaluate_expr(trim(op_ptr + 1),    lc_val, &ok2);
        if (ok_out) *ok_out = ok1 && ok2;
        switch (op) {
        case '+': return v1 + v2;
        case '-': return v1 - v2;
        case '*': return v1 * v2;
        case '/': return v2 ? (v1 / v2) : 0;
        case '&': return v1 & v2;
        case '|': return v1 | v2;
        case '^': return v1 ^ v2;
        default:  return v1;
        }
    }

    uint16_t v = 0;
    int ok = eval_atom(expr, &v, lc_val);
    if (ok_out) *ok_out = ok;
    return v;
}

/*
 * evaluate_word_for_operand — вычисляет значение слова-расширения для операнда.
 * Для PC-относительных режимов (mode 6/7 с reg=7) автоматически преобразует
 * абсолютный адрес в смещение от текущего PC.
 */
static uint16_t evaluate_word_for_operand(const char *expr,
                                          uint8_t mode, uint8_t reg,
                                          uint16_t current_pc) {
    int ok = 1;
    uint16_t v = evaluate_expr(expr, LC, &ok);
    if (!ok && pass_number == 2) {
        char msg[128];
        snprintf(msg, sizeof(msg), "Неизвестный символ: %s", expr);
        asm_error(msg);
    }
    if (reg == 7 && (mode == 6 || mode == 7))
        v = v - current_pc;
    return v;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Обработка escape-последовательностей
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * process_string — разворачивает строку src с escape-последовательностями
 * в байты, начиная с адреса base_lc. При do_write==0 только считает байты,
 * не записывая их в память. Возвращает итоговое количество байт.
 */
static size_t process_string(const char *src, uint32_t base_lc, int do_write) {
    size_t cnt = 0;
    for (const char *p = src; *p; ) {
        uint8_t byte;
        if (*p == '\\') {
            p++;
            switch (*p) {
            case 'n':  byte = '\n'; break;
            case 't':  byte = '\t'; break;
            case 'r':  byte = '\r'; break;
            case '\\': byte = '\\'; break;
            case '"':  byte = '"';  break;
            case '\'': byte = '\''; break;
            case '0':  byte = '\0'; break;
            case 'a':  byte = '\a'; break;
            case 'b':  byte = '\b'; break;
            case 'f':  byte = '\f'; break;
            case 'v':  byte = '\v'; break;
            case 'x': {
                char hex[3] = {0, 0, 0};
                if (*(p+1)) { hex[0] = *(++p); }
                if (*(p+1)) { hex[1] = *(++p); }
                byte = (uint8_t)strtoul(hex, NULL, 16);
                break;
            }
            default:
                if (*p >= '0' && *p <= '7') {
                    char oct[4] = {0};
                    oct[0] = *p;
                    if (*(p+1) >= '0' && *(p+1) <= '7') { oct[1] = *(++p); }
                    if (*(p+1) >= '0' && *(p+1) <= '7') { oct[2] = *(++p); }
                    byte = (uint8_t)strtoul(oct, NULL, 8);
                } else {
                    byte = (uint8_t)*p;
                }
                break;
            }
        } else {
            byte = (uint8_t)*p;
        }
        if (do_write) write_byte(base_lc + (uint32_t)cnt, byte);
        cnt++;
        p++;
    }
    return cnt;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Разбор строки исходника
 * ══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    char label[LABEL_LEN];
    char mnemonic[16];
    char operands_raw[256];
} ParsedLine;

/*
 * parse_line — разбирает одну строку исходника на составляющие: метку
 * (оканчивается ':'), мнемонику (первое слово после метки) и строку
 * операндов (остаток строки после мнемоники). Комментарии удаляются.
 */
static void parse_line(char *line, ParsedLine *pl) {
    memset(pl, 0, sizeof(*pl));

    for (char *c = line; *c; c++) {
        if (*c == '\r' || *c == '\n') { *c = '\0'; break; }
    }
    strip_comment(line);
    line = trim(line);
    if (!*line) return;

    {
        int in_q = 0;
        char *colon = NULL;
        for (char *c = line; *c; c++) {
            if (*c == '"') in_q = !in_q;
            if (*c == ':' && !in_q) { colon = c; break; }
        }
        if (colon) {
            *colon = '\0';
            safe_strcpy(pl->label, trim(line), sizeof(pl->label));
            line = trim(colon + 1);
        }
    }
    if (!*line) return;

    {
        char *sp = line;
        while (*sp && !isspace((unsigned char)*sp)) sp++;
        if (*sp) {
            size_t len = (size_t)(sp - line);
            if (len >= sizeof(pl->mnemonic)) len = sizeof(pl->mnemonic) - 1;
            strncpy(pl->mnemonic, line, len);
            pl->mnemonic[len] = '\0';
            line = trim(sp + 1);
            safe_strcpy(pl->operands_raw, line, sizeof(pl->operands_raw));
        } else {
            safe_strcpy(pl->mnemonic, line, sizeof(pl->mnemonic));
        }
    }
}

/*
 * split_operands — разбивает строку операндов на отдельные токены по запятым,
 * корректно игнорируя запятые внутри строковых литералов и скобок.
 * Возвращает количество найденных операндов.
 */
static int split_operands(const char *src, char ops[][128], int max_ops) {
    int cnt = 0;
    int in_q = 0, paren = 0;
    char cur[128]; size_t cur_len = 0;

    for (const char *p = src; ; p++) {
        char c = *p;
        if (c == '"') in_q = !in_q;
        if (!in_q) {
            if (c == '(' || c == '[') paren++;
            if (c == ')' || c == ']') paren--;
        }
        if ((c == ',' && !in_q && paren == 0) || c == '\0') {
            cur[cur_len] = '\0';
            if (cnt < max_ops) {
                safe_strcpy(ops[cnt], trim(cur), 128);
                cnt++;
            }
            cur_len = 0;
            if (c == '\0') break;
        } else {
            if (cur_len < sizeof(cur) - 1) cur[cur_len++] = c;
        }
    }
    return cnt;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Разбор операнда
 * ══════════════════════════════════════════════════════════════════════════ */

typedef struct {
    uint8_t mode;
    uint8_t reg;
    int     has_next_word;
    char    word_expr[64];
} OperandInfo;

/*
 * get_register_number — возвращает номер регистра (0–7) по его строковому
 * обозначению (R0–R7, SP=6, PC=7). Возвращает -1 если строка не является
 * именем регистра.
 */
static int get_register_number(const char *str) {
    if (!str || !*str) return -1;
    if (portable_strcasecmp(str, "SP") == 0) return 6;
    if (portable_strcasecmp(str, "PC") == 0) return 7;
    if ((str[0] == 'R' || str[0] == 'r') &&
        isdigit((unsigned char)str[1]) && str[2] == '\0') {
        int r = str[1] - '0';
        if (r >= 0 && r <= 7) return r;
    }
    return -1;
}

/*
 * parse_operand — определяет режим адресации PDP-11 и заполняет структуру
 * OperandInfo (mode, reg, флаг наличия слова-расширения и его выражение).
 * Распознаёт все восемь режимов: прямой, косвенный, авто-инкремент/декремент,
 * индексный, непосредственный (#), абсолютный, а также их отложенные варианты (@).
 */
static void parse_operand(const char *op_in, OperandInfo *info) {
    memset(info, 0, sizeof(*info));
    char op[128];
    safe_strcpy(op, op_in, sizeof(op));
    char *s = trim(op);
    if (!*s) return;

    int is_deferred = 0;
    if (s[0] == '@' || s[0] == '*') {
        is_deferred = 1;
        s++;
    }

    if (s[0] == '#') {
        info->mode = is_deferred ? 3 : 2;
        info->reg  = 7;
        info->has_next_word = 1;
        safe_strcpy(info->word_expr, s + 1, sizeof(info->word_expr));
        return;
    }

    if (strncmp(s, "-(", 2) == 0) {
        char reg_buf[16] = {0};
        safe_strcpy(reg_buf, s + 2, sizeof(reg_buf));
        char *cp = strchr(reg_buf, ')'); if (cp) *cp = '\0';
        info->mode = is_deferred ? 5 : 4;
        info->reg  = (uint8_t)get_register_number(trim(reg_buf));
        return;
    }

    {
        char *plus = strrchr(s, '+');
        if (plus && plus > s && *(plus - 1) == ')') {
            char *open = strchr(s, '(');
            if (open) {
                char reg_buf[16] = {0};
                size_t rlen = (size_t)(plus - 1 - open - 1);
                if (rlen < sizeof(reg_buf)) {
                    strncpy(reg_buf, open + 1, rlen);
                    reg_buf[rlen] = '\0';
                }
                info->mode = is_deferred ? 3 : 2;
                info->reg  = (uint8_t)get_register_number(trim(reg_buf));
                return;
            }
        }
    }

    {
        char *popen  = strchr(s, '(');
        char *pclose = strrchr(s, ')');
        if (popen && pclose && pclose > popen) {
            char reg_buf[16] = {0};
            size_t rlen = (size_t)(pclose - popen - 1);
            if (rlen < sizeof(reg_buf)) {
                strncpy(reg_buf, popen + 1, rlen);
                reg_buf[rlen] = '\0';
            }
            info->reg = (uint8_t)get_register_number(trim(reg_buf));

            if (popen == s) {
                if (is_deferred) {
                    info->mode = 7;
                    info->has_next_word = 1;
                    strcpy(info->word_expr, "0");
                } else {
                    info->mode = 1;
                }
            } else {
                info->mode = is_deferred ? 7 : 6;
                info->has_next_word = 1;
                size_t elen = (size_t)(popen - s);
                if (elen >= sizeof(info->word_expr)) elen = sizeof(info->word_expr) - 1;
                strncpy(info->word_expr, s, elen);
                info->word_expr[elen] = '\0';
            }
            return;
        }
    }

    {
        int r = get_register_number(s);
        if (r != -1) {
            info->mode = is_deferred ? 1 : 0;
            info->reg  = (uint8_t)r;
            return;
        }
    }

    info->mode = is_deferred ? 7 : 6;
    info->reg  = 7;
    info->has_next_word = 1;
    safe_strcpy(info->word_expr, s, sizeof(info->word_expr));
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Директивы — расчёт размера (для прохода 1)
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * word_count_bytes — возвращает суммарный размер в байтах для директивы .WORD
 * с несколькими значениями через запятую (по 2 байта на каждое значение).
 */
static uint16_t word_count_bytes(const char *operands) {
    char ops[32][128];
    int n = split_operands(operands, ops, 32);
    return (uint16_t)(n * 2);
}

/*
 * byte_count_bytes — возвращает количество байт для директивы .BYTE
 * с несколькими значениями через запятую (по 1 байту на каждое значение).
 */
static uint16_t byte_count_bytes(const char *operands) {
    char ops[64][128];
    int n = split_operands(operands, ops, 64);
    return (uint16_t)n;
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Вспомогательная: извлечь строку из операнда (снять кавычки)
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * extract_string — снимает обрамляющие двойные кавычки с операнда строковой
 * директивы (.ASCII/.ASCIZ). Возвращает указатель на статический буфер
 * с содержимым без кавычек.
 */
static const char *extract_string(const char *raw) {
    static char buf[256];
    safe_strcpy(buf, raw, sizeof(buf));
    char *s = trim(buf);
    if (s[0] == '"') s++;
    char *end = strrchr(s, '"');
    if (end) *end = '\0';
    return s;
}

/*
 * is_ignored_directive — проверяет, является ли мнемоника директивой,
 * которую ассемблер игнорирует (.TITLE, .IDENT, .GLOBL и др.).
 * Возвращает 1 если директива игнорируется, 0 иначе.
 */
static int is_ignored_directive(const char *m) {
    static const char *ignored[] = {
        ".TITLE", ".IDENT", ".GLOBL", ".PSECT", ".WEAK",
        ".LIMIT", ".IF",    ".IFF",   ".IFT",   ".IFTF",
        ".ENDC",  ".NLIST", ".LIST",  ".SBTTL", ".RAD50",
        NULL
    };
    for (int i = 0; ignored[i]; i++)
        if (portable_strcasecmp(ignored[i], m) == 0) return 1;
    return 0;
}

static void dump_registers_info(const char *label, uint16_t lc, const ParsedLine *pl) {
    if (!verbose && !quiet) return;
    
    printf("  [%s] LC=%06o", label, lc);
    
    // Информация о метке
    if (pl->label[0]) {
        printf(" LABEL=%s", pl->label);
    }
    
    // Информация об инструкции
    if (pl->mnemonic[0]) {
        printf(" INSTR=%s", pl->mnemonic);
        
        // Попытка определить используемые регистры
        const char *regs[] = {"R0", "R1", "R2", "R3", "R4", "R5", "SP", "PC"};
        for (int i = 0; i < 8; i++) {
            if (strstr(pl->operands_raw, regs[i])) {
                printf(" %s", regs[i]);
            }
        }
    }
    
    printf("\n");
}

// Добавить функцию для анализа операндов и определения регистров
static void analyze_operand_registers(const char *operand) {
    if (!verbose) return;
    
    const char *regs[] = {"R0", "R1", "R2", "R3", "R4", "R5", "SP", "PC"};
    for (int i = 0; i < 8; i++) {
        if (strstr(operand, regs[i])) {
            printf("    использует %s", regs[i]);
        }
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 *  ПЕРВЫЙ ПРОХОД: сбор меток и расчёт размеров
 * ══════════════════════════════════════════════════════════════════════════ */
/*
 * pass1 — первый проход ассемблера. Читает исходный файл построчно,
 * парсит метки и регистрирует их адреса в таблице символов, а также
 * продвигает счётчик адреса LC на размер каждой инструкции/директивы.
 * Машинный код не генерируется. По завершении таблица символов содержит
 * адреса всех меток, включая используемые до определения (forward references).
 */
/*
 * pass1 — первый проход ассемблера. Читает исходный файл построчно,
 * парсит метки и регистрирует их адреса в таблице символов, а также
 * продвигает счётчик адреса LC на размер каждой инструкции/директивы.
 * Машинный код не генерируется. По завершении таблица символов содержит
 * адреса всех меток, включая используемые до определения (forward references).
 */
static void pass1(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        fprintf(stderr, "Ошибка: не удалось открыть '%s'\n", filename);
        error_count++;
        return;
    }

    char       line[512];
    ParsedLine pl;
    OperandInfo oi;
    int symbol_count = 0;

    memset(memory,      0, sizeof(memory));
    memset(memory_used, 0, sizeof(memory_used));
    LC           = 0;
    max_lc       = 0;
    current_line = 0;
    pass_number  = 1;

    if (!quiet) {
        printf("=== ПРОХОД 1 (%s) ===\n", filename);
    }

    while (fgets(line, sizeof(line), f)) {
        current_line++;
        parse_line(line, &pl);

        // Вывод исходной строки в verbose режиме
        if (verbose && !quiet && (pl.label[0] || pl.mnemonic[0])) {
            printf("\n  [строка %d] ", current_line);
            if (pl.label[0]) printf("%s:", pl.label);
            if (pl.mnemonic[0]) printf(" %s", pl.mnemonic);
            if (pl.operands_raw[0]) printf(" %s", pl.operands_raw);
            printf("\n");
        }

        // Выравнивание если инструкция после метки
        if (pl.label[0] && pl.mnemonic[0]) {
            const Instruction *chk = get_instruction(pl.mnemonic);
            if (chk && (LC & 1)) {
                if (verbose && !quiet) {
                    printf("    Выравнивание: %06o -> ", LC);
                }
                LC++;
                if (verbose && !quiet) {
                    printf("%06o\n", LC);
                }
            }
        }

        // Регистрация метки
        if (pl.label[0]) {
            add_symbol(pl.label, LC);
            symbol_count++;
            if (verbose && !quiet) {
                printf("    МЕТКА: %s = %06o\n", pl.label, LC);
            }
        }

        if (!pl.mnemonic[0]) continue;

        if (is_ignored_directive(pl.mnemonic)) {
            if (verbose && !quiet) {
                printf("    Игнорируемая директива: %s\n", pl.mnemonic);
            }
            continue;
        }

        // Обработка директив
        if (portable_strcasecmp(pl.mnemonic, ".ORG") == 0) {
            int ok = 1;
            uint16_t old_lc = LC;
            LC = evaluate_expr(pl.operands_raw, LC, &ok);
            if (verbose && !quiet) {
                printf("    .ORG: %06o -> %06o\n", old_lc, LC);
            }
            continue;
        }
        
        if (portable_strcasecmp(pl.mnemonic, ".EVEN") == 0) {
            if (LC & 1) {
                if (verbose && !quiet) {
                    printf("    .EVEN: выравнивание %06o -> ", LC);
                }
                LC++;
                if (verbose && !quiet) {
                    printf("%06o\n", LC);
                }
            } else if (verbose && !quiet) {
                printf("    .EVEN: уже выровнено\n");
            }
            continue;
        }
        
        if (portable_strcasecmp(pl.mnemonic, ".ODD") == 0) {
            if (!(LC & 1)) {
                if (verbose && !quiet) {
                    printf("    .ODD: выравнивание %06o -> ", LC);
                }
                LC++;
                if (verbose && !quiet) {
                    printf("%06o\n", LC);
                }
            } else if (verbose && !quiet) {
                printf("    .ODD: уже выровнено\n");
            }
            continue;
        }
        
        if (portable_strcasecmp(pl.mnemonic, ".WORD") == 0) {
            if (LC & 1) {
                if (verbose && !quiet) {
                    printf("    .WORD: выравнивание %06o -> ", LC);
                }
                LC++;
                if (verbose && !quiet) {
                    printf("%06o\n", LC);
                }
            }
            int old_lc = LC;
            int bytes = word_count_bytes(pl.operands_raw);
            LC += bytes;
            if (verbose && !quiet) {
                printf("    .WORD: %d байт, адрес %06o -> %06o\n", 
                       bytes, old_lc, LC);
                
                // Показать значения слов
                char ops[32][128];
                int n = split_operands(pl.operands_raw, ops, 32);
                for (int i = 0; i < n; i++) {
                    int ok = 1;
                    uint16_t val = evaluate_expr(ops[i], old_lc + i*2, &ok);
                    printf("      слово %d: %s = %06o\n", i+1, ops[i], val);
                }
            }
            continue;
        }
        
        if (portable_strcasecmp(pl.mnemonic, ".BYTE") == 0) {
            int old_lc = LC;
            int bytes = byte_count_bytes(pl.operands_raw);
            LC += bytes;
            if (verbose && !quiet) {
                printf("    .BYTE: %d байт, адрес %06o -> %06o\n", 
                       bytes, old_lc, LC);
                
                char ops[64][128];
                int n = split_operands(pl.operands_raw, ops, 64);
                for (int i = 0; i < n; i++) {
                    int ok = 1;
                    uint8_t val = (uint8_t)evaluate_expr(ops[i], old_lc + i, &ok);
                    printf("      байт %d: %s = %03o\n", i+1, ops[i], val);
                }
            }
            continue;
        }
        
        if (portable_strcasecmp(pl.mnemonic, ".BLKW") == 0) {
            int ok = 1;
            uint16_t n = evaluate_expr(pl.operands_raw, LC, &ok);
            if (LC & 1) {
                if (verbose && !quiet) {
                    printf("    .BLKW: выравнивание %06o -> ", LC);
                }
                LC++;
                if (verbose && !quiet) {
                    printf("%06o\n", LC);
                }
            }
            int old_lc = LC;
            LC += (uint16_t)(n * 2);
            if (verbose && !quiet) {
                printf("    .BLKW: %d слов, адрес %06o -> %06o\n", n, old_lc, LC);
            }
            continue;
        }
        
        if (portable_strcasecmp(pl.mnemonic, ".BLKB") == 0) {
            int ok = 1;
            int old_lc = LC;
            uint16_t n = evaluate_expr(pl.operands_raw, LC, &ok);
            LC += n;
            if (verbose && !quiet) {
                printf("    .BLKB: %d байт, адрес %06o -> %06o\n", n, old_lc, LC);
            }
            continue;
        }
        
        if (portable_strcasecmp(pl.mnemonic, ".ASCII") == 0 ||
            portable_strcasecmp(pl.mnemonic, ".ASCIZ") == 0) {
            const char *str = extract_string(pl.operands_raw);
            int old_lc = LC;
            int bytes = (int)process_string(str, 0, 0);
            LC += bytes;
            if (portable_strcasecmp(pl.mnemonic, ".ASCIZ") == 0) {
                LC++;
                bytes++;
            }
            if (verbose && !quiet) {
                printf("    %s: %d байт, адрес %06o -> %06o\n", 
                       pl.mnemonic, bytes, old_lc, LC);
                printf("      строка: \"%s\"\n", str);
            }
            continue;
        }
        
        if (portable_strcasecmp(pl.mnemonic, ".END") == 0) {
            if (verbose && !quiet) {
                printf("    .END: остановка на проходе 1\n");
            }
            break;
        }

        // Обработка инструкций
        const Instruction *instr = get_instruction(pl.mnemonic);
        if (!instr) {
            if (!quiet) {
                printf("  Предупреждение: неизвестная инструкция %s (строка %d)\n", 
                       pl.mnemonic, current_line);
            }
            LC += 2;
            continue;
        }

        // Выравнивание перед инструкцией
        if (LC & 1) {
            if (verbose && !quiet) {
                printf("    Выравнивание: %06o -> ", LC);
            }
            LC++;
            if (verbose && !quiet) {
                printf("%06o\n", LC);
            }
        }
        
        int old_lc = LC;
        LC += 2;  // основное слово инструкции

        if (verbose && !quiet) {
            printf("    ИНСТРУКЦИЯ: %s", instr->mnemonic);
        }

        char ops[4][128] = {{0},{0},{0},{0}};
        int nops = split_operands(pl.operands_raw, ops, 4);

        // Подсчет дополнительных слов для операндов
        switch (instr->type) {
        case SINGLE_OP:
        case EIS_OP:
            if (nops >= 1) {
                parse_operand(ops[0], &oi);
                if (oi.has_next_word) {
                    LC += 2;
                    if (verbose && !quiet) {
                        printf(" + доп.слово");
                    }
                }
            }
            break;
        case DOUBLE_OP:
            if (nops >= 1) {
                parse_operand(ops[0], &oi);
                if (oi.has_next_word) {
                    LC += 2;
                    if (verbose && !quiet) {
                        printf(" + доп.слово(src)");
                    }
                }
            }
            if (nops >= 2) {
                parse_operand(ops[1], &oi);
                if (oi.has_next_word) {
                    LC += 2;
                    if (verbose && !quiet) {
                        printf(" + доп.слово(dst)");
                    }
                }
            }
            break;
        case JSR_OP:
        case XOR_OP:
            if (nops >= 2) {
                parse_operand(ops[1], &oi);
                if (oi.has_next_word) {
                    LC += 2;
                    if (verbose && !quiet) {
                        printf(" + доп.слово");
                    }
                }
            }
            break;
        default:
            break;
        }

        if (verbose && !quiet) {
            printf(", размер %d байт, адрес %06o -> %06o\n", 
                   LC - old_lc, old_lc, LC);
            
            // Показать операнды
            for (int i = 0; i < nops; i++) {
                OperandInfo tmp_oi;
                parse_operand(ops[i], &tmp_oi);
                printf("      операнд %d: %s (mode=%d, reg=%d, has_next=%d)\n",
                       i+1, ops[i], tmp_oi.mode, tmp_oi.reg, tmp_oi.has_next_word);
                if (tmp_oi.has_next_word && tmp_oi.word_expr[0]) {
                    printf("        доп.слово: %s\n", tmp_oi.word_expr);
                }
            }
        }
    }

    fclose(f);

    if (!quiet) {
        printf("\n--- ИТОГИ ПРОХОДА 1 ---\n");
        printf("  Найдено меток: %d\n", symbol_count);
        printf("  Размер кода: %u байт (%06o)\n", (unsigned)(LC), LC);
        printf("  Последний адрес: %06o\n", LC);
        
        if (verbose) {
            printf("\n  ТАБЛИЦА СИМВОЛОВ:\n");
            for (Symbol *s = symbol_table; s != NULL; s = s->next) {
                printf("    %-32s %06o\n", s->name, s->address);
            }
            
            // Статистика по типам инструкций
            printf("\n  СТАТИСТИКА:\n");
            printf("    Всего строк: %d\n", current_line);
            printf("    Свободной памяти: %d байт\n", MEMORY_SIZE - LC);
        }
        printf("----------------------\n\n");
    }
}

/* ══════════════════════════════════════════════════════════════════════════
 *  ВТОРОЙ ПРОХОД: генерация кода и листинга
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * emit_instr — выводит строку листинга с адресом, машинным словом и
 * соответствующей строкой исходника в формате "АДРЕС: СЛОВО   исходник".
 */
static void emit_instr(FILE *lst, uint16_t addr, uint16_t word, const char *src) {
    fprintf(lst, "%06o: %06o   %s\n", addr, word, src ? src : "");
}

/*
 * emit_extra — выводит в листинг дополнительное слово-расширение операнда
 * (адрес и значение) с пометкой-тегом в поле исходника.
 */
static void emit_extra(FILE *lst, uint16_t addr, uint16_t word, const char *tag) {
    fprintf(lst, "%06o: %06o   ; %s\n", addr, word, tag ? tag : "");
}

/*
 * pass2 — второй проход ассемблера. Повторно читает исходный файл,
 * генерирует машинный код всех инструкций и директив, записывает байты
 * в массив memory через write_byte/write_word, а также формирует файл
 * листинга (.lst) с адресами, кодами и исходными строками. В конце
 * листинга выводит таблицу символов.
 */
static void pass2(const char *in_filename, const char *lst_filename) {
    FILE *src_f = fopen(in_filename, "r");
    if (!src_f) return;

    FILE *lst = fopen(lst_filename, "w");
    if (!lst) {
        asm_error("Не удалось создать LST-файл");
        fclose(src_f);
        return;
    }

    LC           = 0;
    max_lc       = 0;
    current_line = 0;
    pass_number  = 2;

    fprintf(lst, "; PDP-11 Assembler — Листинг\n");
    fprintf(lst, "; Файл: %s\n", in_filename);
    fprintf(lst, ";\n");
    fprintf(lst, "%-8s %-8s  %s\n", "АДРЕС", "КОД", "ИСХОДНИК");
    fprintf(lst, "%-80s\n", "------------------------------------------------------------------------");

    char       line[512];
    ParsedLine pl;

    while (fgets(line, sizeof(line), src_f)) {
        current_line++;

        char orig[512];
        safe_strcpy(orig, line, sizeof(orig));
        for (char *c = orig; *c; c++) if (*c == '\r' || *c == '\n') { *c = '\0'; break; }

        parse_line(line, &pl);

        if (!pl.mnemonic[0]) {
            if (pl.label[0])
                fprintf(lst, "%06o:          %s\n", LC, orig);
            else
                fprintf(lst, "                  %s\n", orig);
            continue;
        }

        if (is_ignored_directive(pl.mnemonic)) {
            fprintf(lst, "                  %s\n", orig);
            continue;
        }

        if (portable_strcasecmp(pl.mnemonic, ".ORG") == 0) {
            int ok = 1;
            LC = evaluate_expr(pl.operands_raw, LC, &ok);
            fprintf(lst, "           = %06o  %s\n", LC, orig);
            continue;
        }

        if (portable_strcasecmp(pl.mnemonic, ".EVEN") == 0) {
            if (LC & 1) {
                write_byte(LC, 0);
                fprintf(lst, "%06o: 000      %s\n", LC, orig);
                LC++;
            } else {
                fprintf(lst, "                  %s\n", orig);
            }
            continue;
        }

        if (portable_strcasecmp(pl.mnemonic, ".ODD") == 0) {
            if (!(LC & 1)) {
                write_byte(LC, 0);
                fprintf(lst, "%06o: 000      %s\n", LC, orig);
                LC++;
            } else {
                fprintf(lst, "                  %s\n", orig);
            }
            continue;
        }

        if (portable_strcasecmp(pl.mnemonic, ".BYTE") == 0) {
            char ops[64][128];
            int n = split_operands(pl.operands_raw, ops, 64);
            fprintf(lst, "%06o:          %s\n", LC, orig);
            for (int i = 0; i < n; i++) {
                int ok = 1;
                uint8_t v = (uint8_t)evaluate_expr(trim(ops[i]), LC, &ok);
                write_byte(LC, v);
                fprintf(lst, "%06o:   %03o\n", LC, v);
                LC++;
            }
            continue;
        }

        if (portable_strcasecmp(pl.mnemonic, ".WORD") == 0) {
            if (LC & 1) { write_byte(LC, 0); LC++; }
            char ops[32][128];
            int n = split_operands(pl.operands_raw, ops, 32);
            fprintf(lst, "%06o:          %s\n", LC, orig);
            for (int i = 0; i < n; i++) {
                int ok = 1;
                uint16_t v = evaluate_expr(trim(ops[i]), LC, &ok);
                write_word(LC, v);
                fprintf(lst, "%06o: %06o\n", LC, v);
                LC += 2;
            }
            continue;
        }

        if (portable_strcasecmp(pl.mnemonic, ".BLKW") == 0) {
            if (LC & 1) { write_byte(LC, 0); LC++; }
            int ok = 1;
            uint16_t n = evaluate_expr(pl.operands_raw, LC, &ok);
            fprintf(lst, "%06o:          %s\n", LC, orig);
            for (uint16_t i = 0; i < n; i++) {
                write_word(LC, 0);
                LC += 2;
            }
            continue;
        }

        if (portable_strcasecmp(pl.mnemonic, ".BLKB") == 0) {
            int ok = 1;
            uint16_t n = evaluate_expr(pl.operands_raw, LC, &ok);
            fprintf(lst, "%06o:          %s\n", LC, orig);
            for (uint16_t i = 0; i < n; i++) {
                write_byte(LC, 0);
                LC++;
            }
            continue;
        }

        if (portable_strcasecmp(pl.mnemonic, ".ASCII") == 0 ||
            portable_strcasecmp(pl.mnemonic, ".ASCIZ") == 0) {
            const char *str = extract_string(pl.operands_raw);
            fprintf(lst, "%06o:          %s\n", LC, orig);
            uint32_t base = LC;
            size_t cnt = process_string(str, base, 1);
            LC += (uint16_t)cnt;
            if (portable_strcasecmp(pl.mnemonic, ".ASCIZ") == 0) {
                write_byte(LC, 0);
                LC++;
            }
            continue;
        }

        if (portable_strcasecmp(pl.mnemonic, ".END") == 0) break;

        const Instruction *instr = get_instruction(pl.mnemonic);
        if (!instr) {
            char msg[64];
            snprintf(msg, sizeof(msg), "Неизвестная мнемоника: %s", pl.mnemonic);
            asm_error(msg);
            LC += 2;
            continue;
        }

        if (LC & 1) { write_byte(LC, 0); LC++; }

        uint16_t mc = instr->base_opcode;

        char ops[4][128] = {{0},{0},{0},{0}};
        int nops = split_operands(pl.operands_raw, ops, 4);
        (void)nops;

        if (instr->type == NO_OP) {
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;
        }
        else if (instr->type == SPL_OP) {
            int ok = 1;
            uint16_t n = evaluate_expr(ops[0], LC, &ok);
            mc |= (n & 7);
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;
        }
        else if (instr->type == MARK_OP) {
            int ok = 1;
            uint16_t n = evaluate_expr(ops[0], LC, &ok);
            mc |= (n & 077);
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;
        }
        else if (instr->type == EMT_OP) {
            int ok = 1;
            uint16_t n = evaluate_expr(ops[0], LC, &ok);
            mc |= (n & 0377);
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;
        }
        else if (instr->type == RTS_OP) {
            int reg = get_register_number(ops[0]);
            if (reg == -1) { asm_error("RTS: ожидается регистр"); reg = 0; }
            mc |= (uint16_t)reg;
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;
        }
        else if (instr->type == SINGLE_OP) {
            OperandInfo oi;
            parse_operand(ops[0], &oi);
            mc |= (uint16_t)((oi.mode << 3) | oi.reg);
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;
            if (oi.has_next_word) {
                uint16_t w = evaluate_word_for_operand(oi.word_expr, oi.mode, oi.reg, LC + 2);
                write_word(LC, w);
                emit_extra(lst, LC, w, "ext");
                LC += 2;
            }
        }
        else if (instr->type == DOUBLE_OP) {
            OperandInfo src_oi, dst_oi;
            parse_operand(ops[0], &src_oi);
            parse_operand(ops[1], &dst_oi);
            mc |= (uint16_t)((src_oi.mode << 9) | (src_oi.reg << 6) |
                             (dst_oi.mode << 3) |  dst_oi.reg);
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;

            uint16_t src_ext_pc = LC + 2 + (dst_oi.has_next_word ? 2 : 0);
            uint16_t dst_ext_pc = LC + 2 + (src_oi.has_next_word ? 2 : 0) + 2;

            if (src_oi.has_next_word) {
                uint16_t w = evaluate_word_for_operand(src_oi.word_expr, src_oi.mode, src_oi.reg, src_ext_pc);
                write_word(LC, w);
                emit_extra(lst, LC, w, "src-ext");
                LC += 2;
            }
            if (dst_oi.has_next_word) {
                uint16_t w = evaluate_word_for_operand(dst_oi.word_expr, dst_oi.mode, dst_oi.reg, dst_ext_pc);
                write_word(LC, w);
                emit_extra(lst, LC, w, "dst-ext");
                LC += 2;
            }
        }
        else if (instr->type == JSR_OP) {
            int reg = get_register_number(ops[0]);
            if (reg == -1) { asm_error("JSR: первый операнд должен быть регистром"); reg = 0; }
            OperandInfo oi;
            parse_operand(ops[1], &oi);
            mc |= (uint16_t)((reg << 6) | (oi.mode << 3) | oi.reg);
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;
            if (oi.has_next_word) {
                uint16_t w = evaluate_word_for_operand(oi.word_expr, oi.mode, oi.reg, LC + 2);
                write_word(LC, w);
                emit_extra(lst, LC, w, "ext");
                LC += 2;
            }
        }
        else if (instr->type == XOR_OP) {
            int reg = get_register_number(ops[0]);
            if (reg == -1) { asm_error("XOR: первый операнд должен быть регистром"); reg = 0; }
            OperandInfo oi;
            parse_operand(ops[1], &oi);
            mc |= (uint16_t)((reg << 6) | (oi.mode << 3) | oi.reg);
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;
            if (oi.has_next_word) {
                uint16_t w = evaluate_word_for_operand(oi.word_expr, oi.mode, oi.reg, LC + 2);
                write_word(LC, w);
                emit_extra(lst, LC, w, "ext");
                LC += 2;
            }
        }
        else if (instr->type == EIS_OP) {
            OperandInfo oi;
            parse_operand(ops[0], &oi);
            int reg = get_register_number(ops[1]);
            if (reg == -1) { asm_error("EIS: второй операнд должен быть регистром"); reg = 0; }
            mc |= (uint16_t)((reg << 6) | (oi.mode << 3) | oi.reg);
            write_word(LC, mc);
            emit_instr(lst, LC, mc, orig);
            LC += 2;
            if (oi.has_next_word) {
                uint16_t w = evaluate_word_for_operand(oi.word_expr, oi.mode, oi.reg, LC + 2);
                write_word(LC, w);
                emit_extra(lst, LC, w, "ext");
                LC += 2;
            }
        }
        else if (instr->type == BRANCH) {
            uint16_t instr_addr = LC;
            write_word(LC, mc);
            LC += 2;

            int ok = 1;
            uint16_t target = evaluate_expr(ops[0], LC, &ok);
            if (!ok) {
                asm_error("BRANCH: неизвестная метка");
                target = LC;
            }
            int off = (int)((int16_t)(target - LC)) / 2;
            if (off < -128 || off > 127) {
                char msg[128];
                snprintf(msg, sizeof(msg),
                    "BRANCH: метка слишком далеко (offset=%d), допустимо -128..127", off);
                asm_error(msg);
                off = 0;
            }
            mc |= (uint16_t)(off & 0xFF);
            write_word(instr_addr, mc);
            emit_instr(lst, instr_addr, mc, orig);
        }
        else if (instr->type == SOB_OP) {
            int reg = get_register_number(ops[0]);
            if (reg == -1) { asm_error("SOB: первый операнд должен быть регистром"); reg = 0; }

            uint16_t instr_addr = LC;
            write_word(LC, mc);
            LC += 2;

            int ok = 1;
            uint16_t target = evaluate_expr(ops[1], LC, &ok);
            if (!ok) {
                asm_error("SOB: неизвестная метка");
                target = LC;
            }
            int off = (int)((instr_addr + 2) - target) / 2;
            if (off < 0 || off > 63) {
                char msg[128];
                snprintf(msg, sizeof(msg),
                    "SOB: метка должна быть выше и не дальше 63 слов (offset=%d)", off);
                asm_error(msg);
                off = 0;
            }
            mc |= ((uint16_t)reg << 6) | (uint16_t)(off & 077);
            write_word(instr_addr, mc);
            emit_instr(lst, instr_addr, mc, orig);
        }
    }

    fprintf(lst, "\n%s\n", "------------------------------------------------------------------------");
    fprintf(lst, "ТАБЛИЦА СИМВОЛОВ:\n");
    for (Symbol *s = symbol_table; s != NULL; s = s->next)
        fprintf(lst, "  %-32s %06o\n", s->name, s->address);

    fclose(src_f);
    fclose(lst);
}

/* ══════════════════════════════════════════════════════════════════════════
 *  Экспорт в формат SIMH tape (.lda)
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * tape_checksum — вычисляет контрольную сумму блока SIMH tape: однобайтовое
 * дополнение до двух от суммы всех байт заголовка (block_len, addr) и данных.
 */
static uint8_t tape_checksum(uint16_t block_len, uint16_t addr,
                             const uint8_t *data, int data_len) {
    uint8_t sum = 0;
    sum += (uint8_t)(block_len & 0xFF);
    sum += (uint8_t)(block_len >> 8);
    sum += (uint8_t)(addr & 0xFF);
    sum += (uint8_t)(addr >> 8);
    for (int i = 0; i < data_len; i++) sum += data[i];
    return (uint8_t)((~sum + 1) & 0xFF);
}

/*
 * write_tape_block — записывает один блок в формате SIMH tape: нулевой
 * разделитель, маркер 0x01 0x00, длину блока, адрес загрузки, контрольную
 * сумму и данные. Нулевой разделитель позволяет симулятору найти маркер
 * при пропуске 0x00-байт.
 */
static void write_tape_block(FILE *f, uint16_t addr,
                             const uint8_t *data, int data_len) {
    uint16_t block_len = (uint16_t)(data_len + 6);
    uint8_t  csum      = tape_checksum(block_len, addr, data, data_len);

    uint8_t pad = 0x00;
    fwrite(&pad, 1, 1, f);

    uint8_t marker[2] = { 0x01, 0x00 };
    fwrite(marker,     1, 2, f);
    fwrite(&block_len, 2, 1, f);
    fwrite(&addr,      2, 1, f);
    fwrite(&csum,      1, 1, f);
    if (data_len > 0)
        fwrite(data, 1, (size_t)data_len, f);
}

#define MAX_BLOCK_DATA 250

/*
 * export_tape — сохраняет содержимое массива memory в файл формата SIMH tape
 * (.lda). Занятые непрерывные участки памяти разбиваются на блоки не более
 * MAX_BLOCK_DATA байт. В конце записывается финальный блок с нулевыми данными,
 * задающий стартовый адрес программы.
 */
static void export_tape(const char *out_filename, uint16_t start_addr) {
    FILE *f = fopen(out_filename, "wb");
    if (!f) {
        fprintf(stderr, "Ошибка: не удалось создать '%s'\n", out_filename);
        return;
    }
    
    int blocks_written = 0;
    uint32_t pos = 0;
    
    while (pos < MEMORY_SIZE) {
        // Пропускаем пустые байты
        while (pos < MEMORY_SIZE && !memory_used[pos]) {
            pos++;
        }
        if (pos >= MEMORY_SIZE) break;
        
        uint32_t start = pos;
        uint32_t end = start;
        
        // Блок не больше 250 байт
        while (end < MEMORY_SIZE && memory_used[end] && (end - start) < 250) {
            end++;
        }
        
        int data_len = end - start;
        
        // Блок с данными
        uint8_t marker[2] = {0x01, 0x00};
        uint16_t block_len = data_len + 6;
        
        // Контрольная сумма
        uint8_t csum = 0;
        csum += (uint8_t)(block_len & 0xFF);
        csum += (uint8_t)(block_len >> 8);
        csum += (uint8_t)(start & 0xFF);
        csum += (uint8_t)(start >> 8);
        for (int i = 0; i < data_len; i++) {
            csum += memory[start + i];
        }
        csum = (uint8_t)((~csum + 1) & 0xFF);
        
        fwrite(marker, 1, 2, f);
        fwrite(&block_len, 2, 1, f);
        fwrite(&start, 2, 1, f);
        fwrite(&csum, 1, 1, f);
        fwrite(&memory[start], 1, data_len, f);
        
        blocks_written++;
        pos = end;
    }
    
    // Финальный блок со стартовым адресом
    uint8_t marker[2] = {0x01, 0x00};
    uint16_t block_len = 6;
    uint8_t csum = 0;
    csum += (uint8_t)(block_len & 0xFF);
    csum += (uint8_t)(block_len >> 8);
    csum += (uint8_t)(start_addr & 0xFF);
    csum += (uint8_t)(start_addr >> 8);
    csum = (uint8_t)((~csum + 1) & 0xFF);
    
    fwrite(marker, 1, 2, f);
    fwrite(&block_len, 2, 1, f);
    fwrite(&start_addr, 2, 1, f);
    fwrite(&csum, 1, 1, f);
    
    fclose(f);
    
    if (!quiet) {
        printf("  Записано блоков: %d\n", blocks_written);
    }
}
/* ══════════════════════════════════════════════════════════════════════════
 *  main
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * strip_ext — удаляет расширение файла из строки пути (от последней точки,
 * которая стоит после последнего разделителя каталогов).
 */
static void strip_ext(char *s) {
    char *slash = strrchr(s, '/');
    if (!slash) slash = strrchr(s, '\\');
    char *dot   = strrchr(s, '.');
    if (dot && (!slash || dot > slash)) *dot = '\0';
}

/*
 * print_usage — выводит на стандартный вывод краткую справку по использованию
 * ассемблера: синтаксис команды, описание ключей и примеры вызова.
 */
static void print_usage(const char *prog) {
    printf("PDP-11 Assembler — совместим с PDP-11-simulator\n\n");
    printf("Использование:\n");
    printf("  %s [ключи] <файл.asm>\n\n", prog);
    printf("Ключи:\n");
    printf("  -o <файл>   Базовое имя выходных файлов (.lda / .lst)\n");
    printf("  -s <addr>   Стартовый адрес (восьмеричный, по умолчанию 01000)\n");
    printf("  -v          Подробный вывод\n");
    printf("  -h          Эта справка\n\n");
    printf("Примеры:\n");
    printf("  %s prog.asm\n", prog);
    printf("  %s -o out -s 02000 prog.asm\n", prog);
}

/*
 * main — точка входа. Разбирает аргументы командной строки (-o, -s, -v, -h),
 * формирует имена выходных файлов, запускает pass1 и pass2, при отсутствии
 * ошибок вызывает export_tape для записи .lda-файла и выводит итоговую статистику.
 */
int main(int argc, char *argv[]) {
    char in_file[256]  = {0};
    char out_base[256] = {0};
    uint16_t start_addr = 01000;
    int verbose = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 'o':
                if (i + 1 < argc) safe_strcpy(out_base, argv[++i], sizeof(out_base));
                break;
            case 's':
                if (i + 1 < argc)
                    start_addr = (uint16_t)strtoul(argv[++i], NULL, 8);
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                fprintf(stderr, "Неизвестный ключ: %s\n", argv[i]);
                break;
            }
        } else {
            if (!in_file[0]) safe_strcpy(in_file, argv[i], sizeof(in_file));
        }
    }

    if (!in_file[0]) {
        print_usage(argv[0]);
        return 1;
    }

    if (!out_base[0]) {
        safe_strcpy(out_base, in_file, sizeof(out_base));
        strip_ext(out_base);
    }

    char lst_file[300], lda_file[300];
    snprintf(lst_file, sizeof(lst_file), "%s.lst", out_base);
    snprintf(lda_file, sizeof(lda_file), "%s.lda", out_base);

    if (verbose) {
        printf("Входной файл:    %s\n", in_file);
        printf("Листинг:         %s\n", lst_file);
        printf("Выходной модуль: %s\n", lda_file);
        printf("Стартовый адрес: %06o\n\n", start_addr);
    }

    pass1(in_file);
    if (error_count) {
        printf("Проход 1 завершён с ошибками (%d). Прерывание.\n", error_count);
        free_symbol_table();
        return 1;
    }

    if (verbose) {
        printf("Символов найдено: ");
        int cnt = 0;
        for (Symbol *s = symbol_table; s; s = s->next) cnt++;
        printf("%d\n\n", cnt);
    }

    printf("=== Проход 2 ===\n");
    current_line = 0;
    pass2(in_file, lst_file);

    if (verbose && error_count == 0) {
        printf("\nТаблица символов:\n");
        for (Symbol *s = symbol_table; s; s = s->next)
            printf("  %-32s %06o\n", s->name, s->address);
        printf("\n");
    }

    if (error_count == 0) {
        export_tape(lda_file, start_addr);
        printf("\n✓ Успех!\n");
        printf("  Выходной модуль : %s\n", lda_file);
        printf("  Листинг         : %s\n", lst_file);
        printf("  Стартовый адрес : %06o\n", start_addr);
        printf("  Размер кода     : %u байт\n", (unsigned)(max_lc + 1));
    } else {
        printf("\nСборка завершена с ошибками: %d\n", error_count);
    }

    free_symbol_table();
    return (error_count == 0) ? 0 : 1;
}
