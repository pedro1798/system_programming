#define _XOPEN_SOURCE 700 
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h> 
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h> 

#define PORT 12345
#define MAX_CLIENTS 2
#define TIMEOUT_SEC 180
#define MAX_SNAKE_LEN 1024
#define FOOD_COUNT 5
#define PLAYER_COUNT 2
#define GAME_TICK_MS 100 

typedef struct Point {
    int x, y;
} Point;

typedef struct SnakeData {
    Point body[MAX_SNAKE_LEN];
    int length;
    int score;
    int is_dead;
} SnakeData;

typedef struct Snake {
    SnakeData data;
    int dir_x, dir_y;
    int client_fd; 
} Snake;

typedef struct GameState {
    SnakeData players[PLAYER_COUNT];
    Point foods[FOOD_COUNT];
    int remaining_time;
    int winner_id; 
    int map_width, map_height;
    int start_x, start_y;
} GameState;

volatile sig_atomic_t remaining_time = TIMEOUT_SEC;
volatile sig_atomic_t game_tick_flag = 0;

Snake players[PLAYER_COUNT];
Point foods[FOOD_COUNT];
int game_map_width = 60;
int game_map_height = 25;
// [수정] UI 공간 확보를 위해 Y 시작점을 3으로 내림
int box_start_x = 2; 
int box_start_y = 3; 

pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

int get_winner_id();

void timer_handler(int sig, siginfo_t *si, void *uc) {
    if (sig == SIGUSR1) {
        if (get_winner_id() == -1 && remaining_time > 0) {
            remaining_time--;
        }
    } else if (sig == SIGALRM) {
        game_tick_flag = 1;
    }
}

void setup_timers() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO; 
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);

    timer_t timer_tick;
    struct sigevent sev;
    struct itimerspec its;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = &timer_tick;
    timer_create(CLOCK_REALTIME, &sev, &timer_tick);
    
    its.it_value.tv_sec = 1;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 1;
    its.it_interval.tv_nsec = 0;
    timer_settime(timer_tick, 0, &its, NULL);

    struct itimerval it_val;
    it_val.it_value.tv_sec = 0;
    it_val.it_value.tv_usec = GAME_TICK_MS * 1000;
    it_val.it_interval.tv_sec = 0;
    it_val.it_interval.tv_usec = GAME_TICK_MS * 1000;
    setitimer(ITIMER_REAL, &it_val, NULL);
}

void spawn_food(Point *target_food) {
    while(1) {
        int fx = (rand() % (game_map_width - 2)) + box_start_x + 1;
        int fy = (rand() % (game_map_height - 2)) + box_start_y + 1;
        
        int overlap = 0;
        for(int p=0; p<PLAYER_COUNT; p++) {
             // 죽은 뱀 자리에도 먹이가 안 생기게 하려면 이 조건 유지
             // 죽은 뱀이 사라지게 하려면 !is_dead 조건 추가
             for(int i=0; i<players[p].data.length; i++) {
                if(players[p].data.body[i].x == fx && players[p].data.body[i].y == fy) overlap = 1;
            }
        }
        for(int i=0; i<FOOD_COUNT; i++) if(foods[i].x == fx && foods[i].y == fy) overlap = 1;

        if(!overlap) {
            target_food->x = fx;
            target_food->y = fy;
            break;
        }
    }
}

void init_game() {
    remaining_time = TIMEOUT_SEC;
    
    // P1
    players[0].data.length = 5;
    players[0].data.score = 0;
    players[0].dir_x = 1; players[0].dir_y = 0;
    players[0].data.is_dead = 0;
    for (int i = 0; i < players[0].data.length; i++) {
        players[0].data.body[i].x = box_start_x + 5 - i;
        players[0].data.body[i].y = box_start_y + game_map_height / 2;
    }

    // P2
    players[1].data.length = 5;
    players[1].data.score = 0;
    players[1].dir_x = -1; players[1].dir_y = 0;
    players[1].data.is_dead = 0;
    for (int i = 0; i < players[1].data.length; i++) {
        players[1].data.body[i].x = box_start_x + game_map_width - 6 + i;
        players[1].data.body[i].y = box_start_y + game_map_height / 2;
    }

    for(int i=0; i<FOOD_COUNT; i++) spawn_food(&foods[i]);
    printf("Game Started/Reset!\n");
}

