#pragma once
#include "tetrimino.h"

Tetrimino make_tet(int x, int y, Tetrimino tet);
Tetrimino *generate_tets();
char** generate_grid(int boX_height, int box_width);
