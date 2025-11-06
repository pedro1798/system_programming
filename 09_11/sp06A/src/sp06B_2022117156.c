#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/* 
 * - 명령어 줄에는 명령어 외의 1개의 인자만 입력받을 수 있다.그 외의 경우는 오류로 간주한다.
 * - 명령어 줄의 1개의 인자에 대한 명령어 수행에 대한 결과를 출력한다.
 * - 해당 명령어가 존재하지 않는 경우에는 오류메시지를 출력해야 한다.
 */

int main(int argc, char* argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Too many argument. Please type only one.");
        exit(1);
    }
    execvp(argv[1], &argv[1]);
    perror("execvp failed");
    exit(1);
}
