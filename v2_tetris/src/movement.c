#include "../include/tetrimino.h"

Tetrimino rotate_tet(Tetrimino tet, int box_height, int box_width) {
    Tetrimino tmp = tet;
    Tetrimino rot = tet;

    rot.x1 -= tet.x2;
    rot.x2 -= tet.x2;
    rot.x3 -= tet.x2;
    rot.x4 -= tet.x2;
    rot.y1 -= tet.y2;
    rot.y2 -= tet.y2;
    rot.y3 -= tet.y2;
    rot.y4 -= tet.y2;

    tmp.x1 = -rot.y1;
    tmp.x2 = -rot.y2;
    tmp.x3 = -rot.y3;
    tmp.x4 = -rot.y4;
    tmp.y1 = rot.x1;
    tmp.y2 = rot.x2;
    tmp.y3 = rot.x3;
    tmp.y4 = rot.x4;

    tmp.x1 += tet.x2;
    tmp.x2 += tet.x2;
    tmp.x3 += tet.x2;
    tmp.x4 += tet.x2;
    tmp.y1 += tet.y2;
    tmp.y2 += tet.y2;
    tmp.y3 += tet.y2;
    tmp.y4 += tet.y2;
    if (tmp.x1 >= 0 && tmp.x1 < box_width &&
        tmp.y1 >= 0 && tmp.y1 < box_height &&
        tmp.x2 >= 0 && tmp.x2 < box_width &&
        tmp.y2 >= 0 && tmp.y2 < box_height &&
        tmp.x3 >= 0 && tmp.x3 < box_width &&
        tmp.y3 >= 0 && tmp.y3 < box_height &&
        tmp.x4 >= 0 && tmp.x4 < box_width &&
        tmp.y4 >= 0 && tmp.y4 < box_height) {
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
