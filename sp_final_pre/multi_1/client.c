#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ncurses.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define PORT 12345
#define MAX_SNAKE_LEN 1024
#define BOX_WIDTH 40
#define BOX_HEIGHT 20
#define TIMEOUT_SEC 180

typedef struct Point {
    int x, y;
} Point;

typedef struct {
    Point snake[MAX_SNAKE_LEN];
    int snake_len;
    Point food;
    int score;
    int is_game_over;
    int remaining_time;
    int is_paused;
    int want_restart;
    int game_round;
} GameState;

int recv_all(int sock, void *buffer, size_t size) {
    size_t total_received = 0;
    char *ptr = (char *)buffer;
    while (total_received < size) {
        ssize_t received = recv(sock, ptr + total_received, size - total_received, 0);
        if (received < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        if (received == 0) return 0;
        total_received += received;
    }
    return 1;
}

void reset_game_state(GameState *state) {
    memset(state, 0, sizeof(GameState));
    state->snake_len = 3;
    for(int i=0; i<3; i++) {
        state->snake[i].x = BOX_WIDTH/2 - i;
        state->snake[i].y = BOX_HEIGHT/2;
    }
    state->food.x = (rand() % (BOX_WIDTH - 2)) + 1;
    state->food.y = (rand() % (BOX_HEIGHT - 2)) + 1;
    // [중요] 초기화 시 시간도 충분히 주어서 즉시 게임오버 되는 것 방지
    state->remaining_time = TIMEOUT_SEC; 
}

void draw_board(int start_y, int start_x, GameState *state, char *title) {
    for (int y = 0; y < BOX_HEIGHT; y++) {
        for (int x = 0; x < BOX_WIDTH; x++) {
            if (y == 0 || y == BOX_HEIGHT - 1 || x == 0 || x == BOX_WIDTH - 1) {
                mvaddch(start_y + y, start_x + x, '#');
            }
        }
    }
    mvprintw(start_y - 1, start_x, "%s - Score: %d", title, state->score);

    if (state->is_game_over || state->remaining_time <= 0) {
        mvprintw(start_y + BOX_HEIGHT / 2, start_x + BOX_WIDTH / 2 - 4, "GAME OVER");
    } else {
        for (int i = 0; i < state->snake_len; i++) {
            mvaddch(start_y + state->snake[i].y, start_x + state->snake[i].x, (i == 0 ? 'O' : 'o'));
        }
        mvaddch(start_y + state->food.y, start_x + state->food.x, '@');
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <Server IP>\n", argv[0]);
        exit(1);
    }

    int sock;
    struct sockaddr_in server_addr;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect error"); exit(1);
    }

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    srand(time(NULL) + 1);

    GameState my_state, opp_state;
    // opp_state 초기 쓰레기값 방지
    memset(&opp_state, 0, sizeof(GameState));
    opp_state.remaining_time = TIMEOUT_SEC; 

    reset_game_state(&my_state);
    
    int current_round = 0; 
    int dir_x = 1, dir_y = 0;
    int remaining_time = TIMEOUT_SEC;

    int running = 1;
    while (running) {
        int ch = getch();
        if (ch == 'q') {
            running = 0;
            my_state.remaining_time = -999;
        } else if (ch == 'r') {
            my_state.want_restart = 1;
        } else if (ch == 's') {
            my_state.is_paused = !my_state.is_paused;
        }

        // --- 데이터 수신 ---
        if (recv_all(sock, &opp_state, sizeof(opp_state)) <= 0) break;

        // 라운드 체크 (재시작)
        if (opp_state.game_round > current_round) {
            current_round = opp_state.game_round;
            reset_game_state(&my_state);
            dir_x = 1; dir_y = 0;
        }

        remaining_time = opp_state.remaining_time;
        if (remaining_time == -999) break;

        // [핵심 수정] 내 상태의 시간을 서버 시간과 동기화
        // 이 줄이 없으면 memset으로 인해 시간이 0이 되어 스스로 Game Over를 띄움
        my_state.remaining_time = remaining_time;

        int is_any_paused = my_state.is_paused || opp_state.is_paused;
        int is_time_up = (remaining_time <= 0);

        // --- 데이터 송신 ---
        if (send(sock, &my_state, sizeof(my_state), 0) == -1) break;

        // --- 로직 업데이트 ---
        if (!is_any_paused && !my_state.is_game_over && !is_time_up) {
            if (ch == KEY_UP && dir_y == 0) { dir_x = 0; dir_y = -1; }
            else if (ch == KEY_DOWN && dir_y == 0) { dir_x = 0; dir_y = 1; }
            else if (ch == KEY_LEFT && dir_x == 0) { dir_x = -1; dir_y = 0; }
            else if (ch == KEY_RIGHT && dir_x == 0) { dir_x = 1; dir_y = 0; }

            Point tail = my_state.snake[my_state.snake_len - 1];
            for (int i = my_state.snake_len - 1; i > 0; i--) {
                my_state.snake[i] = my_state.snake[i - 1];
            }
            my_state.snake[0].x += dir_x;
            my_state.snake[0].y += dir_y;

            if (my_state.snake[0].x <= 0 || my_state.snake[0].x >= BOX_WIDTH - 1 ||
                my_state.snake[0].y <= 0 || my_state.snake[0].y >= BOX_HEIGHT - 1) {
                my_state.is_game_over = 1;
            }
            for (int i = 1; i < my_state.snake_len; i++) {
                if (my_state.snake[0].x == my_state.snake[i].x && my_state.snake[0].y == my_state.snake[i].y)
                    my_state.is_game_over = 1;
            }
            if (my_state.snake[0].x == my_state.food.x && my_state.snake[0].y == my_state.food.y) {
                my_state.score += 10;
                if (my_state.snake_len < MAX_SNAKE_LEN) {
                    my_state.snake[my_state.snake_len] = tail;
                    my_state.snake_len++;
                }
                my_state.food.x = (rand() % (BOX_WIDTH - 2)) + 1;
                my_state.food.y = (rand() % (BOX_HEIGHT - 2)) + 1;
            }
        }

        // --- 렌더링 ---
        erase();
        if (is_any_paused) mvprintw(3, 30, "--- PAUSED (Press 's') ---");
        mvprintw(1, 2, "CLIENT (P2) | Time: %d | Press 'r' to Restart, 'q' to Quit", remaining_time);
        
        draw_board(5, 2, &opp_state, "PEER");
        draw_board(5, 45, &my_state, "YOU");

        if (is_time_up) mvprintw(3, 30, "--- TIME UP ---");

        refresh();
        napms(100);
    }

    endwin();
    close(sock);
    return 0;
}
