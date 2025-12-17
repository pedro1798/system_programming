#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE  // usleep 사용을 위해 추가

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include <time.h>

// =============================================================
// [공통 정의] 헤더 파일 내용 통합
// =============================================================
#define PORT 9999
#define INITIAL_WIDTH 40
#define INITIAL_HEIGHT 20
#define MAX_SNAKE_LEN 100
#define TIMEOUT_SEC 180 // 3분

// 키 정의 (ncurses 상수와 충돌 방지를 위해 CMD_ 접두어 사용)
#define KEY_UP_CMD 'U'
#define KEY_DOWN_CMD 'D'
#define KEY_LEFT_CMD 'L'
#define KEY_RIGHT_CMD 'R'
#define KEY_PAUSE_CMD 'S'
#define CMD_RESTART 'Y'  // 이름 변경 (KEY_RESTART 충돌 회피)
#define CMD_QUIT 'N'     // 이름 변경

typedef struct {
    int x, y;
} Point;

typedef struct {
    int id;                 // 플레이어 ID (0 or 1)
    Point body[MAX_SNAKE_LEN];
    int length;
    int dir_x, dir_y;
    int score;
    int map_width;          // 동적 맵 크기
    int map_height;
    int speed_ms;           // 현재 속도 (ms)
    int is_paused;          // 일시정지
    int is_dead;            // 사망 여부
    Point food;             // 먹이 위치
    
    // 로직용 (클라이언트 전송용으로 포함되나 표시는 안함)
    long long food_timestamps[MAX_SNAKE_LEN]; // 먹이 먹은 시간(ms)
    int food_eat_idx;       // 인덱스
    int last_shrink_score;  // 마지막으로 맵을 축소시킨 점수 구간
} PlayerState;

typedef struct {
    PlayerState p[2];       // 플레이어 2명
    int remaining_time;     // 남은 시간
    int game_over;          // 종료 여부 (0:진행, 1:종료)
    int winner_id;          // 승자 ID (-1:무승부)
} GameState;

// =============================================================
// [서버] 전역 변수 및 함수
// =============================================================
int client_socks[2];
GameState state;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 현재 시간(ms) 반환
long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL);
    return te.tv_sec * 1000LL + te.tv_usec / 1000;
}

// 먹이 생성 (뱀 몸통과 겹치지 않게)
void spawn_food(int pid) {
    PlayerState *p = &state.p[pid];
    int valid = 0;
    while (!valid) {
        valid = 1;
        // 맵 크기가 줄어들 수 있으므로 현재 map_width/height 기준 생성
        // 벽(0, width-1) 제외
        if (p->map_width <= 2 || p->map_height <= 2) {
             // 맵이 너무 작아지면 생성 불가 (중앙 강제)
             p->food.x = 1; p->food.y = 1;
             break;
        }
        
        p->food.x = (rand() % (p->map_width - 2)) + 1;
        p->food.y = (rand() % (p->map_height - 2)) + 1;
        
        for (int i = 0; i < p->length; i++) {
            if (p->food.x == p->body[i].x && p->food.y == p->body[i].y) {
                valid = 0;
                break;
            }
        }
    }
}

// 플레이어 초기화
void init_player(int pid) {
    PlayerState *p = &state.p[pid];
    p->id = pid;
    p->map_width = INITIAL_WIDTH;
    p->map_height = INITIAL_HEIGHT;
    p->length = 3;
    p->score = 0;
    // P0: 오른쪽, P1: 왼쪽으로 시작
    p->dir_x = (pid == 0) ? 1 : -1; 
    p->dir_y = 0;
    p->speed_ms = 100; // 0.1초 = 100ms
    p->is_paused = 0;
    p->is_dead = 0;
    p->last_shrink_score = 0;
    p->food_eat_idx = 0;
    
    for(int i=0; i<MAX_SNAKE_LEN; i++) p->food_timestamps[i] = 0;

    int cx = p->map_width / 2;
    int cy = p->map_height / 2;
    for (int i = 0; i < 3; i++) {
        p->body[i].x = cx - (i * p->dir_x);
        p->body[i].y = cy;
    }
    spawn_food(pid);
}

void reset_game() {
    pthread_mutex_lock(&mutex);
    state.remaining_time = TIMEOUT_SEC;
    state.game_over = 0;
    state.winner_id = -1;
    init_player(0);
    init_player(1);
    printf("Game Reset!\n");
    pthread_mutex_unlock(&mutex);
}

