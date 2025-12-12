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

typedef struct GameSize {
    int box_height, box_width;
    int start_x, start_y;
    int term_height, term_width;
} game_size_t;

typedef struct Point {
    int x, y;
} Point;

// 전역 변수 (시그널 핸들러와 공유하기 위해 volatile sig_atomic_t 사용)
volatile sig_atomic_t remaining_time = TIMEOUT_SEC;
volatile sig_atomic_t is_time_over = 0;

// ==========================================
// 2. 타이머 및 시그널 관련 함수
// ==========================================

// 시그널 핸들러
void handler(int sig, siginfo_t *si, void *uc) {
    if (sig == SIGUSR1) {
        // 1초마다 발생하는 틱: 남은 시간 감소
        if (remaining_time > 0) {
            remaining_time--;
        }
    } else if (sig == SIGUSR2) {
        // 3분 종료 시그널
        is_time_over = 1;
    }
}

// POSIX 타이머 생성 함수 (요청하신 형태)
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

// 뱀과 먹이 초기화
void init_game_objects(Point snake[], int *length, Point *food, game_size_t size) {
    *length = 3; // 초기 길이
    // 화면 중앙에서 시작
    int cx = size.start_x + size.box_width / 2;
    int cy = size.start_y + size.box_height / 2;
    
    for(int i=0; i<*length; i++) {
        snake[i].x = cx - i;
        snake[i].y = cy;
    }

    // 먹이 랜덤 위치 (벽 안쪽에 생성)
    food->x = (rand() % (size.box_width - 2)) + size.start_x + 1;
    food->y = (rand() % (size.box_height - 2)) + size.start_y + 1;
}

// UI 그리기 (점수, 남은 시간)
void draw_ui(int score, game_size_t size) {
    mvprintw(size.start_y - 2, size.start_x, "SCORE: %d", score);
    mvprintw(size.start_y - 2, size.start_x + 20, "TIME: %03d sec", remaining_time);
}

// 게임 한 판을 실행하는 함수 (반환값: 1=재시작, 0=종료)
int run_game(game_size_t size) {
    // 1. 초기화
    Point snake[MAX_SNAKE_LEN];
    int snake_len;
    Point food;
    int score = 0;
    int dir_x = 1, dir_y = 0; // 초기 이동 방향 (오른쪽)
    int ch;
    int paused = 0;

    // 전역 타이머 변수 초기화
    remaining_time = TIMEOUT_SEC;
    is_time_over = 0;

    // Ncurses 설정
    initscr();
    cbreak();
    noecho();
    curs_set(0); // 커서 숨김
    keypad(stdscr, TRUE); // 특수키 허용
    nodelay(stdscr, TRUE); // getch()를 비동기로 설정 (대기하지 않음)

    srand(time(NULL));
    init_game_objects(snake, &snake_len, &food, size);

    // 2. 타이머 설정
    timer_t timer_tick, timer_limit;
    struct sigaction sa;

    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);

    // 타이머 1: 1초마다 UI 갱신용 틱 (SIGUSR1)
    make_timer(&timer_tick, SIGUSR1, 1, 1);
    // 타이머 2: 3분(180초) 후 게임 종료 (SIGUSR2)
    make_timer(&timer_limit, SIGUSR2, TIMEOUT_SEC, 0);

    // 3. 게임 루프
    while (!is_time_over) {
        // 입력 처리
        ch = getch();
        if (ch != ERR) {
            if (ch == 'q') { // 종료
                is_time_over = 1; 
                break;
            }
            else if (ch == 's') { // 일시 정지 토글
                paused = !paused;
                if (paused) mvprintw(size.start_y - 1, size.start_x, "== PAUSED ==");
                else mvprintw(size.start_y - 1, size.start_x, "              ");
            }
            else if (!paused) {
                // 방향 전환 (반대 방향으로는 즉시 전환 불가)
                if (ch == KEY_UP && dir_y == 0) { dir_x = 0; dir_y = -1; }
                else if (ch == KEY_DOWN && dir_y == 0) { dir_x = 0; dir_y = 1; }
                else if (ch == KEY_LEFT && dir_x == 0) { dir_x = -1; dir_y = 0; }
                else if (ch == KEY_RIGHT && dir_x == 0) { dir_x = 1; dir_y = 0; }
            }
        }

        if (paused) {
            draw_ui(score, size); // 시간은 계속 흐르므로 UI 업데이트 필요
            napms(100);
            continue;
        }

        // 로직 업데이트: 뱀 몸통 이동 (뒤에서부터 머리 위치로 당김)
        Point tail = snake[snake_len - 1]; // 꼬리 위치 저장 (먹이 먹었을 때 사용)
        for (int i = snake_len - 1; i > 0; i--) {
            snake[i] = snake[i - 1];
        }
        // 머리 이동
        snake[0].x += dir_x;
        snake[0].y += dir_y;

        // 충돌 검사 1: 벽
        if (snake[0].x <= size.start_x || snake[0].x >= size.start_x + size.box_width - 1 ||
            snake[0].y <= size.start_y || snake[0].y >= size.start_y + size.box_height - 1) {
            break; // 게임 오버
        }

        // 충돌 검사 2: 자기 자신
        for (int i = 1; i < snake_len; i++) {
            if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
                is_time_over = 1; // 루프 탈출
                goto game_end;
            }
        }

        // 먹이 섭취 확인
        if (snake[0].x == food.x && snake[0].y == food.y) {
            score += 10;
            if (snake_len < MAX_SNAKE_LEN) {
                snake[snake_len] = tail; // 꼬리 늘리기
                snake_len++;
            }
            // 새 먹이 생성
            while(1) {
                food.x = (rand() % (size.box_width - 2)) + size.start_x + 1;
                food.y = (rand() % (size.box_height - 2)) + size.start_y + 1;
                // 먹이가 뱀 몸통에 생기지 않도록 체크
                int overlap = 0;
                for(int i=0; i<snake_len; i++) if(snake[i].x == food.x && snake[i].y == food.y) overlap = 1;
                if(!overlap) break;
            }
        }

        // 렌더링
        clear();
        
        // 맵 그리기
        for (int y = size.start_y; y < size.start_y + size.box_height; y++) {
            for (int x = size.start_x; x < size.start_x + size.box_width; x++) {
                if (y == size.start_y || y == size.start_y + size.box_height - 1 || 
                    x == size.start_x || x == size.start_x + size.box_width - 1) {
                    mvaddch(y, x, '#');
                }
            }
        }

        // 뱀 그리기
        for (int i = 0; i < snake_len; i++) {
            mvaddch(snake[i].y, snake[i].x, (i==0 ? 'O' : 'o'));
        }
        // 먹이 그리기
        mvaddch(food.y, food.x, '@');
        
        // UI 그리기
        draw_ui(score, size);
        
        refresh(); // 화면 갱신
        napms(100); // 속도 조절 (100ms)

        if (is_time_over) break;
    }

