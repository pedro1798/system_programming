#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <sys/signal.h>
#include <time.h>
#include <unistd.h>
/*
 * :::Todo:::
 * 매번 호출 시 마다 grid 화면에 그리기 
 * grid와 떨어지는 블록 충돌 감지 
 * 블럭 바닥에 닿으면 grid에 추가하고 다음 루프 이동 
 * 바닥 닿고 회전 -> ?? 루프 끝나기 전 순서 잘 조정하면 될듯? 
 * itimereal 로 매 프레임마다 틱 조절하기
 * dumb_idx: 실제 랜덤 값으로 수정하기 
 */

#define GRID_HEIGHT 24
#define GRID_WIDTH 22

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

void draw_tet(WINDOW* win, Tetrimino tet);
void erase_tet(WINDOW *win, Tetrimino tet); 
void update_grid(char* grid[], Tetrimino tet); 
void draw_grid(WINDOW *win, char* grid[], int box_height, int box_width);
Tetrimino rotate_tet(Tetrimino tet);
Tetrimino make_tet(int x, int y, Tetrimino tet);
Tetrimino *generate_tets();
Tetrimino move_tet(char flag, Tetrimino tet, int box_height, int box_width); 
int check_tet(char* grid[], Tetrimino tet);

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
    int box_height = GRID_HEIGHT;
    int box_width = GRID_WIDTH;
    
    /* 박스 시작 좌표 */
     int start_y = (term_height - box_height) / 2;
     int start_x = (term_width - box_width) / 2;

    /* 윈도우 생성 */
    WINDOW *win = newwin(box_height, box_width, start_y, start_x);
    
    /* 윈도우 테두리 그리기 */
    box(win, 0, 0);
    
    /* 테트리미노의 상대좌표 */
    Tetrimino *tets = generate_tets(); 
    
    /* 테트리스 그리드 */
    char **grid = malloc(box_height * sizeof(char*));
    for (int i = 0; i < box_height; i++) {
        grid[i] = malloc(box_width * sizeof(char));
    }
    for (int i = 0; i < box_height; i++) {
        for (int j = 0; j < box_width; j++) {
            // 1로 초기화    
            grid[i][j] = 1;
        }
    }
    
    int end_y, end_x;
    end_y = start_y + box_height;
    end_x = start_x + box_width;
    
    mvwprintw(stdscr, start_y, start_x, "1");
    mvwprintw(stdscr, box_height, box_width, "2");
    mvwprintw(stdscr, term_height-1, term_width-1, "3");
    mvwprintw(stdscr, end_y, end_x, "4");
    
    //refresh();  
    wrefresh(win); // 윈도우 갱신(화면에 실제로 그림)
    
    int new_x = box_width / 2;
    int new_y = 0;
    
    int dumb_idx = 0;
    int ch, speed;
    nodelay(stdscr, TRUE);
    
    Tetrimino old_tet;
    Tetrimino tet = make_tet(new_x, new_y, tets[dumb_idx]); // Let there be tetrimino 

    while(1) {
        erase_tet(win, tet); // 화면에서 이전 테트리미노 지운다. 
        
        old_tet = tet; // old_tet 업데이트

        // NOTE: if 체크 어떻게 할지 정하기  
        if (check_tet(grid, tet)) { // tet이 정지해야 한다면 old_tet으로 그리드 업데이트 후 loop 
            update_grid(grid, old_tet);
            
            /* 테트로미노 좌표 초기화 */
            new_x = box_width / 2; 
            new_y  = 1;

            dumb_idx = (dumb_idx + 1) % 7;

            tet = make_tet(new_x, new_y, tets[dumb_idx]); // 새 테트리미노 생성
            
            continue;
        }
        
        ch = getch();
        
        if (ch == 'q') break;
        if (ch == 'r') { // 테트로미노 회전 
            //erase_tet(win, tet); 
            tet = move_tet('r', tet, box_height, box_width);
        }
        if (ch == 'l') {
            tet = move_tet('l', tet, box_height, box_width);
        } 
        if (ch == 'h') {
            tet = move_tet('h', tet, box_height, box_width);
        }
        
        tet = move_tet('d', tet, box_height, box_width);

        draw_tet(win, tet);
        draw_grid(win, grid, box_height, box_width); 

        wrefresh(win);
        usleep(200000);
    }

    mvwprintw(stdscr, box_height + 1, 2, "Press any key to exit...");
    wrefresh(win);
    nodelay(stdscr, FALSE);
    getch();

    delwin(win);
    endwin();

    for (int i = 0; i < box_height; i++) {
        free(grid[i]);
    }
    free(grid);
    free(tets);

    return 0;
}

