#include <stdio.h>
#include <unistd.h>

int main(void) {
    const char *frames[] = {"-", "//", "|", "/"};
    for (int i = 0;  ; i = (i + 1)%4) {
        printf("\rLoading...%s", frames[i]);
        fflush(stdout);
        usleep(100000); // 0.1초 대기 
    }
}
