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
    refresh(); // 한 번 초기화 
    cbreak(); // 버퍼링된 입력 즉시 전달, 즉 라인 버퍼링 해제(즉시 키 입력 받기)
    noecho(); // 입력 문자 화면 표시하지 않음
    keypad(stdscr, TRUE); // 화살표 등 특수키 활성화
    curs_set(0); // 커서 숨김
    
    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width); // 터미널 크기 얻기
    
    int box_height = term_height * 8 / 10;
    int box_width = term_width * 6 / 10;
    
    int start_y = (term_height - box_height) / 2;
    int start_x = (term_width - box_width) / 2;

    WINDOW *win = newwin(box_height, box_width, start_y, start_x);
    box(win, 0, 0);
    
    char msg[] = "Tetris";
    int msg_y = box_height / 2;
    int msg_x = (box_width - (int)strlen(msg)) / 2;
    
    if (msg_y == box_height - 1) msg_y--;  // 경계 침범 방지

    mvwprintw(win, msg_y, msg_x, "%s", msg);

    wrefresh(win); // 윈도우 갱신(화면에 실제로 그림)
    // getch();
    
    int old_y = start_y + 1;
    int old_x = box_width / 2;

    while(1) {
        if (old_y == (start_y + box_height - 1)) {
            mvwprintw(win, old_y, old_x, "break!!!!!!!!!!");
            break;
        }
        int new_y = old_y + 1;
        int new_x = old_x;
        
        mvwprintw(win, old_y, old_x, " ");
        mvwprintw(win, new_y, new_x, "block");

        sleep(1);
        wrefresh(win);
    }

    delwin(win);
    endwin();
    
    return 0;
}
