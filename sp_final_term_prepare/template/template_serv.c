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

typedef struct GameSize {
   int box_height, box_width;
   int start_x, start_y;
   int term_height, term_width;
} game_size_t;

void init_ncurses();
char** generate_grid(int box_height, int box_width);
int socket_init(char* ip, int port); 
game_size_t game_size_init();

int main(int ac, char* argv[]) {
    if (ac != 3) {
        fprintf(stderr, "parameter: <IP> <PORT>\n");
        exit(1);
    }

    int clnt_sock;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size;
    char buf[BUFSIZ];

    char **grid = generate_grid(box_height, box_width); 
    /* bind까지 완료된 소켓 fd 리턴 */
    int serv_socket = socket_init(argv[1], argv[2]);
    
    if (listen(serv_socket, BACKLOG) == -1) oops("listen() error\n");  
    
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
    if (clnt_sock == -1) oops("accept() error\n");
    
    game_size_t gzise = game_size_init();
    
    // ncurses start 
    init_ncurses();

    WINDOW *win = newwin(gsize.box_height, gsize.box_width, 
                         gsize.start_y,    gsize.start_x);
    box(win); 

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
