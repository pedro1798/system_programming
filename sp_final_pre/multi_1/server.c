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

// [공통 정의]
#define PORT 12345
#define MAX_SNAKE_LEN 1024
#define TIMEOUT_SEC 180
#define BOX_WIDTH 40
#define BOX_HEIGHT 20

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

volatile sig_atomic_t remaining_time = TIMEOUT_SEC;

void handler(int sig, siginfo_t *si, void *uc) {
    if (sig == SIGUSR1) {
        if (remaining_time > 0) remaining_time--;
    }
}

void make_timer(timer_t *timerID, int signo, int sec, int interval_sec) {
    struct sigevent sev;
    struct itimerspec its;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = signo;
    sev.sigev_value.sival_ptr = timerID;
    timer_create(CLOCK_REALTIME, &sev, timerID);
    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = interval_sec;
    its.it_interval.tv_nsec = 0;
    timer_settime(*timerID, 0, &its, NULL);
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
    state->remaining_time = TIMEOUT_SEC; // [중요] 초기 시간 설정
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

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;

    server_sock = socket(PF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);
    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_sock, 1);
    
    printf("Waiting for Challenger...\n");
    client_addr_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size);
    
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    srand(time(NULL));

    GameState my_state, opp_state;
    // opp_state 초기화 (쓰레기값 방지)
    memset(&opp_state, 0, sizeof(GameState));
    opp_state.remaining_time = TIMEOUT_SEC; 

    reset_game_state(&my_state);
    my_state.game_round = 1; 

    int dir_x = 1, dir_y = 0;
    
    timer_t timer_tick;
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    make_timer(&timer_tick, SIGUSR1, 1, 1);

    int running = 1;
    while (running) {
        int ch = getch();
        if (ch == 'q') running = 0;
        else if (ch == 'r') my_state.want_restart = 1;
        else if (ch == 's') my_state.is_paused = !my_state.is_paused;

        if (my_state.want_restart || opp_state.want_restart) {
            int next_round = my_state.game_round + 1;
            reset_game_state(&my_state);
            my_state.game_round = next_round;
            remaining_time = TIMEOUT_SEC;
            dir_x = 1; dir_y = 0;
            opp_state.want_restart = 0;
        }

        // 시간 동기화
        my_state.remaining_time = remaining_time;
        
        if (send(client_sock, &my_state, sizeof(my_state), 0) == -1) break;
        if (recv_all(client_sock, &opp_state, sizeof(opp_state)) <= 0) break;

        if (opp_state.remaining_time == -999) break;

        int is_any_paused = my_state.is_paused || opp_state.is_paused;
        int is_time_up = (remaining_time <= 0);

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

        erase();
        if (is_any_paused) mvprintw(3, 30, "--- PAUSED (Press 's') ---");
        mvprintw(1, 2, "SERVER (P1) | Time: %d | Press 'r' to Restart, 'q' to Quit", remaining_time);
        draw_board(5, 2, &my_state, "YOU");
        draw_board(5, 45, &opp_state, "PEER");
        if (is_time_up) mvprintw(3, 30, "--- TIME UP ---");

        refresh();
        napms(100);
    }

    timer_delete(timer_tick);
    endwin();
    close(client_sock);
    close(server_sock);
    return 0;
}
