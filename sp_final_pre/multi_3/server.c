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
#include <errno.h> // EINTR 처리를 위해 필요

// ==========================================
// 1. 통신 및 게임 상수 정의
// ==========================================
#define PORT 12345 // 충돌 방지를 위해 높은 포트 사용
#define MAX_CLIENTS 2
#define TIMEOUT_SEC 180
#define MAX_SNAKE_LEN 1024
#define FOOD_COUNT 5
#define PLAYER_COUNT 2
#define GAME_TICK_MS 100 // 100ms 마다 게임 로직 실행 (10 FPS)

// ==========================================
// 2. 데이터 구조체 정의 (클라이언트와 공유)
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

// 서버 로직용 뱀 구조체 (방향 정보 추가)
typedef struct Snake {
    SnakeData data;
    int dir_x, dir_y;
    int client_fd; // 해당 뱀을 조종하는 클라이언트의 소켓 파일 디스크립터
} Snake;

// 서버가 클라이언트에게 전송할 전체 게임 상태
typedef struct GameState {
    SnakeData players[PLAYER_COUNT];
    Point foods[FOOD_COUNT];
    int remaining_time;
    int winner_id;
    int map_width, map_height;
    int start_x, start_y;
} GameState;

// ==========================================
// 3. 전역 변수 및 뮤텍스
// ==========================================
volatile sig_atomic_t remaining_time = TIMEOUT_SEC;
volatile sig_atomic_t is_time_over = 0;
volatile sig_atomic_t game_tick_flag = 0;

Snake players[PLAYER_COUNT];
Point foods[FOOD_COUNT];
int game_map_width = 60;
int game_map_height = 25;
int box_start_x = 0;
int box_start_y = 0;

pthread_mutex_t game_mutex = PTHREAD_MUTEX_INITIALIZER;

// ==========================================
// 4. 타이머 및 시그널 관련 함수
// ==========================================
void timer_handler(int sig, siginfo_t *si, void *uc) {
    if (sig == SIGUSR1) {
        if (remaining_time > 0) remaining_time--;
    } else if (sig == SIGUSR2) {
        is_time_over = 1;
    } else if (sig == SIGALRM) {
        game_tick_flag = 1;
    }
}

void setup_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO; 
    sa.sa_sigaction = timer_handler;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
}

void make_timer(timer_t *timerID, int signo, int sec, int interval_sec) {
    struct sigevent sev;
    struct itimerspec its;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = signo;
    sev.sigev_value.sival_ptr = timerID;

    if (timer_create(CLOCK_REALTIME, &sev, timerID) == -1) {
        perror("timer_create");
        exit(1);
    }

    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = interval_sec;
    its.it_interval.tv_nsec = 0;

    if (timer_settime(*timerID, 0, &its, NULL) == -1) {
        perror("timer_settime");
        exit(1);
    }
}

void setup_game_tick_timer() {
    struct itimerval it_val;
    it_val.it_value.tv_sec = 0;
    it_val.it_value.tv_usec = GAME_TICK_MS * 1000;
    it_val.it_interval.tv_sec = 0;
    it_val.it_interval.tv_usec = GAME_TICK_MS * 1000;
    
    if (setitimer(ITIMER_REAL, &it_val, NULL) == -1) {
        perror("setitimer");
        exit(1);
    }
}

