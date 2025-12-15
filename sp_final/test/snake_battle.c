#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <math.h>

// ==========================================
// 1. 데이터 구조체 및 상수 정의
// ==========================================
#define TIMEOUT_SEC 180  // 3분 제한
#define MAX_SNAKE_LEN 1024
#define PORT 9999        // 게임 통신 포트

// 패킷 타입 정의
#define MSG_SCORE 1
#define MSG_SPEED_ATTACK 2
#define MSG_GAME_OVER 3

typedef struct {
    int type;
    int payload;
} Packet;

typedef struct GameSize {
    int box_height, box_width;
    int start_x, start_y;
    int term_height, term_width;
} game_size_t;

typedef struct Point {
    int x, y;
} Point;

// 전역 변수
volatile sig_atomic_t remaining_time = TIMEOUT_SEC;
volatile sig_atomic_t is_time_over = 0;
volatile int opponent_score = 0;
volatile int my_delay = 100; // 기본 속도 0.1초 (100ms)
volatile int shrink_level = 0; // 영역 축소 단계
volatile int opponent_ended = 0;
volatile int opponent_final_score = -1;

int sock = -1; // 소켓 디스크립터

// ==========================================
// 2. 네트워크 및 타이머 함수
// ==========================================

// 데이터 전송 도우미
void send_packet(int type, int payload) {
    if (sock == -1) return;
    Packet p = {type, payload};
    send(sock, &p, sizeof(p), 0);
}

// 네트워크 수신 스레드
void* network_receiver(void* arg) {
    Packet p;
    while (recv(sock, &p, sizeof(p), 0) > 0) {
        if (p.type == MSG_SCORE) {
            int prev_score = opponent_score;
            opponent_score = p.payload;
            
            // 상대방 점수가 100점 단위로 오를 때마다 내 영역 축소
            // (예: 90 -> 100 될 때 트리거)
            if (prev_score / 100 < opponent_score / 100) {
                shrink_level++;
            }
        } 
        else if (p.type == MSG_SPEED_ATTACK) {
            // 속도 2배 (delay 절반)
            if (my_delay > 25) my_delay /= 2; 
        }
        else if (p.type == MSG_GAME_OVER) {
            opponent_ended = 1;
            opponent_final_score = p.payload;
        }
    }
    return NULL;
}

// 시그널 핸들러
void handler(int sig, siginfo_t *si, void *uc) {
    if (sig == SIGUSR1) {
        if (remaining_time > 0) remaining_time--;
    } else if (sig == SIGUSR2) {
        is_time_over = 1;
    }
}

// 타이머 생성
void make_timer(timer_t *timerID, int signo, int sec, int interval_sec) {
    struct sigevent sev;
    struct itimerspec its;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = signo;
    sev.sigev_value.sival_ptr = timerID;

    if (timer_create(CLOCK_REALTIME, &sev, timerID) == -1) {
        perror("timer_create"); exit(1);
    }

    its.it_value.tv_sec = sec;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = interval_sec;
    its.it_interval.tv_nsec = 0;

    timer_settime(*timerID, 0, &its, NULL);
}

// ==========================================
// 3. 게임 로직 함수
// ==========================================

void init_game_objects(Point snake[], int *length, Point *food, game_size_t size) {
    *length = 3;
    int cx = size.start_x + size.box_width / 2;
    int cy = size.start_y + size.box_height / 2;
    
    for(int i=0; i<*length; i++) {
        snake[i].x = cx - i;
        snake[i].y = cy;
    }

    food->x = (rand() % (size.box_width - 2)) + size.start_x + 1;
    food->y = (rand() % (size.box_height - 2)) + size.start_y + 1;
}

// UI 그리기 (상대 점수 포함)
void draw_ui(int score, game_size_t size) {
    mvprintw(size.start_y - 3, size.start_x, "MY SCORE: %d", score);
    mvprintw(size.start_y - 3, size.start_x + 20, "TIME: %03d sec", remaining_time);
    
    // 대전 상대 점수 표시 (요구사항)
    mvprintw(size.start_y - 2, size.start_x, "OPP SCORE: %d", opponent_score);
    
    // 상태 메시지
    if (my_delay < 100) attron(A_BOLD | A_BLINK);
    mvprintw(size.start_y - 2, size.start_x + 20, "SPEED: %s", (my_delay < 100) ? "X2 FAST!" : "NORMAL");
    if (my_delay < 100) attroff(A_BOLD | A_BLINK);
}

