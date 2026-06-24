#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>


static struct  termios old_termios;     //настройки старого терминала
static int terminal_initialized = 0;

static int last_char = -1;
static int cahr_available = 0;


// Инициализация терминала

void terminal_init (void){
    if (terminal_init)
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

