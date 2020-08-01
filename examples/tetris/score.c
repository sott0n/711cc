#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "tetris.h"

int high_score;
int score;
int level;

static char score_filename[1024];

static void load_score(void) {
    FILE *score_f;

    if ((score_f = fopen(score_filename, "r"))) {
        if (fscanf(score_f, "%d", &high_score) != 1)
            high_score = 0;
        fclose(score_f);
    } else {
        if (errno != ENOENT)
            perror("Failed open a file for reading high score");
    }
}

static void save_score(void) {
    FILE *score_fp;

    if ((score_fp = fopen(score_filename, "w"))) {
        fprintf(score_fp, "%d\n", high_score);
        fclose(score_fp);
    } else {
        perror("Failed open a file for writing high score");
    }
}

void init_score(void) {
    snprintf(score_filename, sizeof(score_filename), "%s/.tetoris_score", getenv("HOME"));
    load_score();
    atexit(save_score);
}