void update_game_logic() {
    int i, p, q;
    for (p = 0; p < PLAYER_COUNT; p++) {
        // [중요] 죽은 플레이어는 움직이지 않음 (그대로 멈춰서 장애물이 됨)
        if (players[p].data.is_dead) continue;

        Point tail = players[p].data.body[players[p].data.length - 1]; 
        for (i = players[p].data.length - 1; i > 0; i--) {
            players[p].data.body[i] = players[p].data.body[i - 1];
        }
        players[p].data.body[0].x += players[p].dir_x;
        players[p].data.body[0].y += players[p].dir_y;

        // 벽 충돌
        if (players[p].data.body[0].x <= box_start_x || players[p].data.body[0].x >= box_start_x + game_map_width - 1 ||
            players[p].data.body[0].y <= box_start_y || players[p].data.body[0].y >= box_start_y + game_map_height - 1) {
            players[p].data.is_dead = 1;
        }

        // 자기 몸 충돌
        for (i = 1; i < players[p].data.length; i++) {
            if (players[p].data.body[0].x == players[p].data.body[i].x && players[p].data.body[0].y == players[p].data.body[i].y) {
                players[p].data.is_dead = 1;
            }
        }
        
        // 상대 충돌 (죽은 상대 몸에도 부딪힐 수 있음)
        q = (p + 1) % PLAYER_COUNT;
        for (i = 0; i < players[q].data.length; i++) {
            if (players[p].data.body[0].x == players[q].data.body[i].x && players[p].data.body[0].y == players[q].data.body[i].y) {
                players[p].data.is_dead = 1;
                break;
            }
        }

        // 먹이
        for (i = 0; i < FOOD_COUNT; i++) {
            if (players[p].data.body[0].x == foods[i].x && players[p].data.body[0].y == foods[i].y) {
                players[p].data.score += 10;
                if (players[p].data.length < MAX_SNAKE_LEN) {
                    players[p].data.body[players[p].data.length] = tail;
                    players[p].data.length++;
                }
                spawn_food(&foods[i]);
            }
        }
    }
}

// [수정] 승자 판별 로직: 한 명이 살아있으면 계속 진행 (-1 리턴)
int get_winner_id() {
    int p1_dead = players[0].data.is_dead;
    int p2_dead = players[1].data.is_dead;

    // 둘 다 죽어야 게임 종료
    if (p1_dead && p2_dead) {
        if (players[0].data.score > players[1].data.score) return 0;
        if (players[1].data.score > players[0].data.score) return 1;
        return 2; // 무승부
    }

    // 시간 초과 종료
    if (remaining_time <= 0) {
        if (players[0].data.score > players[1].data.score) return 0; 
        if (players[1].data.score > players[0].data.score) return 1; 
        return 3; // 타임아웃
    }
    
    return -1; // 아직 진행 중 (한 명만 죽은 경우 포함)
}

void broadcast_game_state() {
    GameState state;
    state.remaining_time = remaining_time;
    state.winner_id = get_winner_id();
    state.map_width = game_map_width;
    state.map_height = game_map_height;
    state.start_x = box_start_x;
    state.start_y = box_start_y;

    for(int i=0; i<PLAYER_COUNT; i++) state.players[i] = players[i].data;
    for(int i=0; i<FOOD_COUNT; i++) state.foods[i] = foods[i];

    for(int i=0; i<PLAYER_COUNT; i++) {
        if (players[i].client_fd != 0) {
            send(players[i].client_fd, &state, sizeof(GameState), 0);
        }
    }
}

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    int player_id = -1;
    char input_dir;
    
    pthread_mutex_lock(&game_mutex);
    for(int i=0; i<PLAYER_COUNT; i++) {
        if(players[i].client_fd == 0) {
            players[i].client_fd = client_fd;
            player_id = i;
            send(client_fd, &player_id, sizeof(int), 0);
            break;
        }
    }
    pthread_mutex_unlock(&game_mutex);
    
    if (player_id == -1) { close(client_fd); return NULL; }

    while (recv(client_fd, &input_dir, 1, 0) > 0) {
        pthread_mutex_lock(&game_mutex);
        int winner = get_winner_id();
        
        if (winner == -1) { 
            // 내가 살아있을 때만 이동
            if (!players[player_id].data.is_dead) {
                if (input_dir == 'U' && players[player_id].dir_y == 0) { players[player_id].dir_x = 0; players[player_id].dir_y = -1; }
                else if (input_dir == 'D' && players[player_id].dir_y == 0) { players[player_id].dir_x = 0; players[player_id].dir_y = 1; }
                else if (input_dir == 'L' && players[player_id].dir_x == 0) { players[player_id].dir_x = -1; players[player_id].dir_y = 0; }
                else if (input_dir == 'R' && players[player_id].dir_x == 0) { players[player_id].dir_x = 1; players[player_id].dir_y = 0; }
                else if (input_dir == 'Q') { players[player_id].data.is_dead = 1; }
            }
        } else { 
            // 게임 종료 시 재시작
            if (input_dir == 'r' || input_dir == 'R') {
                init_game(); 
            }
        }
        pthread_mutex_unlock(&game_mutex);
    }

    pthread_mutex_lock(&game_mutex);
    players[player_id].client_fd = 0;
    players[player_id].data.is_dead = 1;
    pthread_mutex_unlock(&game_mutex);
    close(client_fd);
    return NULL;
}

int main() {
    init_game();
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) exit(EXIT_FAILURE);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, MAX_CLIENTS);

    printf("Waiting for clients on port %d...\n", PORT);
    
    for(int i=0; i<PLAYER_COUNT; i++) {
        while (1) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                if (errno == EINTR) continue;
                exit(EXIT_FAILURE);
            }
            break; 
        }
        pthread_t client_thread;
        pthread_create(&client_thread, NULL, handle_client, &new_socket);
        usleep(1000);
    }
    
    printf("Game Start!\n");
    setup_timers();
    
    while (1) {
        if (game_tick_flag) {
            pthread_mutex_lock(&game_mutex);
            if (get_winner_id() == -1) {
                update_game_logic();
            }
            broadcast_game_state();
            pthread_mutex_unlock(&game_mutex);
            game_tick_flag = 0;
        }
        usleep(1000); 
    }

    close(server_fd);
    return 0;
}
