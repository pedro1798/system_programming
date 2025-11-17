#pragma once

typedef struct Tetrimino {
    char name;
    int x1;
    int x2;
    int x3;
    int x4;
    int y1;
    int y2;
    int y3;
    int y4;
} Tetrimino;

typedef struct GameStatus {
    Tetrimino tet;
    int box_width;
    int box_height;
    int interface_x;
    int interface_y;
    int line;
    int line_cleared;
    int level;
    int score;
    int prev_score;
} game_status_t;

typedef struct PrevScore {
    int max_line;
    int max_score;
    int max_level;
	
} prev_score_t;
