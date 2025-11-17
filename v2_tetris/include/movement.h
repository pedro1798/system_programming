#pragma once
#include "tetrimino.h"

Tetrimino rotate_tet(char* grid[], game_status_t status, char* flag);
Tetrimino move_tet(char flag, Tetrimino tet, int box_height, int box_width);
