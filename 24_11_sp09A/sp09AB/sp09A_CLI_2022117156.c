#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024

void oops(char *message);

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    char message[BUF_SIZE];
    int str_len;

    // 인자 개수 확인 (실행파일 + ServerIP + Port + Name)
    if(argc != 4){
        printf("Usage : %s <Server IP> <port> <Client Name>\n", argv[0]);
        exit(1);
    }

    // 1. 소켓 생성
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
        oops("socket() error");

    // 2. 서버 주소 구조체 초기화
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; // IPv4
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // port # 
    serv_addr.sin_port = htons(atoi(argv[2])); //IPv4 address

    // 3. Connect (서버 접속 시도)
    // sockaddr_in serv addr 을 (struct sockaddr*) type로 casting
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        oops("connect() error");
    else
        fprintf(stdout, "Connected to Server.\n");

    // 4. 접속 직후 이름 정보 전송 
    sprintf(message, "I am %s", argv[3]);
    write(sock, message, strlen(message));
    
    // 이름 전송에 대한 응답 읽기
    str_len = read(sock, message, BUF_SIZE - 1);
    if (str_len != -1) {
        message[str_len] = 0;
        printf("%s\n", message);
    }

    // 5. 입력 및 송수신 루프
    while (1)
    {
        /* 특정 파일 스트림에 문자열을 쓰는 함수. file put string의 약자 */
        // fputs("Input message(EXIT to quit): ", stdout);
        fgets(message, BUF_SIZE, stdin);

        // "EXIT" 입력 확인 (개행 문자 제거 후 비교)
        // fgets는 엔터키(\n)까지 포함하므로 처리 필요
        int len = strlen(message);
        if (len > 0 && message[len-1] == '\n') {
            message[len-1] = 0; // 개행 제거
        }

        if(!strcmp(message, "EXIT")) {
            fprintf(stdout, "Program finished.\n");
            break;
        }

        strcat(message, "\n"); 
        
        write(sock, message, strlen(message));

        // 서버로부터 에코 수신
        str_len = read(sock, message, BUF_SIZE - 1);
        if(str_len == -1) 
            oops("read() error");
        
        message[str_len] = 0;
        printf("%s", message);
    }

    close(sock);
    return 0;
}

void oops(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
