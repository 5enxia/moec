#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include<cstdlib>
#include<cctype>
#include<cstdio>
#include<cerrno>
#include<cstring>

#include<unistd.h>
#include<termios.h>

#include<sys/ioctl.h>
#include<sys/types.h>

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}
#define VERSION "1.0.0"

enum editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

typedef struct erow {
    int size;
    char *chars;
} erow;

struct abuf { // append buffer
    char *b;
    int len;
};

struct editorConfig {
    int cx, cy;
    int rowoff;
    int screenrows;
    int screencols;
    int numrows;
    erow *row;
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
int editorReadKey();
void editorProcessKeypress();
void editorRefreshScreen();
void editorDrawRows(struct abuf *ab);
void editorMoveCursor(int key);
void editorOpen(char *filename); // FILE IO
void editorAppendRow(char *s, size_t len);
void editorScroll();

// screen
int getWindowSize(int *rows, int *cols);
int getCursorPosition(int *rows, int *cols);

// append buffer
void abAppend(struct abuf *ab, const char *s, int len);
void abFree(struct abuf *ab);

// option
void parseOption(int argc, char * const argv[]) {
    // parse option
    int opt;
    while((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                debug = true;
                break;
        }
    }
}

int main(int argc, char * const argv[]) {
    parseOption(argc, argv);

    enableRawMode();
    initEditor();
    if (argc >= 2) editorOpen(argv[1]);
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
    E.rowoff = 0;
    E.numrows = 0;
    E.row = NULL;

    err = getWindowSize(&E.screenrows, &E.screencols);
    if (err == -1) die("getWindowSize");
}

int editorReadKey() { // key input
    int nread;
    char c;
    // Doesn't work bellow comment out code...
    // nread = read(STDIN_FILENO, &c, 1);
    // while (nread != 1) {
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b') {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == '0') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
    }
    return c;
}

void editorProcessKeypress() {
    int c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
            write(STDOUT_FILENO, "\x1b[H", 3); // cursor pos 0,0
            exit(0);
            break;

         case HOME_KEY:
            E.cx = 0;
            break;

         case END_KEY:
            E.cx = E.screencols - 1;
            break;

         case PAGE_UP:
         case PAGE_DOWN:
            {
                int times = E.screenrows;
                while (times--) editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            }

         case ARROW_UP:
         case ARROW_DOWN:
         case ARROW_LEFT:
         case ARROW_RIGHT:
            editorMoveCursor(c);
    }
}

void editorRefreshScreen() { // Refresh screen
    editorScroll();

    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6); // hide cursor

    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1); // cursor
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[?25h", 6); // show cursor

    write(STDOUT_FILENO, ab.b, ab.len); // cursor pos 0,0
    abFree(&ab);
}


void editorDrawRows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        int filerow = y + E.rowoff;
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows / 2) {
                // welcome msg
                char msg[80];
                int msglen = snprintf(msg, sizeof(msg), "moec: v%s", VERSION);
                if (msglen > E.screencols) msglen = E.screencols;
                int padding = (E.screencols - msglen) / 2;
                if (padding) {
                    abAppend(ab, "~", 1);
                    padding--;
                }
                while (padding--) abAppend(ab, " ", 1);
                abAppend(ab, msg, msglen);
            } else {
                abAppend(ab, "~", 1);
            }
        } else {
            int len = E.row[filerow].size;
            if (len > E.screencols) len = E.screencols;
            abAppend(ab, E.row[filerow].chars, len);
        }

        abAppend(ab, "\x1b[K", 3); // erase line
        if (y < E.screenrows - 1) {
            abAppend(ab, "\r\n", 2);
        }
    }
}

void editorMoveCursor(int key) {
    switch (key) {
        case ARROW_LEFT:
            if (0 < E.cx) E.cx--;
            break;
        case ARROW_RIGHT:
            if (E.cx < E.screencols - 1) E.cx++;
            break;
        case ARROW_UP:
            if (0 < E.cy) E.cy--;
            break;
        case ARROW_DOWN:
            if (E.cy < E.screenrows - 1) E.cy++;
            E.cy++;
            break;
    }
}

void editorOpen(char *filename) { // FILE IO
    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
     while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
        editorAppendRow(line, linelen);
    }
    free(line);
    fclose(fp);
}

void editorAppendRow(char *s, size_t len) {
    E.row = (erow*)realloc(E.row, sizeof(erow) * (E.numrows + 1));
    int at = E.numrows; // line number
    E.row[at].size = len;
    E.row[at].chars = (char*)malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.numrows++;
}

void editorScroll() {
    if (E.cy < E.rowoff) E.rowoff = E.cy;
    if (E.cy >= E.rowoff + E.screenrows) E.rowoff = E.cy - E.screenrows + 1;
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
