#include <ncurses.h>
#include <strings.h>
#include <itimer.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define oops(msg) { perror(msg); exit(msg); }
#define GRID_HEIGHT 10
#define GRID_WIDTH 10

void init_ncurse();
char** generate_grid(int box_height, int box_width);

int main(int ac, char* argv[]) {
    init_ncurse();
    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width); // 터미널 크기 저장

    int box_height = GRID_HEIGHT;
    int box_width = GRID_WIDTH;

    int start_y = (term_height - box_height) / 2; 
    int start_x = (term_width - box_width) / 2; 

    WINDOW *win = newwin(box_height, box_width, start_y, start_x);
    box(win);

    char **grid = generate_grid(box_height, box_width); 

}

void init_ncurse() {
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

