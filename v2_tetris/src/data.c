#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "../include/tetrimino.h"
#define MAX(a, b) (((a) > (b)) ? (a) : (b)) 

prev_score_t load_data(); 

void save_data(prev_score_t data) {
    prev_score_t prev = load_data();
    prev_score_t tmp;
    
    tmp.max_line = MAX(prev.max_line, data.max_line);
    tmp.max_score = MAX(prev.max_score, data.max_score);
    tmp.max_level = MAX(prev.max_level, data.max_level);

    FILE *fp = fopen(".score.dat", "wb");
    if (fp == NULL) {
        perror("fopne error");
        return;
    }

    fwrite(&tmp, sizeof(prev_score_t), 1, fp);

    fclose(fp);
}

prev_score_t load_data() {
    prev_score_t data;
    FILE *fp = fopen(".score.dat", "rb");

    if (fp == NULL) {
        /* if there's no file */ 
        data.max_line = 0;
        data.max_score = 0;
        data.max_level = 0;
        return data;
    }
    fread(&data, sizeof(prev_score_t), 1, fp);

    fclose(fp);
    return data;
}
