/* utmplib.c는 버퍼를 통한 데이터 플로우를 다루기 위한 
 * 버퍼와 변수와 함수들을 포함한다.
 *
 * utmp 파일에서 읽어오는 버퍼에 관한 함수들
 *  - utmp_open(filename) - open file, returns -1 on error
 *  - utmp_next() - return pointer to next struct, returns NULL on eof
 *  - utmp_close() - close file
 *
 *  reads NRECS per read and then doles them out from the buffer.
 *  NRECS: 레코드의 개수
 *
 * 변수 num_recs와 cur_rec은 버퍼에 얼마나 많은 구조체가 있는지와
 * 얼마나 많은 구조체가 사용되었는지 기록한다.
 */

#include "utmplib.h"
/* 소스코드 파일에도 헤더 선언해야함. 첫 번째 줄에 선언하기. */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>
#include <fcntl.h>
/* sys/types.h: 자료형을 정의하는 헤더. pid_t, uid_t, gid_t 등등 */
#include <sys/types.h>

#define NRECS 16 /* 레코드의 개수 16개 */
#define NULLUT ((struct utmp *) NULL) /* utmp에 대한 NULL 정의 */
/* NULL을 struct utmp * 타입으로 변환. 원래는 struct utmp를 가리키지만 
 * 현재는 null을 가리킨다는 뜻 */
#define UTSIZE (sizeof(struct utmp))

static char utmpbuf[NRECS * UTSIZE]; /* storage */
static int num_recs; /* num stored */
static int cur_rec; /* next to go */
static int fd_utmp = -1; 
/* fd는 항상 0 이상이므로 -1이면 문제 있다 
 * read from */

int utmp_open(char * filename) {
    fd_utmp = open(filename, O_RDONLY);
    cur_rec = num_recs = 0;
    return fd_utmp;
}

struct utmp * utmp_next() {
    struct utmp * recp;
    if (fd_utmp == -1) {
        return NULLUT;
    }
    if (cur_rec == num_recs && utmp_reload() == 0) {
        return NULLUT;
    }

    recp = (struct utmp *) &utmpbuf[cur_rec * UTSIZE];
    cur_rec++;
    return recp;
}

int utmp_reload() {
/*
 * read next bunch of records into buffer
 */
    int amt_read;
    amt_read = read(fd_utmp, utmpbuf, NRECS * UTSIZE );
    num_recs = amt_read / UTSIZE;
    cur_rec = 0;
    
    return num_recs;
}

void utmp_close() {
    if (fd_utmp != -1) {
        close(fd_utmp);
    }
}




