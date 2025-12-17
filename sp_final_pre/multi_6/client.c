#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ncurses.h>
#include <pthread.h>

// =============================================================
// [공통 정의] 헤더 파일 내용 통합
// =============================================================
#define PORT 9999
#define INITIAL_WIDTH 40
#define INITIAL_HEIGHT 20
#define MAX_SNAKE_LEN 100
#define TIMEOUT_SEC 180 

#define KEY_UP_CMD 'U'
#define KEY_DOWN_CMD 'D'
#define KEY_LEFT_CMD 'L'
#define KEY_RIGHT_CMD 'R'
#define KEY_PAUSE_CMD 'S'
#define CMD_RESTART 'Y'   // 이름 변경
#define CMD_QUIT 'N'      // 이름 변경

typedef struct {
    int x, y;
} Point;

typedef struct {
    int id;                
    Point body[MAX_SNAKE_LEN];
    int length;
    int dir_x, dir_y;
    int score;
    int map_width;          
    int map_height;
    int speed_ms;           
    int is_paused;          
    int is_dead;            
    Point food;             
    
    long long food_timestamps[MAX_SNAKE_LEN]; 
    int food_eat_idx;       
    int last_shrink_score;  
} PlayerState;

typedef struct {
    PlayerState p[2];       
    int remaining_time;     
    int game_over;          
    int winner_id;          
} GameState;

// =============================================================
// [클라이언트] 전역 변수 및 함수
// =============================================================
int sock;
GameState state;
int my_id = 0; // 0 or 1 (실행 인자로 받음)

void setup_colors() {
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // 나
    init_pair(2, COLOR_RED, COLOR_BLACK);   // 적 (정보용)
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // 먹이
    init_pair(4, COLOR_WHITE, COLOR_BLACK); // 벽
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // UI 강조
}

// 화면 그리기
void draw_game() {
    clear();
    PlayerState *me = &state.p[my_id];
    PlayerState *opp = &state.p[1 - my_id];

    // [조건 6, 추가조건 1] UI 표시
    attron(COLOR_PAIR(5));
    mvprintw(0, 0, "TIME: %03d sec", state.remaining_time);
    
    // 내 점수
    mvprintw(1, 0, "MY SCORE : %d  (Size: %dx%d)", me->score, me->map_width, me->map_height);
    // 상대 점수 (바로 아래)
    mvprintw(2, 0, "OPP SCORE: %d  (Size: %dx%d)", opp->score, opp->map_width, opp->map_height);
    attroff(COLOR_PAIR(5));

    // 상태 메시지
    if (me->is_paused) {
        mvprintw(0, 20, "== PAUSED (Press S) ==");
    }
    if (me->speed_ms < 100) {
        attron(COLOR_PAIR(2));
        mvprintw(1, 40, "!! SPEED UP !!");
        attroff(COLOR_PAIR(2));
    }
    if (me->map_width < INITIAL_WIDTH) {
        attron(COLOR_PAIR(2));
        mvprintw(2, 40, "!! MAP SHRINK !!");
        attroff(COLOR_PAIR(2));
    }

    // 맵 그리기 기준점 (UI 아래)
    int offset_y = 4;
    int offset_x = 1;

    // [기본조건 2] 맵 (동적 크기 반영)
    attron(COLOR_PAIR(4));
    // 상하 벽
    for (int x = 0; x < me->map_width; x++) {
        mvaddch(offset_y, offset_x + x, '#');
        mvaddch(offset_y + me->map_height - 1, offset_x + x, '#');
    }
    // 좌우 벽
    for (int y = 0; y < me->map_height; y++) {
        mvaddch(offset_y + y, offset_x, '#');
        mvaddch(offset_y + y, offset_x + me->map_width - 1, '#');
    }
    attroff(COLOR_PAIR(4));

    // 먹이
    attron(COLOR_PAIR(3));
    mvaddch(offset_y + me->food.y, offset_x + me->food.x, '@');
    attroff(COLOR_PAIR(3));

    // 뱀
    attron(COLOR_PAIR(1));
    for (int i = 0; i < me->length; i++) {
        char shape = (i == 0) ? 'O' : 'o';
        mvaddch(offset_y + me->body[i].y, offset_x + me->body[i].x, shape);
    }
    attroff(COLOR_PAIR(1));

    refresh();
}

// 종료 화면
void draw_game_over() {
    clear();
    int r = 5, c = 5;
    
    attron(COLOR_PAIR(2));
    mvprintw(r++, c, "==========================");
    mvprintw(r++, c, "       GAME OVER          ");
    mvprintw(r++, c, "==========================");
    attroff(COLOR_PAIR(2));
    
    r++;
    // [추가조건 5] 결과 기록 및 표시
    mvprintw(r++, c, " FINAL SCORES ");
    mvprintw(r++, c, " ME  : %d", state.p[my_id].score);
    mvprintw(r++, c, " OPP : %d", state.p[1-my_id].score);
    r++;
    
    if (state.winner_id == my_id) {
        attron(COLOR_PAIR(1));
        mvprintw(r++, c, " RESULT: YOU WIN! :)");
        attroff(COLOR_PAIR(1));
    } else if (state.winner_id == -1) {
        mvprintw(r++, c, " RESULT: DRAW");
    } else {
        attron(COLOR_PAIR(2));
        mvprintw(r++, c, " RESULT: YOU LOSE :(");
        attroff(COLOR_PAIR(2));
    }
    
    if (state.remaining_time <= 0) {
        mvprintw(r++, c, " (Time Limit Exceeded)");
    }

    r += 2;
    // [기본조건 7] 재시작 질문
    mvprintw(r++, c, " Play Again? (y/n) ");
    refresh();
}

// 키 입력 스레드
void *input_handler(void *arg) {
    int ch;
    char cmd;
    while (1) {
        ch = getch();
        cmd = 0;
        
        if (state.game_over) {
            if (ch == 'y' || ch == 'Y') cmd = CMD_RESTART;
            else if (ch == 'n' || ch == 'N') {
                endwin();
                exit(0);
            }
        } else {
            if (ch == KEY_UP) cmd = KEY_UP_CMD;
            else if (ch == KEY_DOWN) cmd = KEY_DOWN_CMD;
            else if (ch == KEY_LEFT) cmd = KEY_LEFT_CMD;
            else if (ch == KEY_RIGHT) cmd = KEY_RIGHT_CMD;
            else if (ch == 's' || ch == 'S') cmd = KEY_PAUSE_CMD;
        }

        if (cmd != 0) {
            send(sock, &cmd, 1, 0);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <Server_IP> <Player_ID(0 or 1)>\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    my_id = atoi(argv[2]);
    if (my_id < 0 || my_id > 1) {
        printf("Player ID must be 0 or 1.\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection Failed.\n");
        return 1;
    }

    // Ncurses 설정
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    setup_colors();

    pthread_t tid;
    pthread_create(&tid, NULL, input_handler, NULL);

    while (1) {
        // 서버로부터 상태 수신
        int len = recv(sock, &state, sizeof(GameState), 0);
        if (len <= 0) break;

        if (state.game_over) {
            draw_game_over();
        } else {
            draw_game();
        }
    }

    endwin();
    return 0;
}
