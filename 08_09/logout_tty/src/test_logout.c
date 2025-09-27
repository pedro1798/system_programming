#include <stdio.h>

int logout_tty(char *line);

int main(int argc, char *argv[]) {
    if (argc != 2) { /* 매개변수 안 들어왔을 때 처리 */
        fprintf(stderr, "Usage: %s <tty_name>\n", argv[0]);
        return 1;
    }

    // 첫 번째 인자를 터미널 이름으로 전달
    int result = logout_tty(argv[1]);

    if (result == 0) {
        printf("Successfully logged out terminal: %s\n", argv[1]);
    } else {
        printf("Failed to log out terminal: %s\n", argv[1]);
    }

    return 0;
}