int run_game(game_size_t init_size) {
    // 1. 초기화
    Point snake[MAX_SNAKE_LEN];
    int snake_len;
    Point food;
    int score = 0;
    int dir_x = 1, dir_y = 0;
    int ch;
    int paused = 0;
    
    // 콤보 체크용 (최근 5개 먹은 시간)
    time_t eat_history[5] = {0, 0, 0, 0, 0};
    int eat_idx = 0;

    // 상태 초기화
    remaining_time = TIMEOUT_SEC;
    is_time_over = 0;
    shrink_level = 0;
    my_delay = 100; // 0.1초
    send_packet(MSG_SCORE, 0); // 점수 리셋 알림

    game_size_t current_size = init_size;

    // Ncurses 설정
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    srand(time(NULL));
    init_game_objects(snake, &snake_len, &food, current_size);

    // 타이머 설정
    timer_t timer_tick, timer_limit;
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    make_timer(&timer_tick, SIGUSR1, 1, 1);
    make_timer(&timer_limit, SIGUSR2, TIMEOUT_SEC, 0);

    // 3. 게임 루프
    while (!is_time_over) {
        // --- 동적 영역 축소 로직 ---
        // shrink_level 1당 10% 감소. 최대 50%까지만 감소 제한 (너무 작아지면 불가능하므로)
        int safe_shrink = (shrink_level > 5) ? 5 : shrink_level;
        int target_w = init_size.box_width * (10 - safe_shrink) / 10;
        int target_h = init_size.box_height * (10 - safe_shrink) / 10;
        
        // 크기가 변했다면 중앙 정렬 다시 계산
        if (target_w != current_size.box_width || target_h != current_size.box_height) {
            current_size.box_width = target_w;
            current_size.box_height = target_h;
            current_size.start_x = (init_size.term_width - current_size.box_width) / 2;
            current_size.start_y = (init_size.term_height - current_size.box_height) / 2;
            
            // 먹이가 바뀐 영역 밖에 있으면 안으로 이동
            if (food.x < current_size.start_x || food.x >= current_size.start_x + current_size.box_width ||
                food.y < current_size.start_y || food.y >= current_size.start_y + current_size.box_height) {
                 food.x = (rand() % (current_size.box_width - 2)) + current_size.start_x + 1;
                 food.y = (rand() % (current_size.box_height - 2)) + current_size.start_y + 1;
            }
        }

        // 입력 처리
        ch = getch();
        if (ch != ERR) {
            if (ch == 'q') { is_time_over = 1; break; }
            else if (ch == 's') {
                paused = !paused;
                mvprintw(current_size.start_y - 1, current_size.start_x, paused ? "== PAUSED ==" : "            ");
            }
            else if (!paused) {
                if (ch == KEY_UP && dir_y == 0) { dir_x = 0; dir_y = -1; }
                else if (ch == KEY_DOWN && dir_y == 0) { dir_x = 0; dir_y = 1; }
                else if (ch == KEY_LEFT && dir_x == 0) { dir_x = -1; dir_y = 0; }
                else if (ch == KEY_RIGHT && dir_x == 0) { dir_x = 1; dir_y = 0; }
            }
        }

        if (paused) {
            draw_ui(score, current_size);
            napms(100);
            continue;
        }

        // 뱀 이동 로직
        Point tail = snake[snake_len - 1];
        for (int i = snake_len - 1; i > 0; i--) snake[i] = snake[i - 1];
        snake[0].x += dir_x;
        snake[0].y += dir_y;

        // 충돌 검사 1: 벽 (변경된 current_size 기준)
        if (snake[0].x <= current_size.start_x || snake[0].x >= current_size.start_x + current_size.box_width - 1 ||
            snake[0].y <= current_size.start_y || snake[0].y >= current_size.start_y + current_size.box_height - 1) {
            break; // 사망
        }

        // 충돌 검사 2: 자기 자신
        for (int i = 1; i < snake_len; i++) {
            if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
                is_time_over = 1; goto game_end;
            }
        }

        // 먹이 섭취
        if (snake[0].x == food.x && snake[0].y == food.y) {
            score += 10;
            send_packet(MSG_SCORE, score); // 점수 전송

            // --- 콤보 로직 (속도 공격) ---
            time_t now = time(NULL);
            eat_history[eat_idx] = now;
            eat_idx = (eat_idx + 1) % 5;
            
            // 가장 오래된 기록(현재 인덱스 위치가 덮어쓰기 전 가장 오래된 값이었음, 
            // 하지만 순환 큐이므로, 현재 인덱스가 가리키는 곳이 가장 오래된 값임)
            // 5개를 모두 채웠는지 확인 (0이 아니어야 함)
            int oldest_idx = eat_idx; 
            if (eat_history[oldest_idx] != 0) {
                // 최근 5번째 먹은 시간과 현재 시간 차이가 60초 이내
                double diff = difftime(now, eat_history[oldest_idx]);
                if (diff <= 60.0) {
                    send_packet(MSG_SPEED_ATTACK, 1); // 상대에게 공격 전송
                    // 콤보 초기화 (연속 발동 방지)
                    for(int k=0; k<5; k++) eat_history[k] = 0;
                    mvprintw(current_size.start_y - 1, current_size.start_x + 10, "COMBO ATTACK!!");
                }
            }

            if (snake_len < MAX_SNAKE_LEN) {
                snake[snake_len] = tail;
                snake_len++;
            }
            // 새 먹이 생성
            while(1) {
                food.x = (rand() % (current_size.box_width - 2)) + current_size.start_x + 1;
                food.y = (rand() % (current_size.box_height - 2)) + current_size.start_y + 1;
                int overlap = 0;
                for(int i=0; i<snake_len; i++) if(snake[i].x == food.x && snake[i].y == food.y) overlap = 1;
                if(!overlap) break;
            }
        }

        // 렌더링
        clear();
        // 맵 그리기 (current_size 사용)
        for (int y = current_size.start_y; y < current_size.start_y + current_size.box_height; y++) {
            for (int x = current_size.start_x; x < current_size.start_x + current_size.box_width; x++) {
                if (y == current_size.start_y || y == current_size.start_y + current_size.box_height - 1 || 
                    x == current_size.start_x || x == current_size.start_x + current_size.box_width - 1) {
                    mvaddch(y, x, '#');
                }
            }
        }
        for (int i = 0; i < snake_len; i++) mvaddch(snake[i].y, snake[i].x, (i==0 ? 'O' : 'o'));
        mvaddch(food.y, food.x, '@');
        draw_ui(score, current_size);
        
        refresh();
        napms(my_delay); // 가변 속도

        if (is_time_over) break;
    }

