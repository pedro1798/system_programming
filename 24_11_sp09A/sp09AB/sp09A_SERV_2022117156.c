#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUF_SIZE 1024
#define BACKLOG 1

/*
 * struct sockaddr: 범용 구조체, bind(), connect() 같은 소켓 함수들이 인자로 요구하는 표준 규격 
 * 주소체계(2바이트) + 나버지 주소 정보(14바이트) -> 14바이트 안에 포트, IP주소 넣기 힘듦
 * struct sockaddr_in: IPv4 전용 구조체  
 * 주소쳬계 + 포트(2바이트) + IP주소(4바이트) + 패딩(8바이트)
 * IPv4 쓸 땐 sockaddr_in으로 값 채운 후 (struct sockaddr *)&sockaddr 로 casting하면 됨.
 * 
 * 0x12345678을 저장할 때 
 * 네트워크는 빅 엔디안(0x12345678)을 쓴다. (리틀 엔디안: 0x78653412)
 * htons/l: host to network short(2바이트 데이터, 주로 port)/long(4바이트 데이터, 주로 ipv4)
 * ntohs/l: network to host short/long
 * 문자열은 char의 배열, 1바이트의 나열이므로 쪼개질 수 없다. 엔디안 변환은 숫자 덩어리에만 필요
 */
void oops(char *message);

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char message[BUF_SIZE];
    int str_len;

    // 인자 개수 확인 (실행파일 + IP + PORT)
    if(argc != 3){
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    // 1. 서버 소켓 생성
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(serv_sock == -1)
        oops("socket() error");

    // 2. 주소 구조체 초기화 (인자로 받은 IP와 Port 사용)
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // 입력받은 IP 할당
    serv_addr.sin_port = htons(atoi(argv[2]));      // 입력받은 Port 할당

    // 3. Bind 
    // sockaddr_in을 sockaddr * 구조체로 형변환(for polymorphism)
    if(bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1)
        oops("bind() error");

    // 4. Listen
    if(listen(serv_sock, BACKLOG) == -1)
        oops("listen() error");
    
    // printf("Server Start. Waiting for client...\n");

    // 5. Accept (클라이언트 연결 대기)
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(clnt_sock == -1)
        oops("accept() error");
    else
        // printf("Client Connected!\n");

    // 6. 데이터 송수신 루프 (Echo)
    while((str_len = read(clnt_sock, message, BUF_SIZE)) != 0)
    {
        message[str_len] = 0; // 문자열 끝 처리
        
        // 받은 정보 표준 출력
        fprintf(stdout, "%s", message); 
        // 만약 개행이 없다면 보기 좋게 추가
        if(message[str_len-1] != '\n') printf("\n");

        // 클라이언트에게 그대로 전송 (Echo)
        write(clnt_sock, message, str_len);
    }

    // fprintf(stdout, "Client Disconnected.\n");
    close(clnt_sock);
    close(serv_sock);
    return 0;
}

void oops(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
