#include<cstdlib>

#include<unistd.h>
#include<termios.h>

struct termios canonical;

void enableRawMode();
void disableRawMode();

int main(int argc, const char *argv[])
{
    enableRawMode();
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &canonical); // get terminal attribute
    atexit(disableRawMode); // exec when exit
    struct termios raw = canonical;
    raw.c_lflag &= ~(ECHO | ICANON); // change to raw(non canonical) mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // set termial attribute
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &canonical); // set termial attribute
}
