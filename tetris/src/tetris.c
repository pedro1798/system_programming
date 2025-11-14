#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
/*
 * NOTE::
 * 횡적 이동 블럭 있으면 move 못 하게 막기
 * grid 행 다 차면 해당 행 지우고(반짝반짝) 남은 블럭 모두 한 칸 아래로 옮기기
 * 점수 표시하기
 * sleep() 시간 업데이트 해서(프레임 높여서) 떨어지는 속도 조절하기
 * itimereal 로 매 프레임마다 틱 조절하기 - 시그널 사용 
 * dumb_idx: 실제 랜덤 값으로 수정하기 
 * main화면 그리고 Press any key to start 하기
 * 게임 끝나면 다시 fork execvp 해서 새로 시작(?) 
 * 프로세스 새로 만들어서 점수 저장(?)
 */

#define GRID_HEIGHT 24
#define GRID_WIDTH 22

/* 블럭 상대 좌표 */
typedef struct {
    int x1; 
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
void update_grid(char* grid[], Tetrimino tet, int box_height, int box_width); 
void draw_grid(WINDOW *win, char* grid[], int box_height, int box_width);
Tetrimino rotate_tet(Tetrimino tet, int box_height, int box_width);
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
    
    char* hello_str = "Tetris";
    mvwprintw(stdscr, start_y - 1, 
            ((term_width - strlen(hello_str)) / 2), 
            "%s\n",
            hello_str);
    
    /* 테트리스 그리드 초기화 */
    char **grid = malloc(box_height * sizeof(char*));
    for (int i = 0; i < box_height; i++) {
        grid[i] = malloc(box_width * sizeof(char));
    }
    for (int i = 0; i < box_height; i++) {
        for (int j = 0; j < box_width; j++) {
            // 1: 블럭 없음, 0: 블럭 있음;    
            grid[i][j] = 1;
        }
    }
    for (int j = 0; j < box_width; j++) {
        grid[box_height-1][j] = -1;
    }
    /* 
    int end_y, end_x;
    end_y = start_y + box_height;
    end_x = start_x + box_width;
    
    mvwprintw(stdscr, start_y, start_x, "1");
    mvwprintw(stdscr, box_height, box_width, "2");
    mvwprintw(stdscr, term_height-1, term_width-1, "3");
    mvwprintw(stdscr, end_y, end_x, "4");
    
    refresh();  
    wrefresh(win); // 윈도우 갱신(화면에 실제로 그림)
    */
    
    int new_x = box_width / 2;
    int new_y = 1;
    
    int dumb_idx = 0;
    int ch, speed;
    nodelay(stdscr, TRUE);

    // Let there be tetrimino
    Tetrimino new_tet = make_tet(new_x, new_y, tets[dumb_idx]); 
    Tetrimino old_tet = new_tet;

    while(1) {
        erase_tet(win, old_tet); // 화면에서 이전 테트리미노 지운다. 
        // tet이 정지해야 한다면 old_tet으로 그리드 업데이트 후 loop 
        if (check_tet(grid, new_tet)) { 
            update_grid(grid, old_tet, box_height, box_width); // old_tet을 그리드에 업데이트
            
            /* 테트로미노 좌표 초기화 */
            new_x = box_width / 2; 
            new_y = 1;

            dumb_idx = (dumb_idx + 1) % 7; // 다음 테트리미노 

            new_tet = make_tet(new_x, new_y, tets[dumb_idx]); // tet update
            draw_grid(win, grid, box_height, box_width); 
            
            continue;
        }
        old_tet = new_tet;

        /* new_tet 업데이트 */ 
        ch = getch();
        if (ch == 'q') break;
        else if (ch == 'r') { // 테트로미노 회전 
            new_tet = rotate_tet(old_tet, box_height, box_width);
        } else if (ch == 'l') {
            new_tet = move_tet('l', old_tet, box_height, box_width);
        } else if (ch == 'h') {
            new_tet = move_tet('h', old_tet, box_height, box_width);
        } else {
            new_tet = move_tet('d', old_tet, box_height, box_width);
        }

        draw_grid(win, grid, box_height, box_width); 
        draw_tet(win, old_tet);
        
        wrefresh(win);
        usleep(200000);
    }
    /* while(1) 루프 끝나면 */
    char* exit_str = "Press any key to exit...";
    int bottom = ((term_width - strlen(exit_str)) / 2);
    mvwprintw(stdscr, box_height + 10, bottom, "%s", exit_str);
    mvwprintw(stdscr, box_height + 12, bottom, "box_height: %d", box_height);
    mvwprintw(stdscr, box_height + 14, bottom, "box_width: %d", box_width);

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
    L.x1 = 0; L.x2 = 0; L.x3 = 0; L.x4 = 1;
    L.y1 = 0; L.y2 = 1; L.y3 = 2; L.y4 = 2;
    
    /* 2 */
    J.x1 = 0; J.x2 = 0; J.x3 = 0; J.x4 = -1;
    J.y1 = 0; J.y2 = 1; J.y3 = 2; J.y4 = 2;
    
    /* 3 */
    O.x1 = 0; O.x2 = 1; O.x3 = 1; O.x4 = 0;
    O.y1 = 0; O.y2 = 0; O.y3 = 1; O.y4 = 1;
    
    /* 4 */
    S.x1 = 1; S.x2 = 2; S.x3 = 0; S.x4 = 1;
    S.y1 = 0; S.y2 = 0; S.y3 = 1; S.y4 = 1;
    
    /* 5 */
    T.x1 = 0; T.x2 = 0; T.x3 = -1; T.x4 = 1;
    T.y1 = 0; T.y2 = 1; T.y3 = 1; T.y4 = 1;
    
    /* 6 */
    Z.x1 = 0; Z.x2 = 1; Z.x3 = 1; Z.x4 = 1;
    Z.y1 = 0; Z.y2 = 0; Z.y3 = 1; Z.y4 = 2;
   
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
    mvwprintw(win, tet.y1, tet.x1 , "#");
    mvwprintw(win, tet.y2, tet.x2 , "#");
    mvwprintw(win, tet.y3, tet.x3 , "#");
    mvwprintw(win, tet.y4, tet.x4 , "#");
}

