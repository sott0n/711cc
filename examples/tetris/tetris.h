#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <curses.h>
#include <stdbool.h>
#include <time.h>

//
// score.c
//
extern int high_score;
extern int score;
extern int level;

void init_score(void);

//
// game.c
//
struct Piece *get_random_piece(void);
extern int delay;

void move_down(void);
void move_bottom(void);
void move_left(void);
void move_right(void);
void rotate(void);
void start_new_game(void);
void get_next_piece(char next[4][4]);

//
// screen.c
//
enum {
    Y = 22,
    X = 10,
};

extern char screen[Y][X];
void prompt_new_game(void);
