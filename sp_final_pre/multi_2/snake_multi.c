#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>
#include <string.h>

// ==========================================
// 1. 데이터 구조체 및 상수 정의
// ==========================================
#define TIMEOUT_SEC 180  // 3분 제한
#define MAX_SNAKE_LEN 1024
#define FOOD_COUNT 5     // 맵에 존재하는 먹이 개수 (Agar.io 스타일)

typedef struct GameSize {
    int box_height, box_width;
    int start_x, start_y;
    int term_height, term_width;
} game_size_t;

typedef struct Point {
    int x, y;
} Point;

// 뱀 구조체 정의
typedef struct Snake {
    Point body[MAX_SNAKE_LEN];
    int length;
    int dir_x, dir_y;
    int score;
    int color_pair;
    char head_char;
    int is_dead;
} Snake;

// 전역 변수
volatile sig_atomic_t remaining_time = TIMEOUT_SEC;
volatile sig_atomic_t is_time_over = 0;

// ==========================================
// 2. 타이머 및 시그널 관련 함수 (기존 유지)
// ==========================================

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

// ==========================================
// 3. 게임 로직 함수
// ==========================================

// 뱀 초기화
void init_snake(Snake *s, int start_x, int start_y, int color, char head) {
    s->length = 5;
    s->score = 0;
    s->dir_x = 0; // 초기 정지 상태
    s->dir_y = 0;
    s->color_pair = color;
    s->head_char = head;
    s->is_dead = 0;

    for (int i = 0; i < s->length; i++) {
        s->body[i].x = start_x;
        s->body[i].y = start_y + i; // 세로로 배치
    }
}

// 먹이 생성 (뱀이나 다른 먹이와 겹치지 않게)
void spawn_food(Point *target_food, Snake *s1, Snake *s2, Point foods[], game_size_t size) {
    while(1) {
        int fx = (rand() % (size.box_width - 2)) + size.start_x + 1;
        int fy = (rand() % (size.box_height - 2)) + size.start_y + 1;
        
        int overlap = 0;
        // P1 몸통 체크
        for(int i=0; i<s1->length; i++) if(s1->body[i].x == fx && s1->body[i].y == fy) overlap = 1;
        // P2 몸통 체크
        for(int i=0; i<s2->length; i++) if(s2->body[i].x == fx && s2->body[i].y == fy) overlap = 1;
        // 다른 먹이 체크
        for(int i=0; i<FOOD_COUNT; i++) if(foods[i].x == fx && foods[i].y == fy) overlap = 1;

        if(!overlap) {
            target_food->x = fx;
            target_food->y = fy;
            break;
        }
    }
}

// UI 그리기
void draw_ui(Snake *s1, Snake *s2, game_size_t size) {
    attron(COLOR_PAIR(1)); // Green
    mvprintw(size.start_y - 2, size.start_x, "P1(Arrow): %d", s1->score);
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2)); // Red
    mvprintw(size.start_y - 2, size.start_x + 20, "P2(WASD): %d", s2->score);
    attroff(COLOR_PAIR(2));

    mvprintw(size.start_y - 2, size.start_x + 40, "TIME: %03d", remaining_time);
}

// 뱀 이동 로직
void update_snake_pos(Snake *s) {
    if (s->dir_x == 0 && s->dir_y == 0) return; // 움직이지 않음

    // 몸통 이동
    for (int i = s->length - 1; i > 0; i--) {
        s->body[i] = s->body[i - 1];
    }
    // 머리 이동
    s->body[0].x += s->dir_x;
    s->body[0].y += s->dir_y;
}

