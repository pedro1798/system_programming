#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "../include/check.h"
#include "../include/draw.h"
#include "../include/initiate.h"
#include "../include/movement.h"
#include "../include/tetrimino.h"
#include "../include/ticker.h"
#include "../include/update.h"

#define GRID_HEIGHT 24
#define GRID_WIDTH 12
#define TICK 300000
#define LEVEL_SLOPE 3

void show_interface(int row, int col, int line, int score, Tetrimino offset); 

/*
 * draw 함수에 is_collide 감싼거 지우기. 블럭 충돌만 감지함? 
 * 다음 블록, 지운 라인 수, 점수  그리기
 * 고스트 블록 혹은 고스트 라인
 * 현재 점수(Score), 지운 라인 수(Lines Cleared)
 * 프레임 시그널 처리로 개선하기
 */

int main() {
    int tick = TICK;
    set_ticker(tick);

    initscr(); 
    refresh();
    cbreak(); // 버퍼링된 입력 즉시 전달, 라인 버퍼링 해제(즉시 키 입력 받기)
    noecho(); // 입력 문자 화면 표시하지 않음
    keypad(stdscr, TRUE); // 화살표 등 특수키 활성화
    curs_set(0); // 커서 숨김
    
    int term_height, term_width; // 터미널 크기 받는 변수
    getmaxyx(stdscr, term_height, term_width); // 터미널 크기 저장
    
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
    
    /* 상단 메시지 */ 
    char* hello_str = "Tetris";
    mvwprintw(stdscr, start_y - 1, 
            ((term_width - strlen(hello_str)) / 2), 
            "%s\n",
            hello_str);
    
    /* 테트리스 그리드 초기화 */
    char **grid = generate_grid(box_height, box_width); 
    
    int dumb_idx = 0;
    int next_dumb_idx = (dumb_idx + 1) % 7;
    // int speed = TICK;
    int lines_cleared = 0; 
    int level = 0;
    int score = 0;
    int tmp_ch, ch;
    
    int interface_x, interface_y;
    interface_x = start_x - 14;
    interface_y = start_y;
    show_interface(interface_y, 
                   interface_x, 
                   lines_cleared, 
                   score, 
                   tets[next_dumb_idx]); 
    
    nodelay(stdscr, TRUE);
    
    Tetrimino new_tet = make_tet((box_width / 2), 1, tets[dumb_idx]); 
    Tetrimino old_tet = new_tet;
    

    while(1) { /* while - 프레임 갱신 */ 
        erase_tet(win, box_height, box_width, old_tet); 
        
        if (is_collide(grid, new_tet)) {
            int bonus = lines_cleared;
            lines_cleared += update_grid(grid, old_tet, box_height, box_width);
            level = lines_cleared / LEVEL_SLOPE; // 0, 1, 2, ...
            score += 1 * level;
            if (lines_cleared - bonus) score += (lines_cleared - bonus) * 20;
            if (tick > 150000) tick -= level * 500;

            set_ticker(tick);
            
            dumb_idx = (dumb_idx + 1) % 7; 
            next_dumb_idx = (dumb_idx + 1) % 7;
            show_interface(interface_y, interface_x, lines_cleared, score, tets[next_dumb_idx]); 
            new_tet = make_tet((box_width / 2), 1, tets[dumb_idx]); 
            
            draw_grid(win, grid, box_height, box_width); 
            
            continue;
        }
        old_tet = new_tet;
        
        tmp_ch = getch();

        if (tmp_ch != -1) ch = tmp_ch;

        if(get_tick()) {
            Tetrimino tmp;
            /* new_tet 업데이트 */ 
            if (ch == 'q') break;
            else if (ch == 'r' || ch == '\n') { // 테트로미노 회전 
                tmp = rotate_tet(old_tet, box_height, box_width);
                if (is_inside(box_height, box_width, tmp)) new_tet = tmp;
            } else if (ch == 'l' || ch == KEY_RIGHT) {
                tmp = move_tet('l', old_tet, box_height, box_width);
                if (is_inside(box_height, box_width, tmp)) new_tet = tmp;
            } else if (ch == 'h' || ch == KEY_LEFT) {
                tmp = move_tet('h', old_tet, box_height, box_width);
                if (is_inside(box_height, box_width, tmp)) new_tet = tmp;
            } 
            else if (ch == 'j' || ch == KEY_DOWN) { 
                set_ticker(TICK / 3);
            } 
            else if (ch == 'R') {
                /* 처음부터 다시 */
                continue;
                break;
            } 
            new_tet = move_tet('d', new_tet, box_height, box_width);
            
            ch = -1;
            set_tick();
        }

        draw_grid(win, grid, box_height, box_width); 
        draw_ghost_line(win, grid, old_tet, box_height);
        draw_tet(win, box_height, box_width, old_tet);
        
        wrefresh(win);
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

void show_interface(int row, int col, int line, int score, Tetrimino offset) {
    /* lines cleared */
    int level = line / LEVEL_SLOPE;
    mvwprintw(stdscr, row + 0, col, ".::Scores::.");
    mvwprintw(stdscr, row + 1, col, "   LINE: %d", line);
    mvwprintw(stdscr, row + 2, col, "   SCORE: %d", score);
    mvwprintw(stdscr, row + 3, col, "   LEVEL: %d", level);
    
    int block_y = row + 6;
    int block_x = col + 4; 

    mvwprintw(stdscr, row + 5, col + 1, ":::NEXT:::");
    /* erase previous output */ 
    mvwprintw(stdscr, (block_y + 0), (block_x - 2), "      ");
    mvwprintw(stdscr, (block_y + 1), (block_x - 2), "      ");
    mvwprintw(stdscr, (block_y + 2), (block_x - 2), "      ");
    mvwprintw(stdscr, (block_y + 3), (block_x - 2), "      ");
    mvwprintw(stdscr, (block_y + 4), (block_x - 2), "      ");
    
    mvwprintw(stdscr, (block_y + offset.y1), (block_x + offset.x1), "#");
    mvwprintw(stdscr, (block_y + offset.y2), (block_x + offset.x2), "#");
    mvwprintw(stdscr, (block_y + offset.y3), (block_x + offset.x3), "#");
    mvwprintw(stdscr, (block_y + offset.y4), (block_x + offset.x4), "#");
}