game_end:
    timer_delete(timer_tick);
    timer_delete(timer_limit);
    send_packet(MSG_GAME_OVER, score); // 최종 점수 전송

    // 결과 화면 대기
    nodelay(stdscr, FALSE);
    clear();
    mvprintw(init_size.term_height/2 - 4, init_size.term_width/2 - 10, "GAME OVER");
    mvprintw(init_size.term_height/2 - 2, init_size.term_width/2 - 10, "My Score: %d", score);
    
    // 상대방 점수 기다리기 (간단한 동기화)
    mvprintw(init_size.term_height/2 - 1, init_size.term_width/2 - 10, "Waiting for opponent...");
    refresh();
    
    // 상대방이 끝날 때까지 대기 (혹은 이미 끝났으면 바로 표시)
    // 실제 게임에선 타임아웃 처리가 필요하지만 여기선 단순화
    int wait_cycles = 0;
    while(!opponent_ended && wait_cycles < 50) { // 약 5초 대기
        napms(100);
        wait_cycles++;
    }

    if (opponent_ended) {
        mvprintw(init_size.term_height/2, init_size.term_width/2 - 10, "Opponent Score: %d", opponent_final_score);
        if (score > opponent_final_score) 
            mvprintw(init_size.term_height/2 + 1, init_size.term_width/2 - 10, "RESULT: YOU WIN!");
        else if (score < opponent_final_score)
            mvprintw(init_size.term_height/2 + 1, init_size.term_width/2 - 10, "RESULT: YOU LOSE!");
        else
            mvprintw(init_size.term_height/2 + 1, init_size.term_width/2 - 10, "RESULT: DRAW!");
    } else {
        mvprintw(init_size.term_height/2, init_size.term_width/2 - 10, "Opponent disconnected.");
    }

    mvprintw(init_size.term_height/2 + 3, init_size.term_width/2 - 15, "Press 'r' to Restart, 'q' to Quit");
    refresh();

    // 입력 버퍼 비우기 및 입력 대기
    flushinp();
    int ret = 0;
    while(1) {
        int cmd = getch();
        if (cmd == 'r' || cmd == 'R') { ret = 1; break; }
        else if (cmd == 'q' || cmd == 'Q') { ret = 0; break; }
    }
    endwin();
    return ret;
}

// ==========================================
// 4. 메인 함수 (네트워크 연결 설정)
// ==========================================
int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    // 네트워크 초기화
    if (argc < 2) {
        printf("Usage:\n  Server: %s <port>\n  Client: %s <server_ip> <port>\n", argv[0], argv[0]);
        return 1;
    }

    if (argc == 2) { // 서버 모드
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(atoi(argv[1]));
        
        bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
        listen(server_fd, 1);
        printf("Waiting for opponent on port %s...\n", argv[1]);
        
        clilen = sizeof(cli_addr);
        sock = accept(server_fd, (struct sockaddr *)&cli_addr, &clilen);
        printf("Opponent connected!\n");
        close(server_fd); // 1:1이므로 리스닝 소켓 닫음
    } else { // 클라이언트 모드
        sock = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(atoi(argv[2]));
        inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);
        
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            printf("Connection Failed\n");
            return 1;
        }
        printf("Connected to server!\n");
    }

    // 수신 스레드 시작
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, network_receiver, NULL);

    game_size_t gameSize;
    initscr();
    getmaxyx(stdscr, gameSize.term_height, gameSize.term_width);
    endwin();

    gameSize.box_height = 20;
    gameSize.box_width = 40;
    gameSize.start_y = (gameSize.term_height - gameSize.box_height) / 2;
    gameSize.start_x = (gameSize.term_width - gameSize.box_width) / 2;

    while(1) {
        // 게임 시작 전 대기 (옵션)
        sleep(1); 
        int restart = run_game(gameSize);
        // 재시작 시 상태 초기화는 run_game 내부에서 처리됨
        if (!restart) break;
    }

    close(sock);
    return 0;
}
