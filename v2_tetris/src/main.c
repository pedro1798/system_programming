#include <stdlib.h>
#include <locale.h>
#include <fcntl.h>
#include <string.h>
//#include <ncurses.h>
#include <ncursesw/curses.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "../include/check.h"
#include "../include/draw.h"
#include "../include/data.h"
#include "../include/initiate.h"
#include "../include/movement.h"
#include "../include/tetrimino.h"
#include "../include/ticker.h"
#include "../include/update.h"

#define GRID_HEIGHT 24
#define LEVEL_SLOPE 10
#define GRID_WIDTH 12
#define TICK 300000
#define MAX_SCORE 32

int line_score(int line_cnt, int level);

/* NOTE::
 * 블럭 회전 시스템 표준에 맞게 개선하기
 * 상태 변수(박스 크기 등) 구조체에 담아서 전달
 */

int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "");
    (void)argc;
    int dumb_idx = 0;
    int next_dumb_idx = (dumb_idx + 1) % 7;
    // int speed = TICK;
    int line_cleared = 0; 
    int level = 0;
    int score = 0;
    // NOTE:: 0으로 초기화하지 말고 이전 값 미리 불러오기 
    // int prev_score = 0;
    int tmp_ch, ch;

    game_status_t status;
    prev_score_t data = load_data();
   
    /*
    // 파일에 기록 
    FILE *fp = fopen(".score.txt", "r+");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    char score_buf[MAX_SCORE]; 
    fgets(score_buf, MAX_SCORE, fp);
    prev_score = atoi(score_buf);
    status.prev_score = prev_score;
    status.score = 0;
    status.line_cleared = 0;
    fclose(fp);
    */

    int tick = TICK;
    set_ticker(tick);

    initscr(); 
    refresh();
    cbreak(); // 버퍼링된 입력 즉시 전달, 라인 버퍼링 해제(즉시 키 입력 받기)
    noecho(); // 입력 문자 화면 표시하지 않음
    keypad(stdscr, TRUE); // 화살표 등 특수키 활성화
    nodelay(stdscr, TRUE);
    curs_set(0); // 커서 숨김
    
    int term_height, term_width; // 터미널 크기 받는 변수
    getmaxyx(stdscr, term_height, term_width); // 터미널 크기 저장
    
    /* 박스 크기 변수 */
    int box_height = GRID_HEIGHT;
    int box_width = GRID_WIDTH;
    status.box_width = box_width; 
    status.box_height = box_height;
    
    /* 박스 시작 좌표 */
    int start_y = (term_height - box_height) / 2;
    int start_x = (term_width - box_width) / 2;

    /* 윈도우 생성 */
    WINDOW *win = newwin(box_height, box_width, start_y, start_x);
    
    /* 윈도우 테두리 그리기 */
    box(win, '!', '=');
    
    /* 테트리미노의 상대좌표 */
    Tetrimino *tets = generate_tets(); 
    for (int i = start_y+1; i < start_y + box_height-1; i++) {
        mvwprintw(stdscr, i, start_x-1, "%s", "<");
        mvwprintw(stdscr, i, start_x + box_width, "%s", ">");
    }
    for (int j = start_x; j < start_x + box_width; j += 2) {
        mvwprintw(stdscr, start_y + box_height, j, "%s", "\\/");
    }
    /* 상단 메시지 */ 
    char* hello_str = "Tetris";
    mvwprintw(stdscr, start_y - 1, 
            ((term_width - strlen(hello_str)) / 2), 
            "%s\n",
            hello_str);
    
    /* 테트리스 그리드 초기화 */
    char **grid = generate_grid(box_height, box_width); 
    
    status.interface_x = start_x;
    status.interface_y = start_y; 
    
    Tetrimino new_tet = make_tet((box_width / 2), 1, tets[dumb_idx]); 
    Tetrimino old_tet = new_tet;
    status.tet = tets[(dumb_idx + 1) % 7];
    
    draw_interface(status, data);
    
    while(1) { /* while - 프레임 갱신 */ 
        tmp_ch = getch();
        if (tmp_ch != -1) ch = tmp_ch;
        
        erase_tet(win, box_height, box_width, old_tet); 
        
        if (is_collide(grid, new_tet)) {
            int exit = 0;

            int line_cnt = update_grid(grid, old_tet, box_height, box_width);
            line_cleared += line_cnt;
            level = line_cleared / LEVEL_SLOPE; // 0, 1, 2, ...
            
            if (line_cnt != 0) score += line_score(line_cnt, level);
            /* 레벨 올라갈수록 게임 속도 증가 */ 
            // NOTE:: 레벨 올라갈 때 한 번만 바꾸기 
            // if (tick > 150000) tick -= level * 500;

            set_ticker(tick);
            
            dumb_idx = (dumb_idx + 1) % 7; 
            next_dumb_idx = (dumb_idx + 1) % 7;
            status.line = line_cnt; // 이번에 삭제된 라인 수 
            status.line_cleared = line_cleared; // 지금까지 삭제된 라인 수 
            status.level = level;
            status.score = score;
            status.tet = tets[next_dumb_idx];

            draw_interface(status, data);
            
            new_tet = make_tet((box_width / 2), 1, tets[dumb_idx]); 
            
            draw_grid(win, grid, box_height, box_width); 
            
            /* 그리드를 넘어가면 종료 */
            for (int i = 1; i < box_width; i++) {
                if (grid[1][i] == 1) exit = 1;
            }
            if (exit) break;
            
            continue;
        }
        old_tet = new_tet;
        status.tet = old_tet;
        /* NOTE:: 
         * 회전 상대좌표 수정하기
         * 오른쪽 벽에서도 rotate 가능하게
         * 네모블럭 회전 비교하기(?)
         * 왼쪽회전, 오른쪽회전 구현하기 
         */

        /* update */
        Tetrimino tmp;
        if (ch == 'q') break;
        else if (ch == 'x' || ch == KEY_UP) { // 테트로미노 회전 
            new_tet = rotate_tet(grid, status, "cw");
        } else if (ch == 'z'){
            new_tet = rotate_tet(grid, status, "ccw");
        } else if (ch == 'l' || ch == KEY_RIGHT) {
            tmp = move_tet('l', old_tet, box_height, box_width);
            if (is_inside(box_height, box_width, tmp)) new_tet = tmp;
        } else if (ch == 'h' || ch == KEY_LEFT) {
            tmp = move_tet('h', old_tet, box_height, box_width);
            if (is_inside(box_height, box_width, tmp)) new_tet = tmp;
        } 
        else if (ch == 'j' || ch == KEY_DOWN) { 
            set_ticker(TICK / 3);
            set_ticker(15000);
        } 
        else if (ch == 'r') {
            execvp(argv[0], argv);
        }
        ch = -1;

        if(get_tick()) {
            new_tet = move_tet('d', new_tet, box_height, box_width);
            // ch = -1;
            set_tick();
        }

        draw_grid(win, grid, box_height, box_width); 
        draw_ghost_line(win, grid, old_tet, box_height);
        draw_tet(win, box_height, box_width, old_tet);
        usleep(16000);
        wrefresh(win);
    }
    /* while(1) 루프 끝나면 */
    /*
    if (score > prev_score) {
        FILE *fp = fopen(".score.txt", "w");
        fprintf(fp, "%d", score);
        fclose(fp);
    }*/
     
    // if (line_cnt > data.prev_max_line) save_data(data);
    data.max_line = line_cleared;
    data.max_score = score;
    data.max_level = level;

    save_data(data);

    char* exit_str = "Press any key to exit...";
    char* restart = "Press 'r' to restart";
    int bottom = ((term_width - strlen(exit_str)) / 2);

    mvwprintw(stdscr, box_height + 11, bottom, "%s", exit_str);
    mvwprintw(stdscr, box_height + 12, bottom, "%s", restart);

    wrefresh(win);
    nodelay(stdscr, FALSE);
    
    ch = getch();
   
    if (ch == 'r') {
        delwin(win);
        endwin();
        pid_t pid = fork();
        if (pid == 0) {
            execvp(argv[0], argv);
            perror("execvp");
            exit(1);
        } else {
            // waitpid(pid, NULL, 0);
            exit(0);
        }
    }

    delwin(win);
    endwin();

    for (int i = 0; i < box_height; i++) {
        free(grid[i]);
    }
    free(grid);
    free(tets);

    return 0;
}

int line_score(int line_cnt, int level) {
    if (level == 0) level = 1;
    if (line_cnt == 1) return 100 * level;
    else if (line_cnt == 2) return 300 * level;
    else if (line_cnt == 3) return 500 * level;
    else return 800 * level;
}

