#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ncurses.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1" 
#define MAX_SNAKE_LEN 1024
#define FOOD_COUNT 5
#define PLAYER_COUNT 2

typedef struct Point {
    int x, y;
} Point;

typedef struct SnakeData {
    Point body[MAX_SNAKE_LEN];
    int length;
    int score;
    int is_dead;
} SnakeData;

typedef struct GameState {
    SnakeData players[PLAYER_COUNT];
    Point foods[FOOD_COUNT];
    int remaining_time;
    int winner_id;
    int map_width, map_height;
    int start_x, start_y;
} GameState;

void setup_ncurses_colors() {
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK); 
    init_pair(2, COLOR_RED, COLOR_BLACK);   
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); 
    init_pair(4, COLOR_WHITE, COLOR_BLACK); 
    init_pair(5, COLOR_BLACK, COLOR_WHITE); // UI 배경
}

// [수정] UI를 화면 최상단(0, 1행)에 강제 고정
void draw_ui(int my_id, const GameState *state) {
    int opp_id = (my_id + 1) % PLAYER_COUNT;
    int my_score = state->players[my_id].score;
    int opp_score = state->players[opp_id].score;
    
    // UI 배경용 빈 줄 출력 (잔상 제거)
    move(0, 0); clrtoeol();
    move(1, 0); clrtoeol();

    // 1. 왼쪽 위: 남은 시간
    attron(COLOR_PAIR(4) | A_BOLD);
    mvprintw(0, 2, "TIME: %03d sec", state->remaining_time);
    
    // 내가 죽었는지 표시
    if (state->players[my_id].is_dead) {
        attron(COLOR_PAIR(2));
        mvprintw(0, 20, "YOU DIED! (Watching...)");
        attroff(COLOR_PAIR(2));
    }
    attroff(COLOR_PAIR(4) | A_BOLD);

    // 2. 오른쪽 위: 상대방 점수와 자신의 점수
    char score_str[100];
    snprintf(score_str, sizeof(score_str), "[Opp: %d]  [Me: %d]", opp_score, my_score);
    int right_x = state->map_width - strlen(score_str); // 맵 너비 기준 오른쪽 정렬
    if (right_x < 30) right_x = 30; // 최소 위치 보장

    attron(COLOR_PAIR(4));
    mvprintw(0, right_x, "%s", score_str);
    attroff(COLOR_PAIR(4));
}

void render_game(int my_id, const GameState *state) {
    clear();
    
    // 맵 그리기
    attron(COLOR_PAIR(4));
    for (int y = state->start_y; y < state->start_y + state->map_height; y++) {
        for (int x = state->start_x; x < state->start_x + state->map_width; x++) {
            if (y == state->start_y || y == state->start_y + state->map_height - 1 || 
                x == state->start_x || x == state->start_x + state->map_width - 1) {
                mvaddch(y, x, '#');
            }
        }
    }
    attroff(COLOR_PAIR(4));

    // 먹이
    attron(COLOR_PAIR(3));
    for(int i=0; i<FOOD_COUNT; i++) mvaddch(state->foods[i].y, state->foods[i].x, '@');
    attroff(COLOR_PAIR(3));

    // P1 (Green)
    if (!state->players[0].is_dead) {
        attron(COLOR_PAIR(1));
        for (int i = 0; i < state->players[0].length; i++) 
            mvaddch(state->players[0].body[i].y, state->players[0].body[i].x, (i==0 ? 'O' : 'o'));
        attroff(COLOR_PAIR(1));
    } else {
        // 죽은 뱀은 회색(혹은 흐리게) 처리, 일단 흰색
        attron(COLOR_PAIR(4) | A_DIM);
        for (int i = 0; i < state->players[0].length; i++) 
            mvaddch(state->players[0].body[i].y, state->players[0].body[i].x, 'x');
        attroff(COLOR_PAIR(4) | A_DIM);
    }

    // P2 (Red)
    if (!state->players[1].is_dead) {
        attron(COLOR_PAIR(2));
        for (int i = 0; i < state->players[1].length; i++) 
            mvaddch(state->players[1].body[i].y, state->players[1].body[i].x, (i==0 ? 'X' : 'x'));
        attroff(COLOR_PAIR(2));
    } else {
        attron(COLOR_PAIR(4) | A_DIM);
        for (int i = 0; i < state->players[1].length; i++) 
            mvaddch(state->players[1].body[i].y, state->players[1].body[i].x, 'x');
        attroff(COLOR_PAIR(4) | A_DIM);
    }

    draw_ui(my_id, state);
}

void render_game_over(int my_id, const GameState *state, int term_h, int term_w) {
    clear();
    int cy = term_h/2;
    int cx = term_w/2;
    
    attron(COLOR_PAIR(4));
    mvprintw(cy - 5, cx - 5, "GAME OVER");
    mvprintw(cy - 2, cx - 10, "Final Score");
    mvprintw(cy - 1, cx - 10, "----------------");
    mvprintw(cy, cx - 10, "ME : %d", state->players[my_id].score);
    mvprintw(cy + 1, cx - 10, "OPP: %d", state->players[(my_id+1)%2].score);

    if (state->winner_id == 3) {
         mvprintw(cy + 3, cx - 12, "[Time Limit Exceeded!]");
    } 
    
    if (state->winner_id == my_id || (state->winner_id < 2 && state->winner_id == my_id)) {
        attron(A_BOLD);
        mvprintw(cy + 4, cx - 5, "YOU WIN!");
        attroff(A_BOLD);
    } else if (state->winner_id == 2) {
        mvprintw(cy + 4, cx - 2, "DRAW");
    } else {
        mvprintw(cy + 4, cx - 5, "YOU LOSE...");
    }

    mvprintw(cy + 6, cx - 18, "Press 'R' to Restart for BOTH players");
    mvprintw(cy + 7, cx - 10, "Press 'Q' to Quit");
    attroff(COLOR_PAIR(4));
    refresh();
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    GameState state;
    int my_id = -1;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed.\n");
        return -1;
    }

    if (recv(sock, &my_id, sizeof(int), 0) <= 0) return -1;

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    setup_ncurses_colors();

    int term_h, term_w;
    getmaxyx(stdscr, term_h, term_w);

    while (1) {
        if (recv(sock, &state, sizeof(GameState), 0) <= 0) break;

        if (state.winner_id == -1) {
            render_game(my_id, &state);
        } else {
            render_game_over(my_id, &state, term_h, term_w);
        }
        refresh();

        int ch = getch();
        if (ch != ERR) {
            char input_cmd = 0;
            if (state.winner_id == -1) {
                if (ch == KEY_UP) input_cmd = 'U';
                else if (ch == KEY_DOWN) input_cmd = 'D';
                else if (ch == KEY_LEFT) input_cmd = 'L';
                else if (ch == KEY_RIGHT) input_cmd = 'R';
                else if (ch == 'q' || ch == 'Q') input_cmd = 'Q';
            } else {
                if (ch == 'r' || ch == 'R') input_cmd = 'R';
                else if (ch == 'q' || ch == 'Q') break;
            }
            if (input_cmd != 0) send(sock, &input_cmd, 1, 0);
        }
        usleep(10000); 
    }

    endwin();
    close(sock);
    return 0;
}