void draw_grid(WINDOW *win, char* grid[], int box_height, int box_width) {
    /* 
     * 1: 블럭 없음, 0은 블럭 있음. 1로 초기화 
     * 그리드 마지막 줄은 -1로 초기화 되어있음 
     */
    int i, j;
    for (i = 0; i < box_height; i++) {
        for (j = 0; j < box_width; j++) {
            if (grid[i][j] == 0) {
                mvwprintw(win, i, j, "#");
            }
            /* 빈 공간 '.' 초기화 */
            if (i > 0 && i < (box_height - 1) &&
                j > 0 && j < (box_width - 1) &&
                grid[i][j] == 1) {
                mvwprintw(win, i, j, ".");
            }
        }
    }
}

void update_grid(char* grid[], Tetrimino tet, int box_height, int box_width) {
    /*
     * Tetrimino가 정지해야 할 때 실행됨.
     * 가능한 Tetrimino만 넘겨받는다.
     */
    grid[tet.y1][tet.x1] = 0;
    grid[tet.y2][tet.x2] = 0;
    grid[tet.y3][tet.x3] = 0;
    grid[tet.y4][tet.x4] = 0;

    int saturate = 0;
    int sat_idx = -1;

    for (int i = 0; i < box_height; i++) {
        for (int j = 0; j < box_width; j++) {
            if (grid[i][j] == 0) {
                saturate++;
                if (saturate == box_width - 2) {
                    sat_idx = i;
                }
            } 
        }
        saturate = 0;
    }
    /* 
     * i번째 줄이 지워짐 -> [0,i-1] 줄이 아래로 한 칸 내려와야됨 
     * i-1 부터 0번줄까지 올라가며 아래로 한 칸 떨어뜨리기
     */
    if (sat_idx != -1) { 
        for (int k = 0; k < box_width; k++) {
            grid[sat_idx][k] = 1;
        }
        for (int i = sat_idx-1; i > 0; i--) {
            for (int j = 0; j < box_width; j++) {
                if (grid[i][j] == 0) {
                    grid[i][j] = 1; // 밑으로 한 칸 내리기
                    grid[i+1][j] = 0; 
                }
            }
        }
    }
    sat_idx = -1;
}