Tetrimino *generate_tets() {
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

Tetrimino make_tet(int x, int y, Tetrimino tet) {
    Tetrimino new_tet;
    new_tet.x1 = x + tet.x1; 
    new_tet.y1 = y + tet.y1;
    new_tet.x2 = x + tet.x2; 
    new_tet.y2 = y + tet.y2;
    new_tet.x3 = x + tet.x3; 
    new_tet.y3 = y + tet.y3;
    new_tet.x4 = x + tet.x4; 
    new_tet.y4 = y + tet.y4;
    return new_tet;
}

void draw_tet(WINDOW *win, Tetrimino tet) {
    mvwprintw(win, tet.y1, tet.x1 , "1");
    mvwprintw(win, tet.y2, tet.x2 , "2");
    mvwprintw(win, tet.y3, tet.x3 , "3");
    mvwprintw(win, tet.y4, tet.x4 , "4");
}

void draw_grid(WINDOW *win, char* grid[], int box_height, int box_width) {
    /* 1: 블럭 없음, 0은 블럭 있음. 1로 초기화 */
    for (int i = 0; i < box_height; i++) {
        for (int j = 0; j < box_width; j++) {
            if (!grid[i][j]) {
                mvwprintw(win, j, i , "#");
            }
        }
    }
}

void erase_tet(WINDOW *win, Tetrimino tet) {
    mvwprintw(win, tet.y1, tet.x1 , " ");
    mvwprintw(win, tet.y2, tet.x2 , " ");
    mvwprintw(win, tet.y3, tet.x3 , " ");
    mvwprintw(win, tet.y4, tet.x4 , " ");
}

Tetrimino rrotate_tet(Tetrimino tet) {
    Tetrimino tmp;
    
    tmp.x1 = -tet.y1; tmp.y1 = tet.x1; 
    tmp.x2 = -tet.y2; tmp.y2 = tet.x2; 
    tmp.x3 = -tet.y3; tmp.y3 = tet.x3; 
    tmp.x4 = -tet.y4; tmp.y4 = tet.x4; 
    
    return tmp;
}

Tetrimino move_tet(char flag, Tetrimino tet, int box_height, int box_width) {
    box_width--;
    switch(flag) {
        case('l'): // 오른쪽으로 이동
            if (tet.x1 < box_width && tet.x2 < box_width && tet.x3 < box_width && tet.x4 < box_width)
                tet.x1++; tet.x2++; tet.x3++; tet.x4++;
            break;
        case('h'):
            if (tet.x1 > 0 && tet.x2 > 0 && tet.x3 > 0 && tet.x4 > 0)
                tet.x1--; tet.x2--; tet.x3--; tet.x4--;
            break;
        case('d'):
            if (tet.y1 < box_height && tet.y2 < box_height && tet.y3 < box_height && tet.y4 < box_height)
                tet.y1++; tet.y2++; tet.y3++; tet.y4++;
            break;
        case('r'):
            Tetrimino tmp;
            tmp.x1 = -tet.y1; tmp.y1 = tet.x1; 
            tmp.x2 = -tet.y2; tmp.y2 = tet.x2; 
            tmp.x3 = -tet.y3; tmp.y3 = tet.x3; 
            tmp.x4 = -tet.y4; tmp.y4 = tet.x4; 
            
            if (tmp.x1 >= 0 && tmp.x1 < box_width && tmp.y1 >= 0 && tmp.y1 < box_height && 
                tmp.x2 >= 0 && tmp.x2 < box_width && tmp.y2 >= 0 && tmp.y2 < box_height && 
                tmp.x3 >= 0 && tmp.x3 < box_width && tmp.y3 >= 0 && tmp.y3 < box_height && 
                tmp.x4 >= 0 && tmp.x4 < box_width && tmp.y4 >= 0 && tmp.y4 < box_height)
                tet = tmp;
            break;
    }
    return tet;
}

int check_tet(char* grid[], Tetrimino tet) {
    /*
     * 1: 블럭 없음, 0은 블럭 있음. 1로 초기화
     */
    if(grid[tet.x1][tet.y1] && 
       grid[tet.x2][tet.y2] && 
       grid[tet.x3][tet.y3] && 
       grid[tet.x4][tet.y4]) {
        return 0; // 가능하면 0 리턴
    } 
    return 1; // 불가능하면 1 리턴 
}

void update_grid(char* grid[], Tetrimino tet) {
    grid[tet.x1][tet.y1] = 0;
    grid[tet.x2][tet.y2] = 0;
    grid[tet.x3][tet.y3] = 0;
    grid[tet.x4][tet.y4] = 0;
}
