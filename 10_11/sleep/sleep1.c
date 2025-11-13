/*
 * sleep1.c
 * purpose show how sleep works
 * usage sleep1
 * outline sets handler, sets alarm, pauses, then returns
 */

#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int main() {
    void wakeup(int);
    printf("about to sleep for 4 seconds\n");
    signal(SIGALRM, wakeup);
    alarm(4);
    int p = pause(); // 시그널이 올 때 까지 무기한 잠들기
    printf("Morning so soon?, pause() returns %d when gets signal\n", p);
}

void wakeup(int signum) {
    printf("Alarm received from kernel\n");
}
