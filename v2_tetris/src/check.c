#include "../include/tetrimino.h"

int is_inside(int row, int col, Tetrimino tet) {
    if (
        tet.x1 > 0 && tet.x1 < col-1 &&    
        tet.x2 > 0 && tet.x2 < col-1 &&    
        tet.x3 > 0 && tet.x3 < col-1 &&    
        tet.x4 > 0 && tet.x4 < col-1 &&    
        tet.y1 > 0 && tet.y1 < row-1 &&    
        tet.y2 > 0 && tet.y2 < row-1 &&    
        tet.y3 > 0 && tet.y3 < row-1 &&    
        tet.y4 > 0 && tet.y4 < row-1 ) {
        return 1;
    }    
    return 0;
}
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