game_end:
    // 4. 타이머 삭제 및 정리
    timer_delete(timer_tick);
    timer_delete(timer_limit);

    // 5. 종료 화면 및 재시작 질문
    nodelay(stdscr, FALSE); // 입력 대기 모드로 변경
    clear();
    mvprintw(size.term_height/2 - 2, size.term_width/2 - 10, "GAME OVER");
    mvprintw(size.term_height/2 - 1, size.term_width/2 - 10, "Final Score: %d", score);
    
    if (remaining_time <= 0) {
        mvprintw(size.term_height/2, size.term_width/2 - 10, "Reason: Time Limit Exceeded!");
    } else {
        mvprintw(size.term_height/2, size.term_width/2 - 10, "Reason: Collision or Quit");
    }

    mvprintw(size.term_height/2 + 2, size.term_width/2 - 15, "Press 'r' to Restart, 'q' to Quit");
    refresh();

    int ret = 0;
    while(1) {
        int cmd = getch();
        if (cmd == 'r' || cmd == 'R') {
            ret = 1;
            break;
        } else if (cmd == 'q' || cmd == 'Q') {
            ret = 0;
            break;
        }
    }

    endwin(); // Ncurses 종료
    return ret;
}

// ==========================================
// 4. 메인 함수
// ==========================================
int main() {
    // 1. GameSize 구조체 설정 (터미널 크기 가정)
    game_size_t gameSize;
    
    // Ncurses를 잠깐 켜서 화면 크기를 알아냄
    initscr();
    getmaxyx(stdscr, gameSize.term_height, gameSize.term_width);
    endwin();

    // 게임 박스 크기 설정
    gameSize.box_height = 20;
    gameSize.box_width = 40;
    gameSize.start_y = (gameSize.term_height - gameSize.box_height) / 2;
    gameSize.start_x = (gameSize.term_width - gameSize.box_width) / 2;

    // 2. 게임 루프 (재시작 제어)
    while(1) {
        int restart = run_game(gameSize);
        if (!restart) {
            printf("Good Bye! Final Game Ended.\n");
            break;
        }
    }

    return 0;
}
