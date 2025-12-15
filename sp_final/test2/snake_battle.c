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
#define MSG_RESTART 4    // 재시작 강제 명령
#define MSG_QUIT_APP 5   // 종료 강제 명령

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
volatile int my_delay = 100;
volatile int shrink_level = 0;
volatile int opponent_ended = 0;
volatile int opponent_final_score = -1;

// 상대방의 의사를 저장하는 변수
volatile int force_restart = 0;
volatile int force_quit = 0;

int sock = -1; 

// ==========================================
// 2. 네트워크 및 타이머 함수
// ==========================================

void send_packet(int type, int payload) {
    if (sock == -1) return;
    Packet p = {type, payload};
    send(sock, &p, sizeof(p), 0);
}

void* network_receiver(void* arg) {
    Packet p;
    while (recv(sock, &p, sizeof(p), 0) > 0) {
        if (p.type == MSG_SCORE) {
            int prev_score = opponent_score;
            opponent_score = p.payload;
            if (prev_score / 100 < opponent_score / 100) {
                shrink_level++;
            }
        } 
        else if (p.type == MSG_SPEED_ATTACK) {
            if (my_delay > 25) my_delay /= 2; 
        }
        else if (p.type == MSG_GAME_OVER) {
            opponent_ended = 1;
            opponent_final_score = p.payload;
        }
        else if (p.type == MSG_RESTART) {
            force_restart = 1; // 상대방이 재시작을 눌렀음
        }
        else if (p.type == MSG_QUIT_APP) {
            force_quit = 1;    // 상대방이 종료를 눌렀음
        }
    }
    return NULL;
}

void handler(int sig, siginfo_t *si, void *uc) {
    if (sig == SIGUSR1) {
        if (remaining_time > 0) remaining_time--;
    } else if (sig == SIGUSR2) {
        is_time_over = 1;
    }
}

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

void draw_ui(int score, game_size_t size) {
    mvprintw(size.start_y - 3, size.start_x, "MY SCORE: %d", score);
    mvprintw(size.start_y - 3, size.start_x + 20, "TIME: %03d sec", remaining_time);
    
    mvprintw(size.start_y - 2, size.start_x, "OPP SCORE: %d", opponent_score);
    
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
    
    time_t eat_history[5] = {0, 0, 0, 0, 0};
    int eat_idx = 0;

    remaining_time = TIMEOUT_SEC;
    is_time_over = 0;
    shrink_level = 0;
    my_delay = 100;
    
    send_packet(MSG_SCORE, 0); 

    game_size_t current_size = init_size;

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    srand(time(NULL));
    init_game_objects(snake, &snake_len, &food, current_size);

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
        // --- 동적 영역 축소 및 UI 갱신 로직 (생략 없이 동일하게 유지) ---
        int safe_shrink = (shrink_level > 5) ? 5 : shrink_level;
        int target_w = init_size.box_width * (10 - safe_shrink) / 10;
        int target_h = init_size.box_height * (10 - safe_shrink) / 10;
        
        if (target_w != current_size.box_width || target_h != current_size.box_height) {
            current_size.box_width = target_w;
            current_size.box_height = target_h;
            current_size.start_x = (init_size.term_width - current_size.box_width) / 2;
            current_size.start_y = (init_size.term_height - current_size.box_height) / 2;
            
            if (food.x < current_size.start_x || food.x >= current_size.start_x + current_size.box_width ||
                food.y < current_size.start_y || food.y >= current_size.start_y + current_size.box_height) {
                 food.x = (rand() % (current_size.box_width - 2)) + current_size.start_x + 1;
                 food.y = (rand() % (current_size.box_height - 2)) + current_size.start_y + 1;
            }
        }

        ch = getch();
        if (ch != ERR) {
            if (ch == 'q') { is_time_over = 1; break; } // 게임 중 q는 자살(Time over) 처리
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

        Point tail = snake[snake_len - 1];
        for (int i = snake_len - 1; i > 0; i--) snake[i] = snake[i - 1];
        snake[0].x += dir_x;
        snake[0].y += dir_y;

        if (snake[0].x <= current_size.start_x || snake[0].x >= current_size.start_x + current_size.box_width - 1 ||
            snake[0].y <= current_size.start_y || snake[0].y >= current_size.start_y + current_size.box_height - 1) {
            break; 
        }

        for (int i = 1; i < snake_len; i++) {
            if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
                is_time_over = 1; goto game_end;
            }
        }

        if (snake[0].x == food.x && snake[0].y == food.y) {
            score += 10;
            send_packet(MSG_SCORE, score);

            time_t now = time(NULL);
            eat_history[eat_idx] = now;
            eat_idx = (eat_idx + 1) % 5;
            int oldest_idx = eat_idx; 
            if (eat_history[oldest_idx] != 0) {
                double diff = difftime(now, eat_history[oldest_idx]);
                if (diff <= 60.0) {
                    send_packet(MSG_SPEED_ATTACK, 1);
                    for(int k=0; k<5; k++) eat_history[k] = 0;
                    mvprintw(current_size.start_y - 1, current_size.start_x + 10, "COMBO ATTACK!!");
                }
            }

            if (snake_len < MAX_SNAKE_LEN) {
                snake[snake_len] = tail;
                snake_len++;
            }
            while(1) {
                food.x = (rand() % (current_size.box_width - 2)) + current_size.start_x + 1;
                food.y = (rand() % (current_size.box_height - 2)) + current_size.start_y + 1;
                int overlap = 0;
                for(int i=0; i<snake_len; i++) if(snake[i].x == food.x && snake[i].y == food.y) overlap = 1;
                if(!overlap) break;
            }
        }

        clear();
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
        napms(my_delay);

        if (is_time_over) break;
    }

