#include "../include/tetrimino.h"
#include <string.h>
#include "../include/check.h"
#include <stdlib.h>
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
 
Tetrimino rotate_tet(char* grid[], game_status_t status, char* flag) {
    Tetrimino tet = status.tet;
    if (tet.name == 'O') return tet;

    int min_x = MIN(MIN(tet.x1, tet.x2), MIN(tet.x3, tet.x4));
    int max_x = MAX(MAX(tet.x1, tet.x2), MAX(tet.x3, tet.x4));
    int pivot_x = tet.x2; 
    int pivot_y = tet.y2;

    Tetrimino tmp = tet;
    Tetrimino rot = tet;

    if (!strcmp(flag, "cw")) {
        rot.x1 -= pivot_x;
        rot.x2 -= pivot_x;
        rot.x3 -= pivot_x;
        rot.x4 -= pivot_x;
        rot.y1 -= pivot_y;
        rot.y2 -= pivot_y;
        rot.y3 -= pivot_y;
        rot.y4 -= pivot_y;

        tmp.x1 = -rot.y1;
        tmp.x2 = -rot.y2;
        tmp.x3 = -rot.y3;
        tmp.x4 = -rot.y4;
        tmp.y1 = rot.x1;
        tmp.y2 = rot.x2;
        tmp.y3 = rot.x3;
        tmp.y4 = rot.x4;

        tmp.x1 += pivot_x;
        tmp.x2 += pivot_x;
        tmp.x3 += pivot_x;
        tmp.x4 += pivot_x;
        tmp.y1 += pivot_y;
        tmp.y2 += pivot_y;
        tmp.y3 += pivot_y;
        tmp.y4 += pivot_y;
    } else if  (!strcmp(flag, "ccw")) {
        rot.x1 -= pivot_x;
        rot.x2 -= pivot_x;
        rot.x3 -= pivot_x;
        rot.x4 -= pivot_x;
        rot.y1 -= pivot_y;
        rot.y2 -= pivot_y;
        rot.y3 -= pivot_y;
        rot.y4 -= pivot_y;

        tmp.x1 = rot.y1;
        tmp.x2 = rot.y2;
        tmp.x3 = rot.y3;
        tmp.x4 = rot.y4;
        tmp.y1 = -rot.x1;
        tmp.y2 = -rot.x2;
        tmp.y3 = -rot.x3;
        tmp.y4 = -rot.x4;

        tmp.x1 += pivot_x;
        tmp.x2 += pivot_x;
        tmp.x3 += pivot_x;
        tmp.x4 += pivot_x;
        tmp.y1 += pivot_y;
        tmp.y2 += pivot_y;
        tmp.y3 += pivot_y;
        tmp.y4 += pivot_y;
    } else exit(-1);
    
    /* tmp는 회전시킨 블럭 */
    if (is_inside(status.box_height, status.box_width, tmp) &&
        !is_collide(grid, tmp)) 
        return tmp;
     
    if (min_x <= 2) {
        /* 벽에 붙었을 때 회전 */
        tmp.x1++; tmp.x2++; tmp.x3++; tmp.x4++;
        if (is_inside(status.box_height, status.box_width, tmp) &&
            !is_collide(grid, tmp)) 
            return tmp;
        tmp.x1++; tmp.x2++; tmp.x3++; tmp.x4++;
        if (is_inside(status.box_height, status.box_width, tmp) &&
            !is_collide(grid, tmp)) 
            return tmp;
    }
    
    if (max_x >= status.box_width - 2) {
        /* 벽에 붙었을 때 회전 */
        tmp.x1--; tmp.x2--; tmp.x3--; tmp.x4--;
        if (is_inside(status.box_height, status.box_width, tmp) &&
            !is_collide(grid, tmp)) 
            return tmp;
        tmp.x1--; tmp.x2--; tmp.x3--; tmp.x4--;
        if (is_inside(status.box_height, status.box_width, tmp) &&
            !is_collide(grid, tmp)) 
            return tmp;
    }

    return tet;
}

Tetrimino move_tet(char flag, Tetrimino tet, int box_height, int box_width) {
    box_height -= 1;
    box_width -= 2; /* 양쪽 box 경계 제외 */

    switch(flag) {
        case('l'): // 오른쪽
            if (tet.x1 < box_width &&
                tet.x2 < box_width &&
                tet.x3 < box_width &&
                tet.x4 < box_width) {
                tet.x1++; tet.x2++; tet.x3++; tet.x4++;
            }
            break;
        case('h'): // 왼쪽 
            if (tet.x1 > 1 &&
                tet.x2 > 1 &&
                tet.x3 > 1 &&
                tet.x4 > 1) {
                tet.x1--; tet.x2--; tet.x3--; tet.x4--;
            }
            break;
        case('d'): { // 아래쪽
            if (tet.y1 < box_height &&
                tet.y2 < box_height &&
                tet.y3 < box_height &&
                tet.y4 < box_height) {
                tet.y1++; tet.y2++; tet.y3++; tet.y4++;
            }
            break;
        }
    }
    return tet;
}
