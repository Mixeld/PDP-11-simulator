#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "terminal.h"


static struct  termios old_termios;     //настройки старого терминала
static int terminal_initialized = 0;

static int last_char = -1;
static int char_available = 0;

void terminal_init (void){
    if (terminal_initialized)
        return;

    tcgetattr (STDIN_FILENO, &old_termios);     //сохранение текущих настроек терминала


    struct termios new_termios = old_termios;
    new_termios.c_lflag &= ~ (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    terminal_initialized = 1;
} 

void terminal_cleanup(void){
    if (!terminal_initialized)
        return;
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);

    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl (STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    terminal_initialized = 0;
    
}

static void check_keyboard (void) {
    if (char_available)
        return;

    char c;
    int n = read(STDIN_FILENO, &c, 1);
    
    static int counter = 0;
    counter++;
    if (counter % 100000 == 0) {
        fprintf(stderr, "[DEBUG] check_keyboard вызвана %d раз, n=%d\n", counter, n);
    }
    
    if (n == 1){
        fprintf(stderr, "[DEBUG] ПОЛУЧЕН СИМВОЛ: %d ('%c')\n", c, c);
        last_char = (unsigned char) c;
        char_available = 1;
    }
}

uint16_t terminal_read_tks(void){

    check_keyboard();

    if (char_available)
        return TERMINAL_READY;
    else
        return 0; 
}

uint16_t terminal_read_tkb (void){
    check_keyboard();

    if (char_available) {
        uint16_t result = (uint16_t)last_char;
        char_available = 0;   
        return result;
    }

    return 0;
}

uint16_t terminal_read_tps(void){
    return TERMINAL_READY;
}


//Запись в принтер
void terminal_write_tpb(uint16_t val) {
    char c = (char)(val & 0xFF);
    putchar(c);
    fflush(stdout);
}

// проверка адресов

int terminal_is_io_address(uint16_t addr){
    return (addr >= TKS && addr <= TPB + 1);
}