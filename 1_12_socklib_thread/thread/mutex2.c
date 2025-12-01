#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h> // isalnum을 사용하기 위해 필요함

struct arg_set {
    char *fname;   /* two values in one arg */
    int count;     /* file to examine */
                   /* number of words */
};

struct arg_set args1;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t flag = PTHREAD_COND_INITIALIZER;
void *mailbox; // [추정] 데이터를 전달하는 전역 포인터 (이미지에선 누락됨)
int total_words = 0;
struct arg_set args2; // [추정] 두 번째 스레드용 구조체

// 메인 함수에 보이는 전역 변수 선언은 이중 선언일 수 있습니다.
int reports_in = 0;
int main(int ac, char *av[])
{
    pthread_t t1, t2;
    void  *count_words(void *);

    if ( ac != 3 )
        exit(1);

    // 보고서 락 (락은 메인 함수 마지막에 해제되어야 함)
    pthread_mutex_lock(&lock);

    // args1 설정 및 스레드 1 생성
    args1.fname = av[1];
    args1.count = 0;
    pthread_create(&t1, NULL, count_words, (void *)&args1);

    // args2 설정 및 스레드 2 생성
    args2.fname = av[2];
    args2.count = 0;
    pthread_create(&t2, NULL, count_words, (void *)&args2);

    while ( reports_in < 2 ) {
        /* print("MAIN: waiting for flag to go up\n"); */

        // 조건 변수 대기: 락을 풀고 대기하다가 시그널을 받으면 락을 얻음
        pthread_cond_wait(&flag, &lock);

        /* print("MAIN: Wow! flag was raised, I have the lock\n"); */
        // 전역 변수 mailbox를 통해 스레드로부터 받은 결과 출력
        printf("%7d: %s\n", mailbox->count, mailbox->fname);
        total_words += mailbox->count;

        // [이미지에서 잘린 부분] 해당 스레드를 join하고 mailbox를 초기화하는 로직 추정
        if ( mailbox == &args1 )
            pthread_join(t1, NULL);
        if ( mailbox == &args2 )
            pthread_join(t2, NULL);

        mailbox = NULL;

        // [이미지에서 잘린 부분]
        // pthread_cond_signal(&flag); // 이 위치에서의 signal은 이상하므로 원본을 따르지 않음
        reports_in++;
    }

    // [이미지에서 잘린 부분] 최종 정리
    pthread_mutex_unlock(&lock);
    printf("Total words: %d\n", total_words);

    return 0; // 메인 함수 닫는 괄호
}
