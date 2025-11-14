#pragma once
#include "tetrimino.h"
typedef struct _win_st WINDOW;

void draw_tet(WINDOW* win, Tetrimino tet);
void draw_grid(WINDOW *win, char* grid[], int box_height, int box_width);
void erase_tet(WINDOW *win, Tetrimino tet);
