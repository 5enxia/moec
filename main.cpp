#include<cstdlib>
#include<cctype>
#include<cstdio>
#include<cerrno>

#include<unistd.h>
#include<termios.h>
#include<sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)

// debug
bool debug = false;

// err
int err;
void die(const char *s);

// control canonical
void enableRawMode();
void disableRawMode();

// editor
struct editorConfig {
    int screenrows;
    int screencols;
    struct termios orig_termios;
};
struct editorConfig E;
void initEditor();
char editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
void editorDrawRows();
int getWindowSize(int &rows, int &cols);
int getCursorPosition(int &rows, int &cols);

int main(int argc, char * const argv[]) {
    // parse option
    int opt;
    while((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                debug = 1;
                break;
        }
    }

    enableRawMode();
    initEditor();

    while(1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}

void die(const char *s) { // Error
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3); // cursor pos 0,0
    perror(s);
    exit(1);
}

void enableRawMode() { // Enable raw mode (non canonical mode)
    err = tcgetattr(STDIN_FILENO, &E.orig_termios); // get terminal attribute
    if (err < 0) die("tcgetattr");

    atexit(disableRawMode); // exec when exit

    struct termios raw = E.orig_termios;
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

void disableRawMode() { // Set termial attribute
    err = tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios);
    if (err < 0) die("tcsetattr");
}

void initEditor() {
    err = getWindowSize(E.screenrows, E.screencols);
    if (err == -1) die("getWindowSize");
}

char editorReadKey() { // key input
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
            write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
            write(STDOUT_FILENO, "\x1b[H", 3); // cursor pos 0,0
            exit(0);
            break;
    }
}

void editorRefreshScreen() { // Refresh screen
    write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3); // cursor pos 0,0
    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3); // cursor pos 0,0
}


void editorDrawRows() {
    int y;
    for (y = 0; y < E.screenrows; y++) write(STDOUT_FILENO, "~\r\n", 3);
}

int getWindowSize(int &rows, int &cols) {
    struct winsize ws;
    err = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    // bool debug = false;
    if (debug || err == -1 || ws.ws_col == 0){
        err = write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12);
        if (err != 12) return -1;
        return getCursorPosition(rows, cols);
    }
    cols = ws.ws_col;
    rows = ws.ws_row;
    return 0;
}

int getCursorPosition(int &rows, int &cols) {
    err = write(STDOUT_FILENO, "\x1b[6n", 4);
    if (err != 4) return -1;
    printf("\r\n");
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (iscntrl(c)) printf("%d\r\n", c);
        printf("%d ('%c')\r\n", c, c);
    }
    editorReadKey();
    return -1;
}
