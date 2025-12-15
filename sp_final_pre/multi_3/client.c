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

// ==========================================
// 1. 통신 및 게임 상수 정의
// ==========================================
#define PORT 12345
#define SERVER_IP "127.0.0.1" // 같은 PC에서 실행 시
#define MAX_SNAKE_LEN 1024
#define FOOD_COUNT 5
#define PLAYER_COUNT 2

// ==========================================
// 2. 데이터 구조체 정의 (서버와 공유)
// ==========================================
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

// ==========================================
// 3. ncurses 출력 함수
// ==========================================
void setup_ncurses_colors() {
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // P1 (Green)
    init_pair(2, COLOR_RED, COLOR_BLACK);   // P2 (Red)
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Food
    init_pair(4, COLOR_WHITE, COLOR_BLACK);  // Wall/UI
}

void draw_ui(int player_id, const GameState *state) {
    // P1 정보
    attron(COLOR_PAIR(1));
    mvprintw(state->start_y - 2, state->start_x, "P1 Score: %d %s", state->players[0].score, state->players[0].is_dead ? "[DEAD]" : "");
    attroff(COLOR_PAIR(1));

    // P2 정보
    attron(COLOR_PAIR(2));
    mvprintw(state->start_y - 2, state->start_x + 25, "P2 Score: %d %s", state->players[1].score, state->players[1].is_dead ? "[DEAD]" : "");
    attroff(COLOR_PAIR(2));

    // 시간 정보
    attron(COLOR_PAIR(4));
    mvprintw(state->start_y - 2, state->start_x + 50, "TIME: %03d sec", state->remaining_time);
    
    mvprintw(state->start_y - 1, state->start_x, "You are Player %d. Control with Arrow Keys. (Q to Quit)", player_id + 1);
    attroff(COLOR_PAIR(4));
}

void render_game(int my_id, const GameState *state) {
    clear();
    
    // 맵 테두리
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

    // P1 그리기
    if (!state->players[0].is_dead) {
        attron(COLOR_PAIR(1));
        for (int i = 0; i < state->players[0].length; i++) 
            mvaddch(state->players[0].body[i].y, state->players[0].body[i].x, (i==0 ? 'O' : 'o'));
        attroff(COLOR_PAIR(1));
    }

    // P2 그리기
    if (!state->players[1].is_dead) {
        attron(COLOR_PAIR(2));
        for (int i = 0; i < state->players[1].length; i++) 
            mvaddch(state->players[1].body[i].y, state->players[1].body[i].x, (i==0 ? 'X' : 'x'));
        attroff(COLOR_PAIR(2));
    }

    draw_ui(my_id, state);
    refresh();
}

void draw_game_over(int my_id, const GameState *state, int term_h, int term_w) {
    clear();
    int cy = term_h/2;
    int cx = term_w/2;
    
    mvprintw(cy - 4, cx - 5, "GAME OVER");
    mvprintw(cy + 2, cx - 10, "P1 Score: %d", state->players[0].score);
    mvprintw(cy + 3, cx - 10, "P2 Score: %d", state->players[1].score);

    // 승자 결정 메시지
    attron(COLOR_PAIR(4));
    if (state->winner_id == my_id) {
        mvprintw(cy, cx - 10, "YOU WIN! (Player %d)", my_id + 1);
    } else if (state->winner_id == (my_id + 1) % PLAYER_COUNT) {
        mvprintw(cy, cx - 10, "YOU LOSE! (Player %d Wins)", (my_id + 1) % PLAYER_COUNT + 1);
    } else if (state->winner_id == 2) {
        mvprintw(cy, cx - 5, "DRAW!");
    } else {
        // 타이머 끝나기 전에 접속이 끊기거나 서버가 종료된 경우
        mvprintw(cy, cx - 10, "Game Ended Unexpectedly.");
    }

    if (state->remaining_time <= 0 && state->winner_id != -1) {
        mvprintw(cy - 2, cx - 12, "Time Limit Exceeded! (Highest Score Wins)");
    }
    
    mvprintw(cy + 5, cx - 15, "Press 'q' to Quit");
    attroff(COLOR_PAIR(4));
    refresh();

    while(getch() != 'q');
}

// ==========================================
// 4. 메인 함수
// ==========================================
int main(int argc, char const *argv[]) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    GameState state;
    int my_id = -1;

    // 1. 소켓 연결
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    printf("Connecting to server %s:%d...\n", SERVER_IP, PORT);
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        // 서버가 켜져 있지 않거나 방화벽 문제일 가능성이 높습니다.
        printf("\nConnection Failed. Ensure server is running on %s:%d.\n", SERVER_IP, PORT);
        return -1;
    }
    printf("Connection successful.\n");

    // 2. 플레이어 ID 수신
    if (recv(sock, &my_id, sizeof(int), 0) <= 0 || my_id == -1) {
        printf("Failed to receive player ID or server is full.\n");
        close(sock);
        return -1;
    }
    printf("Assigned as Player %d. Waiting for game start...\n", my_id + 1);

    // 3. Ncurses 초기화
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    setup_ncurses_colors();

    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width);

    // 4. 게임 루프
    int game_running = 1;
    while (game_running) {
        // (A) 서버로부터 게임 상태 수신
        valread = recv(sock, &state, sizeof(GameState), 0);
        if (valread == sizeof(GameState)) {
            render_game(my_id, &state);
            if (state.winner_id != -1) {
                game_running = 0;
            }
        } else if (valread == 0) {
            // 서버 연결 끊김
            game_running = 0;
            printf("Server disconnected. Game over.\n");
        }
        
        // (B) 사용자 입력 처리 및 서버로 전송
        int ch = getch();
        if (ch != ERR) {
            char input_dir = 'N'; // No movement
            if (ch == KEY_UP) input_dir = 'U';
            else if (ch == KEY_DOWN) input_dir = 'D';
            else if (ch == KEY_LEFT) input_dir = 'L';
            else if (ch == KEY_RIGHT) input_dir = 'R';
            else if (ch == 'q' || ch == 'Q') {
                input_dir = 'Q';
                game_running = 0;
            }

            if (input_dir != 'N') {
                send(sock, &input_dir, 1, 0);
            }
        }

        usleep(10000); // 10ms
    }

    // 5. 종료 화면
    draw_game_over(my_id, &state, term_height, term_width);

    endwin();
    close(sock);
    return 0;
}
