#include "../include/tetrimino.h"
#include <ncurses.h>

void draw_tet(WINDOW* win, Tetrimino tet) {
    char* block = "#";
    mvwprintw(win, tet.y1, tet.x1, "%s", block);
    mvwprintw(win, tet.y2, tet.x2, "%s", block);
    mvwprintw(win, tet.y3, tet.x3, "%s", block);
    mvwprintw(win, tet.y4, tet.x4, "%s", block);
}

void draw_grid(WINDOW *win, char* grid[], int box_height, int box_width) {
    char* block = "#";
    char* hollow = ".";
    for (int i = 0; i < box_height; i++) {
        for (int j = 0; j < box_width; j++) {
            if (grid[i][j] == 1) {
                mvwprintw(win, i, j, "%s", block);               
            }
            if ( i > 0 && i < (box_height - 1) &&
                 j > 0 && j < (box_width - 1) &&
                 grid[i][j] == 0) {
                mvwprintw(win, i, j, "%s", hollow);    
            }

        }
    }
}

void erase_tet(WINDOW *win, Tetrimino tet) {
    char* hollow = ".";
    mvwprintw(win, tet.y1, tet.x1, "%s", hollow);
    mvwprintw(win, tet.y2, tet.x2, "%s", hollow);
    mvwprintw(win, tet.y3, tet.x3, "%s", hollow);
    mvwprintw(win, tet.y4, tet.x4, "%s", hollow);
}
