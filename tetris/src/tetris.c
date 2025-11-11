#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <sys/signal.h>
#include <time.h>
#include <unistd.h>

/* tets free하기! */

/*
 * int LINES / COLS : 현재 터미널 행/열 크기
 * stdscr: 기본(전체) 윈도우, initscr() 호출 시 자동 생성 
 */

/* 블럭 상대 좌표 */
typedef struct {
    int x1; // x1, y1: 가장 왼쪽 위 pivot 
    int y1;
    int x2;
    int y2;
    int x3;
    int y3;
    int x4;
    int y4;
} Tetrimino; 

void draw(WINDOW* win, int x, int y, Tetrimino tet);

Tetrimino rotate(Tetrimino tet);
Tetrimino *gen_tetrimino();

int main() {
    initscr(); // ncurses 모드 진입(표준 화면 초기화)
    refresh(); // 한 번 초기화 
    cbreak(); // 버퍼링된 입력 즉시 전달, 즉 라인 버퍼링 해제(즉시 키 입력 받기)
    noecho(); // 입력 문자 화면 표시하지 않음
    keypad(stdscr, TRUE); // 화살표 등 특수키 활성화
    curs_set(0); // 커서 숨김
    
    int term_height, term_width; // 터미널 크기 받는 변수
    getmaxyx(stdscr, term_height, term_width); // 터미널 크기 얻기
    
    /* 박스 크기 변수 */
    int box_height = term_height * 8 / 10;
    int box_width = term_width * 6 / 10;
    
    /* 박스 시작 좌표 */
    int start_y = (term_height - box_height) / 2;
    int start_x = (term_width - box_width) / 2;

    /* 윈도우 생성 */
    WINDOW *win = newwin(box_height, box_width, start_y, start_x);
    /* 윈도우 테두리 그리기 */
    box(win, 0, 0);
    
    char msg[] = "Tetris";
    int msg_y = box_height / 2;
    int msg_x = (box_width - (int)strlen(msg)) / 2;
    
    if (msg_y == box_height - 1) msg_y--;  // 경계 침범 방지
    
    /* 테트리미노의 상대좌표 */
    Tetrimino *tets = gen_tetrimino(); 
    
    
    /* 테트리스 그리드 */
    char **grid = malloc(box_height * sizeof(char*));
    for (int i = 0; i < box_height; i++) {
        grid[i] = malloc(box_width * sizeof(char));
    }

    mvwprintw(win, msg_y, msg_x, "%s", msg);

    wrefresh(win); // 윈도우 갱신(화면에 실제로 그림)
    // getch();
    
    char* txt = "block"; 
    
    int old_y = 2;
    int old_x = box_width / 2;
    
    int dumb_idx = 0;

    while(1) {
        if (old_y >= (box_height - 1)) {
            old_y = 2;
            dumb_idx = (dumb_idx + 1) % 7;
            continue;
        }
        
        draw(win, old_x, old_y++, tets[dumb_idx]);
                
        wrefresh(win);
        usleep(200000);
    }

    mvwprintw(win, box_height - 1, 2, "Press any key to exit...");
    wrefresh(win);
    getch();

    delwin(win);
    endwin();

    for (int i = 0; i < box_height; i++) {
        free(grid[i]);
    }
    free(grid);
    
    return 0;
}

Tetrimino *gen_tetrimino() {
    Tetrimino *arr = malloc(sizeof(Tetrimino) * 7);

    Tetrimino I, L, J, O, S, T, Z;
    /* 0 */
    I.x1 = 0; I.x2 = 0; I.x3 = 0; I.x4 = 0;
    I.y1 = 0; I.y2 = 1; I.y3 = 2; I.y4 = 3;
    
    /* 1 */
    L.x1 = 0; L.x2 = 1; L.x3 = 2; L.x4 = 2;
    L.y1 = 0; L.y2 = 0; L.y2 = 0; L.y3 = 1;
    
    /* 2 */
    J.x1 = 0; J.x2 = 0; J.x3 = -1; J.x4 = -2;
    J.y1 = 0; J.y2 =1; J.y3 = 1; J.y4 = 1;
    
    /* 3 */
    O.x1 = 0; O.x2 = 1; O.x3 = 1; O.x4 = 0;
    O.y1 = 0; O.y2 = 0; O.y3 =1; O.y4 = 1;
    
    /* 4 */
    S.x1 = 0; S.x2 = 0; S.x3 = -1; S.x4 = -1;
    S.y1 = 0; S.y2 = 1; S.y3 = 1; S.y4 = 2;
    
    /* 5 */
    T.x1 = 0; T.x2 = 0; T.x3 = -1; T.x4 = 0;
    T.y1 = 0; T.y2 = 1; T.y3 = 1; T.y4 = 2;
    
    /* 6 */
    Z.x1 = 0; Z.x2 = 0; Z.x3 = 1; Z.x4 = 1;
    Z.y1 = 0;Z.y1 = 0;  Z.y2 = 1; Z.y3 = 1; Z.y4 = 2;
   
    // Tetrimino tets[] = {I, L, J, O, S, T, Z};
    
    arr[0] = I;
    arr[1] = L;
    arr[2] = J;
    arr[3] = O;
    arr[4] = S;
    arr[5] = T;
    arr[6] = Z;

    return arr; 
}

void draw(WINDOW *win, int x, int y, Tetrimino tet) {
    int x1 = x + tet.x1; 
    int y1 = y + tet.y1;
    int x2 = x + tet.x2; 
    int y2 = y + tet.y2;
    int x3 = x + tet.x3; 
    int y3 = y + tet.y3;
    int x4 = x + tet.x4; 
    int y4 = y + tet.y4;
    
    mvwprintw(win, y1, x1 , " ");
    mvwprintw(win, y2, x2 , " ");
    mvwprintw(win, y3, x3 , " ");
    mvwprintw(win, y4, x4 , " ");
    
    mvwprintw(win, y1+1, x1 , "1");
    mvwprintw(win, y2+1, x2 , "2");
    mvwprintw(win, y3+1, x3 , "3");
    mvwprintw(win, y4+1, x4 , "4");
}

Tetrimino rotate(Tetrimino tet) {
    Tetrimino tmp;
    
    tmp.x1 = -tet.y1; tmp.y1 = tet.x1; 
    tmp.x2 = -tet.y2; tmp.y2 = tet.x2; 
    tmp.x3 = -tet.y3; tmp.y3 = tet.x3; 
    tmp.x4 = -tet.y4; tmp.y4 = tet.x4; 
    
    return tmp;
}
