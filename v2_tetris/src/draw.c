#include "../include/tetrimino.h"
#include <ncurses.h>
#include <check.h>
#define LEVEL_SLOPE 10
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

void draw_tet(WINDOW* win, int row, int col, Tetrimino tet) {
    char* block = "#";
    if (is_inside(row, col, tet)) {
        mvwprintw(win, tet.y1, tet.x1, "%s", block);
        mvwprintw(win, tet.y2, tet.x2, "%s", block);
        mvwprintw(win, tet.y3, tet.x3, "%s", block);
        mvwprintw(win, tet.y4, tet.x4, "%s", block);
    }
}

void erase_tet(WINDOW *win, int row, int col, Tetrimino tet) {
    char* hollow = ".";
    if (is_inside(row, col, tet)) {
        mvwprintw(win, tet.y1, tet.x1, "%s", hollow);
        mvwprintw(win, tet.y2, tet.x2, "%s", hollow);
        mvwprintw(win, tet.y3, tet.x3, "%s", hollow);
        mvwprintw(win, tet.y4, tet.x4, "%s", hollow);
    }
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

void draw_ghost_line(WINDOW *win, char* grid[], Tetrimino tet, int row) {
    char* line = " ";
    // int max_x = MAX(MAX(tet.x1, tet.x2), MAX(tet.x3, tet.x4));
    // int min_x = MIN(MIN(tet.x1, tet.x2), MIN(tet.x3, tet.x4));
    int max_y = MAX(MAX(tet.y1, tet.y2), MAX(tet.y3, tet.y4)); 
    /*
    for (int i = max_y; i < row; i++) {
        if (grid[i][max_x] == 0) {
            mvwprintw(win, i, max_x, "%s", line);
        }
    } 
    */
    for (int i = max_y; i < row; i++) {
        if (grid[i][tet.x1] == 0) {
            mvwprintw(win, i, tet.x1, "%s", line);
        } else break;
    } 
    for (int i = max_y; i < row; i++) {
        if (grid[i][tet.x2] == 0) {
            mvwprintw(win, i, tet.x2, "%s", line);
        } else break;
    } 
    for (int i = max_y; i < row; i++) {
        if (grid[i][tet.x3] == 0) {
            mvwprintw(win, i, tet.x3, "%s", line);
        }
    } 
    for (int i = max_y; i < row; i++) {
        if (grid[i][tet.x4] == 0) {
            mvwprintw(win, i, tet.x4, "%s", line);
        }
    } 
}

void draw_interface(game_status_t status, prev_score_t prev) {
    /* line cleared */
    int col = status.interface_x + status.box_width + 3; 
    int row = status.interface_y + 1;
    int line_cleared = status.line_cleared; // 이번에 삭제된 라인 수
    int level = (line_cleared / LEVEL_SLOPE);
    int score = status.score;
    Tetrimino offset = status.tet;

    mvwprintw(stdscr, row, col, "::Scores::");
    mvwprintw(stdscr, row + 1, col, "LINE: %d", line_cleared);
    mvwprintw(stdscr, row + 2, col, "SCORE: %d", score);
    mvwprintw(stdscr, row + 3, col, "LEVEL: %d", level);
    
    mvwprintw(stdscr, row + 5, col, "::Prev Game::");
    //mvwprintw(stdscr, row + 6, col, "Max Score: %d", status.prev_score);
    mvwprintw(stdscr, row + 6, col, "Max LINE: %d", prev.max_line);
    mvwprintw(stdscr, row + 7, col, "Max Score: %d", prev.max_score);
    mvwprintw(stdscr, row + 8, col, "Max LEVEL: %d", prev.max_level);


    int block_y = row + 10;
    int block_x = col + 4;

    mvwprintw(stdscr, block_y, col, "::NEXT::");
    
    mvwprintw(stdscr, block_y+1, col, "           ");
    mvwprintw(stdscr, block_y+2, col, "           ");
    mvwprintw(stdscr, block_y+3, col, "           ");
    mvwprintw(stdscr, block_y+4, col, "           ");
    block_y++;

    mvwprintw(stdscr, (block_y + offset.y1), (block_x + offset.x1), "#");
    mvwprintw(stdscr, (block_y + offset.y2), (block_x + offset.x2), "#");
    mvwprintw(stdscr, (block_y + offset.y3), (block_x + offset.x3), "#");
    mvwprintw(stdscr, (block_y + offset.y4), (block_x + offset.x4), "#");
}    