game_end:
    timer_delete(timer_tick);
    timer_delete(timer_limit);
    send_packet(MSG_GAME_OVER, score);

    // ===============================================
    // ★ 게임 종료 및 결과 대기 화면
    // ===============================================
    nodelay(stdscr, TRUE); // 입력 대기를 비동기로 설정 (중요)
    
    int final_ret = 0;
    
    while(1) {
        clear();
        mvprintw(init_size.term_height/2 - 4, init_size.term_width/2 - 10, "== GAME OVER ==");
        mvprintw(init_size.term_height/2 - 2, init_size.term_width/2 - 10, "My Score: %d", score);
        
        // 상대방 점수 표시
        if (opponent_ended) {
            mvprintw(init_size.term_height/2, init_size.term_width/2 - 10, "Opponent Score: %d", opponent_final_score);
            if (score > opponent_final_score) 
                mvprintw(init_size.term_height/2 + 1, init_size.term_width/2 - 10, "RESULT: YOU WIN!");
            else if (score < opponent_final_score)
                mvprintw(init_size.term_height/2 + 1, init_size.term_width/2 - 10, "RESULT: YOU LOSE!");
            else
                mvprintw(init_size.term_height/2 + 1, init_size.term_width/2 - 10, "RESULT: DRAW!");
        } else {
             mvprintw(init_size.term_height/2, init_size.term_width/2 - 10, "Waiting for opponent to finish...");
        }

        // 메뉴 표시 (두 플레이어 모두 종료된 후 활성화)
        if (opponent_ended) {
            attron(A_BOLD);
            mvprintw(init_size.term_height/2 + 3, init_size.term_width/2 - 20, "Press 'r' to Restart(ALL), 'q' to Quit(ALL)");
            attroff(A_BOLD);
        }

        refresh();

        // 1. 내 입력 확인 (능동적 결정)
        // 상대방이 아직 끝나지 않았어도, 내가 강제로 전체 종료/재시작을 보낼 수도 있음 (기획에 따라 다름)
        // 여기서는 "두 플레이어 모두 종료됐을 때" 단위를 원했으므로, 상대가 끝난 뒤 입력을 받음.
        if (opponent_ended) {
            int cmd = getch();
            if (cmd == 'r' || cmd == 'R') {
                send_packet(MSG_RESTART, 1); // 상대에게 재시작 명령
                final_ret = 1;
                break;
            } 
            else if (cmd == 'q' || cmd == 'Q') {
                send_packet(MSG_QUIT_APP, 1); // 상대에게 종료 명령
                final_ret = 0;
                break;
            }
        } else {
            // 입력 버퍼 비우기 (상대방 끝날 때까지 대기)
            getch();
        }

        // 2. 상대방 명령 확인 (수동적 수용)
        if (force_restart) {
            mvprintw(init_size.term_height/2 + 5, init_size.term_width/2 - 15, "Opponent forced restart!");
            refresh();
            napms(1000); // 메시지 확인 시간
            final_ret = 1;
            break;
        }
        if (force_quit) {
            mvprintw(init_size.term_height/2 + 5, init_size.term_width/2 - 15, "Opponent forced quit!");
            refresh();
            napms(1000);
            final_ret = 0;
            break;
        }

        napms(100); // CPU 사용량 조절
    }

    endwin();
    return final_ret;
}

// ==========================================
// 4. 메인 함수
// ==========================================
int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;

    if (argc < 2) {
        printf("Usage:\n  Server: %s <port>\n  Client: %s <server_ip> <port>\n", argv[0], argv[0]);
        return 1;
    }

    if (argc == 2) { 
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
        close(server_fd);
    } else { 
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
        // 매 판마다 플래그 초기화
        opponent_ended = 0;
        opponent_final_score = -1;
        force_restart = 0;
        force_quit = 0;

        sleep(1); 
        int restart = run_game(gameSize);
        if (!restart) break; // 0 반환(종료) 시 루프 탈출
    }

    close(sock);
    return 0;
}
