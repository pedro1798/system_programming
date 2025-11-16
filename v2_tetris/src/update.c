#include "../include/tetrimino.h"

int update_grid(char* grid[], Tetrimino tet, int box_height, int box_width) {
    int line_cnt = 0;
    int line_width = 0;

    grid[tet.y1][tet.x1] = 1;
    grid[tet.y2][tet.x2] = 1;
    grid[tet.y3][tet.x3] = 1;
    grid[tet.y4][tet.x4] = 1;

    for (int i = 0; i < box_height; i++) {
        for (int j = 0; j < box_width; j++) {
            if (grid[i][j] == 1) {
                if (++line_width == box_width - 2) {
                    /* 라인이 다 찼을 경우 */
                    for (int k = 0; k < box_width; k++) {
                        grid[i][k] = 0; /* i번째 라인을 비운다*/
                    }
                    /* [i-1:0:-1] 라인에 블럭이 있다면 아래로 한 칸 내린다 */
                    for (int l = i - 1; l >= 0 ; l--) {
                        for (int p = 0; p < box_width; p++) {
                            if (grid[l][p] == 1) {
                                grid[l][p] = 0; 
                                grid[l + 1][p] = 1;
                            }
                        }
                    } 
                    line_cnt++;
                }
            }
        }
        line_width = 0;
    }
    
    return line_cnt;
}

