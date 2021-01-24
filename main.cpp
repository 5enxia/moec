#include<cstdlib>
#include<cctype>
#include<cstdio>

#include<unistd.h>
#include<termios.h>

struct termios canonical;

void enableRawMode();
void disableRawMode();

int main(int argc, const char *argv[])
{
    enableRawMode();
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        if (iscntrl(c)) { // Control char ex:ESC, return etc...
            printf("%d\n", c);
        } else {
            printf("%d ('%c')\n", c, c);
        }
    }
    return 0;
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &canonical); // get terminal attribute
    atexit(disableRawMode); // exec when exit
    struct termios raw = canonical;
    raw.c_lflag &= ~(IXON); // Disable software control
    raw.c_lflag &= ~(ECHO | ICANON | ISIG); // Disable echo | raw mode | SIGINT, SIGTSTP
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // set termial attribute
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &canonical); // set termial attribute
}
