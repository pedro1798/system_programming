#include "../include/tetrimino.h"

int is_collide(char* grid[], Tetrimino tet) {
    /*
     * 충돌 여부 판정
     * -1: 바닥
     * 0: 블럭 없음
     * 1: 블럭 있음
     */
    if(grid[tet.y1][tet.x1] == 0 &&
       grid[tet.y2][tet.x2] == 0 &&
       grid[tet.y3][tet.x3] == 0 &&
       grid[tet.y4][tet.x4] == 0) {
        return 0; // FALSE, 충돌하지 않는다
    }
    return 1; // TRUE, 충돌한다 
}
