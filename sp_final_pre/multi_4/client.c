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
}

void draw_ui(int player_id, const GameState *state) {
    attron(COLOR_PAIR(1));
    mvprintw(state->start_y - 2, state->start_x, "P1: %d", state->players[0].score);
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2));
    mvprintw(state->start_y - 2, state->start_x + 25, "P2: %d", state->players[1].score);
    attroff(COLOR_PAIR(2));

    attron(COLOR_PAIR(4));
    mvprintw(state->start_y - 2, state->start_x + 50, "TIME: %03d", state->remaining_time);
    mvprintw(state->start_y - 1, state->start_x, "You: P%d | Move: Arrows | Quit: Q", player_id + 1);
    attroff(COLOR_PAIR(4));
}

void render_game(int my_id, const GameState *state) {
    clear();
    
    // 테두리
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

    // P1
    if (!state->players[0].is_dead) {
        attron(COLOR_PAIR(1));
        for (int i = 0; i < state->players[0].length; i++) 
            mvaddch(state->players[0].body[i].y, state->players[0].body[i].x, (i==0 ? 'O' : 'o'));
        attroff(COLOR_PAIR(1));
    }

    // P2
    if (!state->players[1].is_dead) {
        attron(COLOR_PAIR(2));
        for (int i = 0; i < state->players[1].length; i++) 
            mvaddch(state->players[1].body[i].y, state->players[1].body[i].x, (i==0 ? 'X' : 'x'));
        attroff(COLOR_PAIR(2));
    }

    draw_ui(my_id, state);
}

void render_game_over(int my_id, const GameState *state, int term_h, int term_w) {
    clear();
    int cy = term_h/2;
    int cx = term_w/2;
    
    mvprintw(cy - 5, cx - 5, "GAME OVER");
    
    attron(COLOR_PAIR(1));
    mvprintw(cy - 2, cx - 10, "P1 Score: %d", state->players[0].score);
    attroff(COLOR_PAIR(1));
    
    attron(COLOR_PAIR(2));
    mvprintw(cy - 1, cx - 10, "P2 Score: %d", state->players[1].score);
    attroff(COLOR_PAIR(2));

    attron(COLOR_PAIR(4));
    if (state->winner_id == 3) {
         mvprintw(cy + 1, cx - 12, "[Time Limit Exceeded!]");
         if (state->players[0].score > state->players[1].score) mvprintw(cy + 3, cx - 8, "P1 Wins!");
         else if (state->players[1].score > state->players[0].score) mvprintw(cy + 3, cx - 8, "P2 Wins!");
         else mvprintw(cy + 3, cx - 4, "Draw!");
    } else {
        if (state->winner_id == my_id) {
            mvprintw(cy + 2, cx - 10, "YOU WIN! :)");
        } else if (state->winner_id == (my_id + 1) % PLAYER_COUNT) {
            mvprintw(cy + 2, cx - 10, "YOU LOSE! :(");
        } else {
            mvprintw(cy + 2, cx - 5, "DRAW!");
        }
    }

    mvprintw(cy + 5, cx - 15, "Press 'R' to Restart, 'Q' to Quit");
    attroff(COLOR_PAIR(4));
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
        // 1. 서버로부터 상태 수신
        if (recv(sock, &state, sizeof(GameState), 0) <= 0) {
            break; // 서버 종료됨
        }

        // 2. 화면 렌더링 (게임 중 vs 게임 오버)
        if (state.winner_id == -1) {
            render_game(my_id, &state);
        } else {
            render_game_over(my_id, &state, term_h, term_w);
        }
        refresh();

        // 3. 키 입력 처리 및 전송
        int ch = getch();
        if (ch != ERR) {
            char input_cmd = 0;
            
            // 게임 중일 때
            if (state.winner_id == -1) {
                if (ch == KEY_UP) input_cmd = 'U';
                else if (ch == KEY_DOWN) input_cmd = 'D';
                else if (ch == KEY_LEFT) input_cmd = 'L';
                else if (ch == KEY_RIGHT) input_cmd = 'R';
                else if (ch == 'q' || ch == 'Q') input_cmd = 'Q'; // 자살/항복
            } 
            // 게임 오버일 때
            else {
                if (ch == 'r' || ch == 'R') input_cmd = 'R'; // 재시작 요청
                else if (ch == 'q' || ch == 'Q') break;      // 클라이언트 종료
            }

            if (input_cmd != 0) {
                send(sock, &input_cmd, 1, 0);
            }
        }
        
        usleep(10000); // 10ms
    }

    endwin();
    close(sock);
    return 0;
}
