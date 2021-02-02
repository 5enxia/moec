#include<cstdlib>
#include<cctype>
#include<cstdio>
#include<cerrno>

#include<unistd.h>
#include<termios.h>

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios canonical;
int err;

void die(const char *s);
void enableRawMode();
void disableRawMode();
char editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();

int main(int argc, const char *argv[])
{
    enableRawMode();

    while(1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}

// Error
void die(const char *s) {
    editorRefreshScreen();
    perror(s);
    exit(1);
}

// Enable raw mode (non canonical mode)
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

// Set termial attribute
void disableRawMode() {
    err = tcsetattr(STDIN_FILENO, TCSAFLUSH, &canonical);
    if (err < 0) die("tcsetattr");
}

// key input
char editorReadKey() {
    int nread;
    char c;
    // Doesn't work bellow comment out code...
    // nread = read(STDIN_FILENO, &c, 1);
    // while (nread != 1) {
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

void editorProcessKeypress() {
    char c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
            editorRefreshScreen();
            exit(0);
            break;
    }
}

// Refresh screen
void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3); // initialize cursor position
}
