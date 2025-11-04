#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

void progress_bar(double progress) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int width = w.ws_col - 10; // 남는 공간 고려

    int filled = (int)(progress * width);
    printf("\r[");
    for (int i = 0; i < width; i++)
        putchar(i < filled ? '#' : ' ');
    printf("] %3d%%", (int)(progress * 100));
    fflush(stdout);
}

int main() {
    for (int i = 0; i <= 100; i++) {
        progress_bar(i / 100.0);
        usleep(50000);
    }
    printf("\n");
    return 0;
}
