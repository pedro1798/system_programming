// usleep 사용을 위한 매크로 정의
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ncurses.h>

#define PORT 12345
#define SERVER_IP "127.0.0.1" 
#define MAX_SNAKE_LEN 1024
#define FOOD_COUNT 5
#define PLAYER_COUNT 2

typedef struct Point {
    int x, y;
} Point;

typedef struct SnakeData {
    Point body[MAX_SNAKE_LEN];
    int length;
    int score;
    int is_dead;
} SnakeData;

typedef struct GameState {
    SnakeData players[PLAYER_COUNT];
    Point foods[FOOD_COUNT];
    int remaining_time;
    int winner_id;
    int map_width, map_height;
    int start_x, start_y;
} GameState;

void draw_ui(int player_id, const GameState *state) {
    attron(COLOR_PAIR(1));
    mvprintw(state->start_y - 2, state->start_x, "P1 Score: %d %s", state->players[0].score, state->players[0].is_dead ? "[DEAD]" : "");
    attroff(COLOR_PAIR(1));

    attron(COLOR_PAIR(2));
    mvprintw(state->start_y - 2, state->start_x + 25, "P2 Score: %d %s", state->players[1].score, state->players[1].is_dead ? "[DEAD]" : "");
    attroff(COLOR_PAIR(2));

    mvprintw(state->start_y - 2, state->start_x + 50, "TIME: %03d sec", state->remaining_time);
    mvprintw(state->start_y - 1, state->start_x, "You are Player %d. Control with Arrow Keys.", player_id + 1);
}

void render_game(int my_id, const GameState *state) {
    clear();
    
    attron(COLOR_PAIR(4));
    for (int y = state->start_y; y < state->start_y + state->map_height; y++) {
        for (int x = state->start_x; x < state->start_x + state->map_width; x++) {
            if (y == state->start_y || y == state->start_y + state->map_height - 1 || 
                x == state->start_x || x == state->start_x + state->map_width - 1) {
                mvaddch(y, x, '#');
            }
        }
    }
    attroff(COLOR_PAIR(4));

    attron(COLOR_PAIR(3));
    for(int i=0; i<FOOD_COUNT; i++) mvaddch(state->foods[i].y, state->foods[i].x, '@');
    attroff(COLOR_PAIR(3));

    if (!state->players[0].is_dead) {
        attron(COLOR_PAIR(1));
        for (int i = 0; i < state->players[0].length; i++) 
            mvaddch(state->players[0].body[i].y, state->players[0].body[i].x, (i==0 ? 'O' : 'o'));
        attroff(COLOR_PAIR(1));
    }

    if (!state->players[1].is_dead) {
        attron(COLOR_PAIR(2));
        for (int i = 0; i < state->players[1].length; i++) 
            mvaddch(state->players[1].body[i].y, state->players[1].body[i].x, (i==0 ? 'X' : 'x'));
        attroff(COLOR_PAIR(2));
    }

    draw_ui(my_id, state);
    refresh();
}

void draw_game_over(int my_id, const GameState *state, int term_h, int term_w) {
    clear();
    int cy = term_h/2;
    int cx = term_w/2;
    
    mvprintw(cy - 4, cx - 5, "GAME OVER");
    mvprintw(cy + 2, cx - 10, "P1 Score: %d", state->players[0].score);
    mvprintw(cy + 3, cx - 10, "P2 Score: %d", state->players[1].score);

    if (state->winner_id == my_id) {
        mvprintw(cy, cx - 10, "YOU WIN!");
    } else if (state->winner_id == (my_id + 1) % PLAYER_COUNT) {
        mvprintw(cy, cx - 10, "YOU LOSE!");
    } else if (state->winner_id == 2) {
        mvprintw(cy, cx - 5, "DRAW!");
    } else {
        if (state->players[my_id].is_dead) {
             mvprintw(cy, cx - 10, "YOU DIED! The Other Player Wins.");
        } else {
             mvprintw(cy, cx - 10, "YOU WIN! The Other Player Died.");
        }
    }

    if (state->remaining_time <= 0) {
        mvprintw(cy - 2, cx - 12, "Time Limit Exceeded!");
    }
    
    mvprintw(cy + 5, cx - 15, "Press 'q' to Quit");
    refresh();

    while(getch() != 'q');
}

int main(int argc, char const *argv[]) {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    GameState state;
    int my_id = -1;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    if (recv(sock, &my_id, sizeof(int), 0) <= 0 || my_id == -1) {
        printf("Failed to receive player ID.\n");
        close(sock);
        return -1;
    }

    initscr();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
    
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_BLACK);

    int term_height, term_width;
    getmaxyx(stdscr, term_height, term_width);

    int game_running = 1;
    while (game_running) {
        valread = recv(sock, &state, sizeof(GameState), 0);
        if (valread == sizeof(GameState)) {
            render_game(my_id, &state);
            if (state.winner_id != -1) {
                game_running = 0;
            }
        } else if (valread == 0) {
            game_running = 0;
            printf("Server disconnected.\n");
        }
        
        int ch = getch();
        if (ch != ERR) {
            char input_dir = 'N'; 
            if (ch == KEY_UP) input_dir = 'U';
            else if (ch == KEY_DOWN) input_dir = 'D';
            else if (ch == KEY_LEFT) input_dir = 'L';
            else if (ch == KEY_RIGHT) input_dir = 'R';
            else if (ch == 'q' || ch == 'Q') {
                input_dir = 'Q';
                game_running = 0;
            }

            if (input_dir != 'N') {
                send(sock, &input_dir, 1, 0);
            }
        }
        usleep(10000); 
    }

    draw_game_over(my_id, &state, term_height, term_width);

    endwin();
    close(sock);
    return 0;
}