void erase_tet(WINDOW *win, Tetrimino tet) {
    mvwprintw(win, tet.y1, tet.x1 , " ");
    mvwprintw(win, tet.y2, tet.x2 , " ");
    mvwprintw(win, tet.y3, tet.x3 , " ");
    mvwprintw(win, tet.y4, tet.x4 , " ");
}

Tetrimino rotate_tet(Tetrimino tet, int box_height, int box_width) {
    Tetrimino tmp = tet;
    Tetrimino rot = tet;

    rot.x1 -= tet.x2; 
    rot.x2 -= tet.x2;
    rot.x3 -= tet.x2;
    rot.x4 -= tet.x2;
    rot.y1 -= tet.y2;
    rot.y2 -= tet.y2;
    rot.y3 -= tet.y2;
    rot.y4 -= tet.y2;
    
    tmp.x1 = -rot.y1;
    tmp.x2 = -rot.y2;
    tmp.x3 = -rot.y3;
    tmp.x4 = -rot.y4;
    tmp.y1 = rot.x1;
    tmp.y2 = rot.x2;
    tmp.y3 = rot.x3;
    tmp.y4 = rot.x4;

    tmp.x1 += tet.x2;
    tmp.x2 += tet.x2;
    tmp.x3 += tet.x2;
    tmp.x4 += tet.x2;
    tmp.y1 += tet.y2;
    tmp.y2 += tet.y2;
    tmp.y3 += tet.y2;
    tmp.y4 += tet.y2;
    
    if (tmp.x1 >= 0 && tmp.x1 < box_width && 
        tmp.y1 >= 0 && tmp.y1 < box_height && 
        tmp.x2 >= 0 && tmp.x2 < box_width && 
        tmp.y2 >= 0 && tmp.y2 < box_height && 
        tmp.x3 >= 0 && tmp.x3 < box_width && 
        tmp.y3 >= 0 && tmp.y3 < box_height && 
        tmp.x4 >= 0 && tmp.x4 < box_width && 
        tmp.y4 >= 0 && tmp.y4 < box_height) {
        return tmp;
    }
    return tet;
}

Tetrimino move_tet(char flag, Tetrimino tet, int box_height, int box_width) {
    box_height -= 1;
    box_width -= 2;
    switch(flag) {
        case('l'): // 오른쪽으로 이동
            if (tet.x1 < box_width && 
                tet.x2 < box_width &&
                tet.x3 < box_width && 
                tet.x4 < box_width) {
                tet.x1++; tet.x2++; tet.x3++; tet.x4++;
            }
            break;
        case('h'):
            if (tet.x1 > 1 && 
                tet.x2 > 1 &&
                tet.x3 > 1 && 
                tet.x4 > 1) {
                tet.x1--; tet.x2--; tet.x3--; tet.x4--;
            }
            break;
        case('d'): {
            if (tet.y1 < box_height && 
                tet.y2 < box_height && 
                tet.y3 < box_height && 
                tet.y4 < box_height) {
                tet.y1++; tet.y2++; tet.y3++; tet.y4++;
            }
            break;
        }
    }
    return tet;
}

int check_tet(char* grid[], Tetrimino tet) {
    /*
     * 1: 블럭 없음, 0은 블럭 있음. 1로 초기화
     */
    mvwprintw(stdscr, 2, 2, "x1: %d, y1: %d\n",tet.x1, tet.y1);
    refresh();
    if(grid[tet.y1][tet.x1] == 1 && 
       grid[tet.y2][tet.x2] == 1 && 
       grid[tet.y3][tet.x3] == 1 && 
       grid[tet.y4][tet.x4] == 1) {
        return 0; // 가능하면 0 리턴
    } 
    return 1; // 불가능하면 1 리턴 
}
