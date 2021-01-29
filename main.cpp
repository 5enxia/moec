#include<cstdlib>
#include<cctype>
#include<cstdio>
#include<cerrno>

#include<unistd.h>
#include<termios.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios canonical;
int err;

void die(const char *s) {
    perror(s);
    exit(1);
}

void enableRawMode();
void disableRawMode();
char editorReadKey();

int main(int argc, const char *argv[])
{
    enableRawMode();

    while(1) {
        char c = '\0';

        err = read(STDIN_FILENO, &c, 1);
        if (err < 0) die("read");

        if (iscntrl(c)) { // Control char ex:ESC, return etc...
            printf("%d\r\n", c);
        } else {
            printf("%d ('%c')\r\n", c, c);
        }
        if (c == CTRL_KEY('q')) break;
    }
    return 0;
}

void enableRawMode() {
    err = tcgetattr(STDIN_FILENO, &canonical); // get terminal attribute
    if (err < 0) die("tcgetattr");

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

    err = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // set termial attribute
    if (err < 0) die("tcsetattr");
}

void disableRawMode() {
    // Set termial attribute
    err = tcsetattr(STDIN_FILENO, TCSAFLUSH, &canonical);
    if (err < 0) die("tcsetattr");
}

char editorReadKey() {
    int nread;
    char c;
}
