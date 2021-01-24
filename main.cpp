#include<unistd.h>
#include<termios.h>

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
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw); // get terminal attribute
    raw.c_lflag &= ~(ECHO); // change to raw(non canonical) mode
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // set termial attribute
}
