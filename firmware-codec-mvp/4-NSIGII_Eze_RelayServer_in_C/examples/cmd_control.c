#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <ctype.h>

// Enable raw mode to read keypresses one-by-one (no line buffering)
void enable_raw_mode(struct termios *orig) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, orig);
    raw = *orig;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode(struct termios *orig) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig);
}

int main() {
    struct termios orig_termios;
    enable_raw_mode(&orig_termios);

    printf("NSIGII Command & Control Interpreter (non-relay mode)\n");
    printf("Type normally to relay | Use arrows/numpad for commands | Ctrl+C to quit\n\n");

    int c;
    while (1) {
        c = getchar();

        if (c == EOF || c == 4) break;  // Ctrl+D or Ctrl+C

        // Detect ANSI escape sequences (arrows)
        if (c == 27 && getchar() == '[') {  // \e[
            int code = getchar();
            switch (code) {
                case 'A': printf("EXEC: UP\n");    break;
                case 'B': printf("EXEC: DOWN\n");  break;
                case 'C': printf("EXEC: RIGHT\n"); break;
                case 'D': printf("EXEC: LEFT\n");  break;
                default:  printf("UNKNOWN ESC SEQ: %d\n", code); break;
            }
        }
        // Numpad fallback (NumLock on) - common in terminals
        else if (c >= '0' && c <= '9') {
            switch (c) {
                case '8': printf("EXEC: UP (numpad)\n");    break;
                case '2': printf("EXEC: DOWN (numpad)\n");  break;
                case '6': printf("EXEC: RIGHT (numpad)\n"); break;
                case '4': printf("EXEC: LEFT (numpad)\n");  break;
                case '5': printf("EXEC: SELECT/START\n");   break;
                case '0': printf("EXEC: A BUTTON\n");       break;
                case '1': printf("EXEC: B BUTTON\n");       break;
                default:  putchar(c); printf(" (relayed num)\n"); break;
            }
        }
        // Normal printable text: relay/echo
        else if (isprint(c)) {
            putchar(c);
            printf(" (relayed)\n");
        }
        else {
            printf("RAW KEY: %d\n", c);
        }
        fflush(stdout);
    }

    disable_raw_mode(&orig_termios);
    return 0;
}
