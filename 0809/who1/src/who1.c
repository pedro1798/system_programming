/* who1.c 
 * who1.c는 리눅스의 UTMP 파일 (/var/run/utmp)을 읽어서 
 * 현재 로그인한 사용자 정보를 출력하는 프로그램이다.
 */

#include <stdio.h> /* printf, perror */
#include <stdlib.h> /* exit 등 */

/* UTMP_FILE 매크로와 utmp 정의 등
 * glibc(GNU C Library)에 포함된 표준 헤더
 */
#include <utmp.h> 

#include <fcntl.h> /* open() 등 file control */
#include <unistd.h> /* read, close 함수 등 */

#define SHOWHOST

void show_info(struct utmp *utbufp); 

int main() {
    struct utmp current_record; 
    /* 로그인 세션(사용자, tty, 시간, host 등 저장하는 구조체  
     * UTMP의 한 줄(record)를 담는 변수
     */ 
    int utmpfd; /* UTMP File Discriptor */
    int reclen = sizeof(current_record); /* record length */

    /* /var/run/utmp 파일을 RDONLY로 연다 */
    if ((utmpfd = open(UTMP_FILE, O_RDONLY)) == -1) {
        perror(UTMP_FILE); /* 에러 처리 */
        exit(1);
    }

    while (read(utmpfd, &current_record, reclen) == reclen) {
        /* 읽은 결과가 정확히 레코드 크기와 같으면 반복 
         * 읽어 온 레코드를 show_info 함수의 매개변수로 넘김.
         */
        show_info(&current_record);
    }
    close(utmpfd); /* 파일 디스크립터 close */
    return 0;
}

void show_info(struct utmp *utbufp) { /* utmp에서 읽어 온 정보 출력 */
    if ( utbufp->ut_type != USER_PROCESS ) { /* users only */
        return;
    }
    printf("%-8.8s", utbufp->ut_name); /* the username */
    printf(" ");
    printf("%-8.8s", utbufp->ut_line); /* 터미널명 */
    printf(" ");
    printf("%-8.8d", utbufp->ut_time); /* 로그인 시각 */
    printf(" ");

#ifdef SHOWHOST /* if SHOWHOST is defined, compile this code */
    /* 사용자가 원격 접속했을 때 호스트명 또는 IP */
    printf("(%s)", utbufp->ut_host); 
#endif
    printf("\n");
}
