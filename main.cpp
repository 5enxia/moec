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

    while(1) {
        char c = '\0';
        read(STDIN_FILENO, &c, 1);
        if (iscntrl(c)) { // Control char ex:ESC, return etc...
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
        if (c == 'q') break;
    }
    return 0;
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &canonical); // get terminal attribute
    atexit(disableRawMode); // exec when exit

    struct termios raw = canonical;
    // Character size
    raw.c_cflag |= (CS8);
    // Disable
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON); // return | software control
    raw.c_oflag &= ~(OPOST); // output
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN); // echo | raw mode | SIGINT, SIGTSTP | waiting another type
    // min value of bytes of input needed before read()
    raw.c_cc[VMIN] = 0; // 0 char
    // time of waiting user
    raw.c_cc[VTIME] = 1; // * 100 millis

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // set termial attribute
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &canonical); // set termial attribute
}
