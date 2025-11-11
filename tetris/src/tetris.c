#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <sys/signal.h>
#include <time.h>
#include <unistd.h>

/*
 * int LINES / COLS : 현재 터미널 행/열 크기
 * stdscr: 기본(전체) 윈도우, initscr() 호출 시 자동 생성 
 */

int main() {
    initscr(); // ncurses 모드 진입(표준 화면 초기화)
    cbreak(); // 버퍼링된 입력 즉시 전달, 즉 라인 버퍼링 해제(즉시 키 입력 받기)
    noecho(); // 입력 문자 화면 표시하지 않음
    keypad(stdscr, TRUE); // 화살표 등 특수키 활성화
    curs_set(0); // 커서 숨김
    
    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width); // 터미널 크기 얻기
    
    int box_height = term_height * 2 / 3;
    int box_width = term_width * 2 / 3;
    
    if (box_height > term_height - 2) box_height = term_height - 2;
    if (box_width  > term_width  - 2) box_width  = term_width  - 2;


    int start_y = (term_height - box_height) / 2;
    int start_x = (term_width - box_width) / 2;

    WINDOW *win = newwin(box_height, box_width, start_y, start_x);
    box(win, 0, 0);
    
    char msg[] = "Hello, ncurses world!";
    int msg_y = box_height / 2;
    int msg_x = (box_width - (int)strlen(msg)) / 2;
    
    if (msg_y == box_height - 1) msg_y--;  // 경계 침범 방지

    mvwprintw(win, msg_y, msg_x, "%s", msg);

    wrefresh(win); // 윈도우 갱신(화면에 실제로 그림)
    // refresh();
    /* 종료 대기 */
    // mvprintw(term_height - 2, 2, "Press any key to exit...");
    getch();

    delwin(win);
    endwin();
    
    return 0;
}