// ==========================================
// 5. 게임 로직 함수
// ==========================================
void spawn_food(Point *target_food) {
    // 뱀이나 다른 먹이와 겹치지 않는 곳에 새 먹이를 생성
    // ... (이전 코드와 동일)
    while(1) {
        int fx = (rand() % (game_map_width - 2)) + box_start_x + 1;
        int fy = (rand() % (game_map_height - 2)) + box_start_y + 1;
        
        int overlap = 0;
        for(int p=0; p<PLAYER_COUNT; p++) {
            if (!players[p].data.is_dead) {
                for(int i=0; i<players[p].data.length; i++) {
                    if(players[p].data.body[i].x == fx && players[p].data.body[i].y == fy) overlap = 1;
                }
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
    srand(time(NULL));
    remaining_time = TIMEOUT_SEC;
    is_time_over = 0;

    // P1 초기화 (왼쪽)
    players[0].data.length = 5;
    players[0].data.score = 0;
    players[0].dir_x = 1; players[0].dir_y = 0;
    players[0].data.is_dead = 0;
    players[0].client_fd = 0; // 초기 소켓은 0
    for (int i = 0; i < players[0].data.length; i++) {
        players[0].data.body[i].x = box_start_x + 5 - i;
        players[0].data.body[i].y = box_start_y + game_map_height / 2;
    }

    // P2 초기화 (오른쪽)
    players[1].data.length = 5;
    players[1].data.score = 0;
    players[1].dir_x = -1; players[1].dir_y = 0;
    players[1].data.is_dead = 0;
    players[1].client_fd = 0; // 초기 소켓은 0
    for (int i = 0; i < players[1].data.length; i++) {
        players[1].data.body[i].x = box_start_x + game_map_width - 6 + i;
        players[1].data.body[i].y = box_start_y + game_map_height / 2;
    }

    // 먹이 초기화
    for(int i=0; i<FOOD_COUNT; i++) {
        spawn_food(&foods[i]);
    }
}

void update_game_logic() {
    // 뱀 이동, 충돌 검사 (벽, 자신, 상대), 먹이 섭취 처리
    // ... (이전 코드와 동일한 로직)

    int i, p, q;

    // 1. 뱀 이동
    for (p = 0; p < PLAYER_COUNT; p++) {
        if (players[p].data.is_dead) continue;

        Point tail = players[p].data.body[players[p].data.length - 1]; 
        for (i = players[p].data.length - 1; i > 0; i--) {
            players[p].data.body[i] = players[p].data.body[i - 1];
        }
        players[p].data.body[0].x += players[p].dir_x;
        players[p].data.body[0].y += players[p].dir_y;

        // 2. 충돌 검사: 벽
        if (players[p].data.body[0].x <= box_start_x || players[p].data.body[0].x >= box_start_x + game_map_width - 1 ||
            players[p].data.body[0].y <= box_start_y || players[p].data.body[0].y >= box_start_y + game_map_height - 1) {
            players[p].data.is_dead = 1;
        }

        // 3. 충돌 검사: 자기 몸통
        for (i = 1; i < players[p].data.length; i++) {
            if (players[p].data.body[0].x == players[p].data.body[i].x && players[p].data.body[0].y == players[p].data.body[i].y) {
                players[p].data.is_dead = 1;
            }
        }
        
        // 4. 충돌 검사: 상대방 몸통 및 머리
        q = (p + 1) % PLAYER_COUNT;
        if (!players[q].data.is_dead) {
            for (i = 0; i < players[q].data.length; i++) {
                if (players[p].data.body[0].x == players[q].data.body[i].x && players[p].data.body[0].y == players[q].data.body[i].y) {
                    players[p].data.is_dead = 1;
                    break;
                }
            }
        }

        // 5. 먹이 섭취
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

int get_winner_id() {
    int p1_dead = players[0].data.is_dead;
    int p2_dead = players[1].data.is_dead;

    if (p1_dead && p2_dead) return 2; // 무승부 (Double KO)
    if (p1_dead) return 1; // P2 승
    if (p2_dead) return 0; // P1 승
    
    if (remaining_time <= 0) {
        if (players[0].data.score > players[1].data.score) return 0; // P1 승
        if (players[1].data.score > players[0].data.score) return 1; // P2 승
        return 2; // 무승부 (점수 같음)
    }

    return -1; // 게임 진행 중
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
            // 연결된 모든 클라이언트에게 상태 전송
            send(players[i].client_fd, &state, sizeof(GameState), 0);
        }
    }
}

// ==========================================
// 6. 네트워크 및 메인 함수
// ==========================================

void *handle_client(void *arg) {
    int client_fd = *((int *)arg);
    int player_id = -1;
    char input_dir;
    
    // 뱀 ID 할당
    for(int i=0; i<PLAYER_COUNT; i++) {
        if(players[i].client_fd == 0) {
            players[i].client_fd = client_fd;
            player_id = i;
            if (send(client_fd, &player_id, sizeof(int), 0) <= 0) break;
            printf("Player %d connected. FD: %d\n", player_id + 1, client_fd);
            break;
        }
    }
    
    if (player_id == -1) {
        printf("Maximum clients reached.\n");
        close(client_fd);
        return NULL;
    }

    // 클라이언트 입력(방향키) 수신 루프
    while (recv(client_fd, &input_dir, 1, 0) > 0) {
        pthread_mutex_lock(&game_mutex);
        if (!players[player_id].data.is_dead) {
            // 방향키 입력 처리
            if (input_dir == 'U' && players[player_id].dir_y == 0) { players[player_id].dir_x = 0; players[player_id].dir_y = -1; }
            else if (input_dir == 'D' && players[player_id].dir_y == 0) { players[player_id].dir_x = 0; players[player_id].dir_y = 1; }
            else if (input_dir == 'L' && players[player_id].dir_x == 0) { players[player_id].dir_x = -1; players[player_id].dir_y = 0; }
            else if (input_dir == 'R' && players[player_id].dir_x == 0) { players[player_id].dir_x = 1; players[player_id].dir_y = 0; }
            else if (input_dir == 'Q') { players[player_id].data.is_dead = 1; }
        }
        pthread_mutex_unlock(&game_mutex);
    }

    // 연결 종료 처리 (클라이언트 연결 끊김)
    pthread_mutex_lock(&game_mutex);
    printf("Player %d disconnected.\n", player_id + 1);
    players[player_id].client_fd = 0;
    players[player_id].data.is_dead = 1;
    pthread_mutex_unlock(&game_mutex);

    close(client_fd);
    return NULL;
}


int main() {
    setup_signal_handler();
    init_game();
    
    // 1초/3분 타이머는 미리 설정
    timer_t timer_tick, timer_limit;
    make_timer(&timer_tick, SIGUSR1, 1, 1);
    make_timer(&timer_limit, SIGUSR2, TIMEOUT_SEC, 0);
    // 게임 틱 타이머는 접속 완료 후 시작

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Snake Server listening on port %d...\n", PORT);
    printf("Waiting for %d clients to connect...\n", PLAYER_COUNT);
    
    // 2. 클라이언트 연결 대기 (EINTR 에러 처리 루프)
    for(int i=0; i<PLAYER_COUNT; i++) {
        while (1) {
            new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
            if (new_socket < 0) {
                if (errno == EINTR) { // 시그널 때문에 끊긴 경우
                    continue; 
                }
                perror("accept");
                exit(EXIT_FAILURE);
            }
            break; 
        }

        pthread_t client_thread;
        // 클라이언트 핸들링 스레드 생성
        if (pthread_create(&client_thread, NULL, handle_client, &new_socket) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
        // 새로운 클라이언트 FD를 스레드가 복사할 시간을 위해 짧게 대기
        usleep(1000); 
    }
    
    printf("2 clients connected. Game starting!\n");

    // 3. 2명 접속 완료 후 게임 틱 타이머 시작
    setup_game_tick_timer(); 

    // 4. 메인 게임 루프
    while (get_winner_id() == -1 && !is_time_over) {
        if (game_tick_flag) {
            pthread_mutex_lock(&game_mutex);
            update_game_logic();
            broadcast_game_state();
            pthread_mutex_unlock(&game_mutex);
            game_tick_flag = 0;
        }
        usleep(1000); 
    }

    // 5. 게임 종료 처리
    printf("Game Over! Winner ID: %d\n", get_winner_id());
    broadcast_game_state();
    timer_delete(timer_tick);
    timer_delete(timer_limit);
    close(server_fd);

    return 0;
}
