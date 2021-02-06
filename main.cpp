#include<cstdlib>
#include<cctype>
#include<cstdio>
#include<cerrno>
#include<cstring>

#include<unistd.h>
#include<termios.h>
#include<sys/ioctl.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}
#define VERSION "1.0.0"

struct abuf { // append buffer
    char *b;
    int len;
};

struct editorConfig { // editor params
    int cx, cy;
    int screenrows;
    int screencols;
    struct termios orig_termios;
};

// debug mode
bool debug = false;

// err
int err;
void die(const char *s);

// control canonical
void enableRawMode();
void disableRawMode();

// editor
struct editorConfig E;
void initEditor();
char editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
void editorDrawRows(struct abuf *ab);
int getWindowSize(int *rows, int *cols);
int getCursorPosition(int *rows, int *cols);

// append buffer
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

int main(int argc, char * const argv[]) {
    // parse option
    int opt;
    while((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                debug = true;
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
    E.cx = 0;
    E.cy = 0;

    err = getWindowSize(&E.screenrows, &E.screencols);
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
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6); // hide cursor
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[?25h", 6); // show cursor

    write(STDOUT_FILENO, ab.b, ab.len); // cursor pos 0,0
    abFree(&ab);
}


void editorDrawRows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        if (y == E.screenrows / 2) {
            // welcome msg
            char msg[80];
            int msglen = snprintf(msg, sizeof(msg), "moec: v%s", VERSION);
            if (msglen > E.screencols) msglen = E.screencols;
            int padding = (E.screencols - msglen) / 2;
            abAppend(ab, "~", 1);
            padding--;
            while (padding--) abAppend(ab, " ", 1);
            abAppend(ab, msg, msglen);
        } else {
            abAppend(ab, "~", 1);
        }

        abAppend(ab, "\x1b[K", 3); // erase line
        if (y < E.screencols - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    err = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    if (debug || err == -1 || ws.ws_col == 0){
        err = write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12);
        if (err != 12) return -1;
        return getCursorPosition(rows, cols);
    }
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    err = write(STDOUT_FILENO, "\x1b[6n", 4);
    if (err != 4) return -1;

    while (i < sizeof(buf) - 1) {
        err = read(STDIN_FILENO, &buf[i], 1);
        if (err != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *newab = (char*)realloc(ab->b, ab->len + len);
    if (newab == NULL) return;
    memcpy(&newab[ab->len], s, len);
    ab->b = newab;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}
