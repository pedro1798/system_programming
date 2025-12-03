#include <ncurses.h>
#include <strings.h>
#include <itimer.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BACKLOG 1
#define oops(msg) { perror(msg); exit(msg); }
#define GRID_HEIGHT 10
#define GRID_WIDTH 10

volatile int game_flag = 2;
static volatile sig_atomic_t tick = 0;

/*
 * TODO:
 * main의 while loop에서 flag로 무엇을 실행할지 분기
 * 시작화면, 게임화면 등 로직 함수로 분리, 정수로 플래그 리턴  
 * 매번 시작할 때 grid, 화면 등 초기화
 * 정보 .dat로 저장? 
 */

typedef struct GameSize {
   int box_height, box_width;
   int start_x, start_y;
   int term_height, term_width;
} game_size_t;

void init_ncurses();
char** generate_grid(int box_height, int box_width);
int socket_init(char* ip, int port); 
int run_game(game_size_t gsize);
int main_page();
game_size_t game_size_init();
void set_ticker(int interval);
void sigalrm_handler(int sig);

int main(int ac, char* argv[]) {
    if (ac != 3) {
        fprintf(stderr, "parameter: <IP> <PORT>\n");
        exit(1);
    }
    /* 게임 윈도우 좌표(행, 열 등) 저장 구조체 초기화 */
    game_size_t gzise = game_size_init();
    
    while (1) {
        switch(game_flag) {
            case 1: run_game(gsize); break;
            case 2: main_page(gsize); break;
    }
    
}

void init_ncurses() {
    initscr();
    refresh();
    cbreak(); // 버퍼링된 입력 즉시 전달(즉시 입력 받기)
    noecho(); // 입력 문자 화면 표시 x
    keypad(stdscr, TRUE); // 키패드 입력 활정화 
    nodelay(stdscr, TRUE);
    curs_set(0); // 커서 숨김
}

char** generate_grid(int box_height, int box_width) {
    char **grid = malloc(box_height * sizeof(char *));
    for (int i = 0; i < box_height; i++) {
        grid[i] = malloc(box_width * sizeof(char));
        for (int j = 0; j < box_width; j++) grid[i][j] = 0;
    }
    return grid;
}

int socket_init(char* ip, int port) {
    /*
     * 소켓에 IPv4, port # bind 후 리턴 
     */
    int sock;
    struct sockaddr_in sock_addr;
    /* PF_INET: protocol family, socket 함수 호출 시 */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1) opps("socket() error");

    memset(&sock_addr, 0, sizeof(sock_addr));
    /* Address family, sockaddr_in 구조체 사용 시 
     * PF_INET과 의미는 같음 */
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = inet_addr(ip);
    sock_addr.sin_port = htons(port);

    if (bind(sock, 
            (struct sockaddr *)&sock_addr, 
            sizeof(sock_addr)) == -1) {
        oops("bind() error");
    }
    return sock;
}

game_size_t game_size_init() {
    game_size_t gsize;
    getmaxyx(stdscr, gsize.term_height, 
            gsize.term_width); // 터미널 크기 저장
    
    gsize.box_height = GRID_HEIGHT;
    gsize.box_width = GRID_WIDTH;

    gsize.start_y = (term_height - box_height) / 2; 
    gsize.start_x = (term_width - box_width) / 2; 
    
    return gsize;
}

int run_game(game_size_t gsize) {
    int clnt_sock;
    /* 클라이언트 sin_family, sin_port, sin_addr 저장 변수 */
    struct sockaddr_in clnt_addr;
    /* sockaddr_in 구조체의 크기를 바이트 단위로 저장 */
    socklen_t clnt_addr_size;
    char buf[BUFSIZ];

    /* bind까지 완료된 소켓 fd 리턴 */
    int serv_socket = socket_init(argv[1], argv[2]);
    
    /* 에러처리 */
    if (listen(serv_socket, BACKLOG) == -1) oops("listen() error\n");  
    
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1) oops("accept() error\n");
    
    /* 게임 로직에 필요한 이차원 배열 선언 */
    char **grid = generate_grid(gsize.box_height, gsize.box_width); 

    /* ncurses start */ 
    init_ncurses();

    /* 게임 윈도우 생성 */
    WINDOW *win = newwin(gsize.box_height, gsize.box_width, 
                         gsize.start_y,    gsize.start_x);
    box(win); 
    
}

int main_page(game_size_t gsize) {
    /* ncurses start */
    init_ncurses();
    nodelay(stdscr, FALSE);

    int text_x = gsize_width / 2;
    int text_y = gsize_height / 2;

    char* title_msg = "Game Start";
    char* press_any_key = "Press Any Key to Start";
    char* press_q_to_quit = "Press 'q' to Quit";
    mvwprintw(stdscr, text_y, text_x, "%s", title_msg);
    mvwprintw(stdscr, text_y+1, text_x, "%s", press_any_key);
    mvwprintw(stdscr, text_y+2, text_x, "%s", press_q_to_quit);

    refresh();
    
    int ch = getch();
    if (ch == 'q') return 0;
    else return 1;
}

int set_ticker(int interval) {
    sigset_t block_Set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGALRM);

    if (sigprocmask(SIG_BLOCK, &block_set, NULL) < 0) {
        oops("sigprocmask");
    }
    
    struct sigaction sa_sigalrm;
    sa_sigalrm.sa_handler = sigalrm_handler;
    sigemptyset(&sa_sigalrm.sa_mask);
    sa_sigalrm.sa_flags = SA_RESTART; // getch() 재시작? 

    sigaction(SIGALRM, &sa_sigalrm, NULL);

    /* 타이머 설정 */
    struct itimerval timer = {{0, interval}, {0, interval}};
    setitimer(ITIMER_REAL, &timer, NULL);

    if (sigprocmask(SIG_UNBLOCK, &block_Set, NULL) < 0) oops("sigprocmask");
} 

void sigalrm_handler(int sig) {
    (void)sig;
    tick = 1;
}
