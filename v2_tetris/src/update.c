#include "../include/tetrimino.h"

int update_grid(char* grid[], Tetrimino tet, int box_height, int box_width) {
    grid[tet.y1][tet.x1] = 1;
    grid[tet.y2][tet.x2] = 1;
    grid[tet.y3][tet.x3] = 1;
    grid[tet.y4][tet.x4] = 1;

    int saturate = 0;
    int sat_idx = -1;

    for (int i = 0; i < box_height; i++) {
        for (int j = 0; j < box_width; j++) {
            if (grid[i][j] == 1) {
                /* if there's a block */
                saturate++;
                if (saturate == box_width - 2) {
                    sat_idx = i;
                }
            }
        }
        saturate = 0;
    }
    if (sat_idx != -1) {
        /* if saturated */
        for (int k = 0; k < box_width; k++) {
            /* clear saturated line */
            grid[sat_idx][k] = 0;
        }
        for (int i = sat_idx-1; i > 0; i--) {
            for (int j = 0; j < box_width; j++) {
                if (grid[i][j] == 1) {
                    /* if there's a block */
                    grid[i][j] = 0;  
                    grid[i+1][j] = 1;
                }
	        }
        }
    }
    if (sat_idx == -1) return 0;
    return 1;
}

