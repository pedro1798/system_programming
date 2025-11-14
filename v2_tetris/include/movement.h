#pragma once
#include "tetrimino.h"

Tetrimino rotate_tet(Tetrimino tet, int box_height, int box_width);
Tetrimino move_tet(char flag, Tetrimino tet, int box_height, int box_width);