// 게임 실행 함수
int run_game(game_size_t size) {
    Snake p1, p2;
    Point foods[FOOD_COUNT];
    int ch;
    int paused = 0;

    remaining_time = TIMEOUT_SEC;
    is_time_over = 0;

    // Ncurses 설정
    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);

    // 색상 정의
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // P1
    init_pair(2, COLOR_RED, COLOR_BLACK);   // P2
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // Food
    init_pair(4, COLOR_WHITE, COLOR_BLACK);  // Wall

    srand(time(NULL));

    // 플레이어 초기화 (양쪽 끝에서 시작)
    init_snake(&p1, size.start_x + 5, size.start_y + size.box_height / 2, 1, 'O');
    init_snake(&p2, size.start_x + size.box_width - 6, size.start_y + size.box_height / 2, 2, 'X');

    // 초기 먹이 생성
    for(int i=0; i<FOOD_COUNT; i++) {
        foods[i].x = 0; foods[i].y = 0; // 초기화
        spawn_food(&foods[i], &p1, &p2, foods, size);
    }

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

    // 게임 루프
    while (!is_time_over) {
        // 입력 처리 (버퍼에 있는 모든 키를 읽어서 처리)
        while((ch = getch()) != ERR) {
            if (ch == 'q') { is_time_over = 1; break; }
            else if (ch == 'p') { paused = !paused; }
            
            if (!paused) {
                // P1 Controls (Arrows)
                if (ch == KEY_UP && p1.dir_y == 0) { p1.dir_x = 0; p1.dir_y = -1; }
                else if (ch == KEY_DOWN && p1.dir_y == 0) { p1.dir_x = 0; p1.dir_y = 1; }
                else if (ch == KEY_LEFT && p1.dir_x == 0) { p1.dir_x = -1; p1.dir_y = 0; }
                else if (ch == KEY_RIGHT && p1.dir_x == 0) { p1.dir_x = 1; p1.dir_y = 0; }
                
                // P2 Controls (WASD)
                else if ((ch == 'w' || ch == 'W') && p2.dir_y == 0) { p2.dir_x = 0; p2.dir_y = -1; }
                else if ((ch == 's' || ch == 'S') && p2.dir_y == 0) { p2.dir_x = 0; p2.dir_y = 1; }
                else if ((ch == 'a' || ch == 'A') && p2.dir_x == 0) { p2.dir_x = -1; p2.dir_y = 0; }
                else if ((ch == 'd' || ch == 'D') && p2.dir_x == 0) { p2.dir_x = 1; p2.dir_y = 0; }
            }
        }

        if (is_time_over) break;

        if (paused) {
            draw_ui(&p1, &p2, size);
            mvprintw(size.start_y - 1, size.start_x, "== PAUSED (Press 'p') ==");
            napms(100);
            continue;
        } else {
             mvprintw(size.start_y - 1, size.start_x, "                          ");
        }

        // 1. 꼬리 위치 저장 (먹이 섭취 시 증가용)
        Point p1_tail = p1.body[p1.length - 1];
        Point p2_tail = p2.body[p2.length - 1];

        // 2. 이동 업데이트
        update_snake_pos(&p1);
        update_snake_pos(&p2);

        // 3. 충돌 검사 - 벽 (Game Over Check)
        if (p1.body[0].x <= size.start_x || p1.body[0].x >= size.start_x + size.box_width - 1 ||
            p1.body[0].y <= size.start_y || p1.body[0].y >= size.start_y + size.box_height - 1) {
            p1.is_dead = 1;
        }
        if (p2.body[0].x <= size.start_x || p2.body[0].x >= size.start_x + size.box_width - 1 ||
            p2.body[0].y <= size.start_y || p2.body[0].y >= size.start_y + size.box_height - 1) {
            p2.is_dead = 1;
        }

        // 4. 충돌 검사 - 상호 충돌 (PvP)
        // P1 머리가 P1 몸통, P2 몸통에 닿았는지
        for(int i=1; i<p1.length; i++) if(p1.body[0].x == p1.body[i].x && p1.body[0].y == p1.body[i].y) p1.is_dead = 1;
        for(int i=0; i<p2.length; i++) if(p1.body[0].x == p2.body[i].x && p1.body[0].y == p2.body[i].y) p1.is_dead = 1;
        
        // P2 머리가 P2 몸통, P1 몸통에 닿았는지
        for(int i=1; i<p2.length; i++) if(p2.body[0].x == p2.body[i].x && p2.body[0].y == p2.body[i].y) p2.is_dead = 1;
        for(int i=0; i<p1.length; i++) if(p2.body[0].x == p1.body[i].x && p2.body[0].y == p1.body[i].y) p2.is_dead = 1;

        // 머리끼리 충돌 (무승부 또는 둘 다 사망)
        if(p1.body[0].x == p2.body[0].x && p1.body[0].y == p2.body[0].y) {
            p1.is_dead = 1;
            p2.is_dead = 1;
        }

        if (p1.is_dead || p2.is_dead) goto game_end;

        // 5. 먹이 섭취 확인 (다중 먹이)
        for(int f=0; f<FOOD_COUNT; f++) {
            // P1 Eating
            if (p1.body[0].x == foods[f].x && p1.body[0].y == foods[f].y) {
                p1.score += 10;
                if (p1.length < MAX_SNAKE_LEN) {
                    p1.body[p1.length] = p1_tail;
                    p1.length++;
                }
                spawn_food(&foods[f], &p1, &p2, foods, size);
            }
            // P2 Eating
            if (p2.body[0].x == foods[f].x && p2.body[0].y == foods[f].y) {
                p2.score += 10;
                if (p2.length < MAX_SNAKE_LEN) {
                    p2.body[p2.length] = p2_tail;
                    p2.length++;
                }
                spawn_food(&foods[f], &p1, &p2, foods, size);
            }
        }

        // 6. 렌더링
        clear();
        
        // 맵 테두리
        attron(COLOR_PAIR(4));
        for (int y = size.start_y; y < size.start_y + size.box_height; y++) {
            for (int x = size.start_x; x < size.start_x + size.box_width; x++) {
                if (y == size.start_y || y == size.start_y + size.box_height - 1 || 
                    x == size.start_x || x == size.start_x + size.box_width - 1) {
                    mvaddch(y, x, '#');
                }
            }
        }
        attroff(COLOR_PAIR(4));

        // 먹이들
        attron(COLOR_PAIR(3));
        for(int i=0; i<FOOD_COUNT; i++) mvaddch(foods[i].y, foods[i].x, '@');
        attroff(COLOR_PAIR(3));

        // P1 그리기
        attron(COLOR_PAIR(p1.color_pair));
        for (int i = 0; i < p1.length; i++) 
            mvaddch(p1.body[i].y, p1.body[i].x, (i==0 ? p1.head_char : 'o'));
        attroff(COLOR_PAIR(p1.color_pair));

        // P2 그리기
        attron(COLOR_PAIR(p2.color_pair));
        for (int i = 0; i < p2.length; i++) 
            mvaddch(p2.body[i].y, p2.body[i].x, (i==0 ? p2.head_char : 'x'));
        attroff(COLOR_PAIR(p2.color_pair));

        draw_ui(&p1, &p2, size);
        
        refresh();
        napms(100); 
    }