// 게임 로직 업데이트 (이동, 충돌, 규칙 적용)
void update_game_logic(int pid) {
    PlayerState *me = &state.p[pid];
    PlayerState *opp = &state.p[1-pid];

    if (me->is_dead || me->is_paused || state.game_over) return;

    // 1. 몸통 이동
    Point tail = me->body[me->length - 1];
    for (int i = me->length - 1; i > 0; i--) {
        me->body[i] = me->body[i - 1];
    }
    // 2. 머리 이동
    me->body[0].x += me->dir_x;
    me->body[0].y += me->dir_y;

    // 3. 벽 충돌 (동적 맵 크기 기준)
    if (me->body[0].x <= 0 || me->body[0].x >= me->map_width - 1 ||
        me->body[0].y <= 0 || me->body[0].y >= me->map_height - 1) {
        me->is_dead = 1;
    }

    // 4. 자기 충돌
    for (int i = 1; i < me->length; i++) {
        if (me->body[0].x == me->body[i].x && me->body[0].y == me->body[i].y) {
            me->is_dead = 1;
        }
    }

    // 5. 먹이 섭취
    if (!me->is_dead && me->body[0].x == me->food.x && me->body[0].y == me->food.y) {
        me->score += 10;
        if (me->length < MAX_SNAKE_LEN) {
            me->body[me->length] = tail;
            me->length++;
        }
        
        long long now = current_timestamp();
        
        // [조건 4] 속도 공격: 1분(60000ms) 내 5개 섭취
        me->food_timestamps[me->food_eat_idx % MAX_SNAKE_LEN] = now;
        me->food_eat_idx++;
        
        if (me->food_eat_idx >= 5) {
            long long fifth_prev = me->food_timestamps[(me->food_eat_idx - 5) % MAX_SNAKE_LEN];
            if (now - fifth_prev <= 60000) { // 60초 이내
                if (opp->speed_ms > 50) { // 최대 속도 제한
                    opp->speed_ms /= 2; // 속도 2배 (딜레이 절반)
                    if (opp->speed_ms < 50) opp->speed_ms = 50;
                }
            }
        }

        // [조건 2] 영역 공격: 100점 단위로 상대 맵 10% 축소
        if (me->score >= me->last_shrink_score + 100) {
            me->last_shrink_score = (me->score / 100) * 100;
            
            opp->map_width = (int)(opp->map_width * 0.9);
            opp->map_height = (int)(opp->map_height * 0.9);
            
            // 최소 크기 제한
            if (opp->map_width < 10) opp->map_width = 10;
            if (opp->map_height < 6) opp->map_height = 6;
        }

        spawn_food(pid);
    }
}

// 클라이언트 입력 처리
void *handle_client(void *arg) {
    int pid = *((int *)arg);
    free(arg);
    int sock = client_socks[pid];
    char buffer[1];

    while (recv(sock, buffer, 1, 0) > 0) {
        pthread_mutex_lock(&mutex);
        
        if (state.game_over) {
            // 게임 종료 시 재시작 의사 확인
            if (buffer[0] == CMD_RESTART) reset_game();
        } else {
            PlayerState *p = &state.p[pid];
            if (buffer[0] == KEY_PAUSE_CMD) {
                p->is_paused = !p->is_paused;
            } else if (!p->is_paused) {
                // 반대 방향 입력 방지
                if (buffer[0] == KEY_UP_CMD && p->dir_y == 0) { p->dir_x = 0; p->dir_y = -1; }
                else if (buffer[0] == KEY_DOWN_CMD && p->dir_y == 0) { p->dir_x = 0; p->dir_y = 1; }
                else if (buffer[0] == KEY_LEFT_CMD && p->dir_x == 0) { p->dir_x = -1; p->dir_y = 0; }
                else if (buffer[0] == KEY_RIGHT_CMD && p->dir_x == 0) { p->dir_x = 1; p->dir_y = 0; }
            }
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main() {
    srand(time(NULL));
    int server_sock;
    struct sockaddr_in address;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_sock, (struct sockaddr *)&address, sizeof(address));
    listen(server_sock, 2);

    printf("Snake Server Started on Port %d\nWait for 2 players...\n", PORT);

    // 2명 접속 대기
    for (int i = 0; i < 2; i++) {
        client_socks[i] = accept(server_sock, NULL, NULL);
        printf("Player %d connected.\n", i + 1);
        
        int *pid = malloc(sizeof(int));
        *pid = i;
        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, pid);
    }

    reset_game();

    long long last_tick_p0 = current_timestamp();
    long long last_tick_p1 = current_timestamp();
    long long last_sec_check = current_timestamp();

    // 메인 루프
    while (1) {
        long long now = current_timestamp();

        pthread_mutex_lock(&mutex);

        // 1초마다 시간 감소
        if (!state.game_over && now - last_sec_check >= 1000) {
            state.remaining_time--;
            last_sec_check = now;
            
            // [조건 5] 3분 초과 시 즉시 종료
            if (state.remaining_time <= 0) {
                state.game_over = 1;
                // 점수 승부
                if (state.p[0].score > state.p[1].score) state.winner_id = 0;
                else if (state.p[1].score > state.p[0].score) state.winner_id = 1;
                else state.winner_id = -1;
            }
        }

        if (!state.game_over) {
            // Player 0 로직 (속도에 따라)
            if (now - last_tick_p0 >= state.p[0].speed_ms) {
                update_game_logic(0);
                last_tick_p0 = now;
            }
            // Player 1 로직
            if (now - last_tick_p1 >= state.p[1].speed_ms) {
                update_game_logic(1);
                last_tick_p1 = now;
            }

            // 사망 처리
            if (state.p[0].is_dead || state.p[1].is_dead) {
                state.game_over = 1;
                if (state.p[0].is_dead && state.p[1].is_dead) state.winner_id = -1; // 둘 다 죽음
                else if (state.p[0].is_dead) state.winner_id = 1; // P0사망 -> P1 승
                else state.winner_id = 0;
            }
        }

        // 클라이언트에게 상태 전송
        for (int i = 0; i < 2; i++) {
            send(client_socks[i], &state, sizeof(GameState), 0);
        }

        pthread_mutex_unlock(&mutex);
        usleep(10000); // 10ms tick
    }

    close(server_sock);
    return 0;
}
