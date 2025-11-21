#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
/*
 * 명령어 줄에서 반드시 1개의 string 아규먼트 받는다
 * parent는 child를 fork해서 서로 파이프로 통신한다 
 * parent와 child는 각자가 받은 내용을 즉시 출력한다(pipe 2개 생성?)
 */

int main(int ac, char **argv) {
    if (ac != 2) { /* 아규먼트 스트링 1개 예외처리 */
        fprintf(stderr, "Enter one string.\n");
        exit(1);
    }
    char* param = argv[1];
    int p_to_c[2], c_to_p[2]; // 파이프
    char buffer[BUFSIZ]; // 버퍼 

    /* 0: read 끝, 1: write */
    if (pipe(p_to_c) == -1 || pipe(c_to_p) == -1) { // 파이프 생성 X 에러처리
        perror("pipe creation failed");
        exit(1);
    }

    pid_t pid = fork(); /* fork */

    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) { /* 자식 프로세스 스코프 */
        close(c_to_p[0]); // 안 쓰는 파이프 I/O 닫기
        close(p_to_c[1]);
        /* child가 먼저 부모에게 아규먼트를 전달한다 */
        if (write(c_to_p[1], param, strlen(param) + 1) == -1) {
            perror("write");
            exit(1);
        }
        /* 부모가 ++한 아규먼트를 수신한다 */
        ssize_t bytes_read = read(p_to_c[0], buffer, BUFSIZ);

        if (bytes_read > 0) {
            /* 수신한 아규먼트 출력 */
            fprintf(stdout, "child: %s\n", buffer);
        }

        for (int i = 0; i < strlen(buffer); i++) {
            buffer[i] += 2; /* 다음 아스키 문자로 번경 */
        }

        /* 다시 부모에게 전달 */
        if (write(c_to_p[1], buffer, strlen(buffer) + 1) == -1) {
            perror("write");
            exit(1);
        }
        exit(0); /* 전달이 끝나면 종료한다 */
    } else { /* 부모 프로세스 */
        close(p_to_c[0]); // 안 쓰는 파이프 I/O 닫기 
        close(c_to_p[1]);

        ssize_t bytes_read = read(c_to_p[0], buffer, BUFSIZ); /* 파이프에서 버퍼로 읽어온다 */
        if (bytes_read > 0) {
            fprintf(stdout, "parent: %s\n", buffer);
        }
        for (int i = 0; i < strlen(buffer); i++) {
            buffer[i]++; /* 다음 아스키 문자로 번경 */
        }
        /* 자식 프로세스에게 전달 */
        if (write(p_to_c[1], buffer, strlen(buffer) + 1) == -1) {
            perror("write");
            exit(1);
        }
        /* 다시 읽기 */ 
        bytes_read = read(c_to_p[0], buffer, BUFSIZ);
        /* 읽어 온 데이터 출력 */
        if (bytes_read > 0) {
            fprintf(stdout, "parent: %s\n", buffer);
        }
    }
}

