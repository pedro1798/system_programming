#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>

int main(void) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    printf("Rows: %d, Cols: %d\n", w.ws_row, w.ws_col);
    return 0;
}
