#include "../include/tetrimino.h"
#include <stdlib.h>

Tetrimino make_tet(int x, int y, Tetrimino tet) {
    Tetrimino new_tet;
    new_tet.name = tet.name;
    new_tet.x1 = x + tet.x1;
    new_tet.y1 = y + tet.y1;
    new_tet.x2 = x + tet.x2;
    new_tet.y2 = y + tet.y2;
    new_tet.x3 = x + tet.x3;
    new_tet.y3 = y + tet.y3;
    new_tet.x4 = x + tet.x4;
    new_tet.y4 = y + tet.y4;
    return new_tet;
}

Tetrimino *generate_tets() {
    Tetrimino *arr = malloc(sizeof(Tetrimino) * 7);

    Tetrimino I, L, J, O, S, T, Z;
    /* 0 */
    I.name = 'I';
    I.x1 = 0; I.x2 = 0; I.x3 = 0; I.x4 = 0;
    I.y1 = 0; I.y2 = 1; I.y3 = 2; I.y4 = 3;

    /* 1 */
    L.name = 'L';
    L.x1 = 0; L.x2 = 0; L.x3 = 0; L.x4 = 1;
    L.y1 = 0; L.y2 = 1; L.y3 = 2; L.y4 = 2;

    /* 2 */
    J.name = 'J';
    J.x1 = 0; J.x2 = 0; J.x3 = 0; J.x4 = -1;
    J.y1 = 0; J.y2 = 1; J.y3 = 2; J.y4 = 2;

    /* 3 */
    O.name = 'O';
    O.x1 = 0; O.x2 = 1; O.x3 = 1; O.x4 = 0;
    O.y1 = 0; O.y2 = 0; O.y3 = 1; O.y4 = 1;

    /* 4 */
    S.name = 'S';
    S.x1 = 1; S.x2 = 2; S.x3 = 0; S.x4 = 1;
    S.y1 = 0; S.y2 = 0; S.y3 = 1; S.y4 = 1;

    /* 5 */
    T.name = 'T';
    T.x1 = 0; T.x2 = 0; T.x3 = -1; T.x4 = 1;
    T.y1 = 0; T.y2 = 1; T.y3 = 1; T.y4 = 1;

    /* 6 */
    Z.name = 'Z';
    Z.x1 = 0; Z.x2 = 1; Z.x3 = 1; Z.x4 = 1;
    Z.y1 = 0; Z.y2 = 0; Z.y3 = 1; Z.y4 = 2;

    // Tetrimino tets[] = {I, L, J, O, S, T, Z};    
    
    arr[0] = I;
    arr[1] = L;
    arr[2] = J;
    arr[3] = O;
    arr[4] = S;
    arr[5] = T;
    arr[6] = Z;

    return arr;
}

char** generate_grid(int box_height, int box_width) {
    /*
     * -1: 벽, 0: 블럭 없음, 1: 블럭 있음
     */
    char **grid = malloc(box_height * sizeof(char*));
    
    for (int i = 0; i < box_height; i++) {
        grid[i] = malloc(box_width * sizeof(char));
    }
    
    for (int i = 0; i < box_height; i++) {
        for (int j = 0; j < box_width; j++) {
            grid[i][j] = 0; // 블럭 없음으로 초기화 
        }
    }
    
    for (int j = 0; j < box_width; j++) {
        grid[box_height-1][j] = -1;
    }

    for (int k = 0; k < box_height; k++) {
        grid[k][0] = -1;
        grid[k][box_height-1] = -1;
    }

    return grid;
}