game_end:
    timer_delete(timer_tick);
    timer_delete(timer_limit);

    nodelay(stdscr, FALSE);
    clear();
    
    // 결과 화면
    int cy = size.term_height/2;
    int cx = size.term_width/2;

    mvprintw(cy - 4, cx - 5, "GAME OVER");
    
    if (is_time_over && remaining_time <= 0) {
        mvprintw(cy - 2, cx - 12, "Time Limit Exceeded!");
        if (p1.score > p2.score) mvprintw(cy, cx - 8, "PLAYER 1 WINS!");
        else if (p2.score > p1.score) mvprintw(cy, cx - 8, "PLAYER 2 WINS!");
        else mvprintw(cy, cx - 4, "DRAW!");
    } else {
        if (p1.is_dead && p2.is_dead) mvprintw(cy, cx - 10, "Double KO! DRAW!");
        else if (p1.is_dead) mvprintw(cy, cx - 10, "PLAYER 2 WINS! (P1 Died)");
        else if (p2.is_dead) mvprintw(cy, cx - 10, "PLAYER 1 WINS! (P2 Died)");
    }

    mvprintw(cy + 2, cx - 10, "P1 Score: %d", p1.score);
    mvprintw(cy + 3, cx - 10, "P2 Score: %d", p2.score);
    mvprintw(cy + 5, cx - 15, "Press 'r' to Restart, 'q' to Quit");
    refresh();

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
// 4. 메인 함수
// ==========================================
int main() {
    game_size_t gameSize;
    
    initscr();
    getmaxyx(stdscr, gameSize.term_height, gameSize.term_width);
    endwin();

    // 맵 크기를 좀 더 크게 설정 (2인용 공간 확보)
    gameSize.box_height = 25;
    gameSize.box_width = 60;
    gameSize.start_y = (gameSize.term_height - gameSize.box_height) / 2;
    gameSize.start_x = (gameSize.term_width - gameSize.box_width) / 2;

    while(1) {
        int restart = run_game(gameSize);
        if (!restart) {
            printf("Multiplayer Game Ended.\n");
            break;
        }
    }
    return 0;
}
