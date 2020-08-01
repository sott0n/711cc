#include <string.h>
#include <math.h>

#include "tetris.h"

enum piece_e {
    PIECE_I,
    PIECE_O,
    PIECE_L,
    PIECE_J,
    PIECE_T,
    PIECE_S,
    PIECE_Z,
    NUMBER_OF_PIECES,
};

enum rotation {
    ROT_NORMAL,
    ROT_LEFT,
    ROT_RIGHT,
    ROT_REVERSE,
};

typedef struct Coodinate Coodinate;
struct Coodinate {
    short x, y;
};

typedef struct Piece Piece;
struct Piece {
    enum piece_e p;
    enum rotation rot;
    Coodinate *pos;
};

extern const char tetris[NUMBER_OF_PIECES][4][4][4];

const char tetris[7][4][4][4] = {
    { /* I */
        {{1},{1},{1},{1}},
        {{1,1,1,1}},
        {{1},{1},{1},{1}},
        {{1,1,1,1}},
    },
    { /* O */
        {{1,1},{1,1},{0},{0}},
        {{1,1},{1,1},{0},{0}},
        {{1,1},{1,1},{0},{0}},
        {{1,1},{1,1},{0},{0}},
    },
    { /* L */
        {{1,1},{1},{1},{0}},
        {{1,1,1},{0,0,1},{0},{0}},
        {{0,1},{0,1},{1,1},{0}},
        {{1},{1,1,1},{0},{0}},
    },
    { /* J */
        {{1,1},{0,1},{0,1},{0}},
        {{0,0,1},{1,1,1},{0},{0}},
        {{1},{1},{1,1},{0}},
        {{1,1,1},{1},{0},{0}},
    },
    { /* T */
        {{0,1},{1,1,1},{0},{0}},
        {{1},{1,1},{1},{0}},
        {{1,1,1},{0,1},{0},{0}},
        {{0,1},{1,1},{0,1},{0}},
    },
    { /* S */
        {{0,1},{1,1},{1},{0}},
        {{1,1},{0,1,1},{0},{0}},
        {{0,1},{1,1},{1},{0}},
        {{1,1},{0,1,1},{0},{0}},
    },
    { /* Z */
        {{1},{1,1},{0,1},{0}},
        {{0,1,1},{1,1},{0},{0}},
        {{1},{1,1},{0,1},{0}},
        {{0,1,1},{1,1},{0},{0}},
    },
};

struct Piece *get_random_piece(void) {
    Piece *p = calloc(1, sizeof(Piece));
    p->p = rand() % NUMBER_OF_PIECES;
    p->rot = ROT_NORMAL; 
    Coodinate *c = calloc(1, sizeof(Coodinate));
    c->x = 4;
    c->y = 3;
    p->pos = *c;
    return p;
}

static struct Piece current_p;
static struct Piece next_p;

int delay;

static void add_current_piece(void) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (tetris[current_p.p][current_p.rot][y][x])
                screen[current_p.pos->y - y][current_p.pos->x + x] = current_p.p + 1;
        }
    }
}

static void remove_current_piece(void) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (tetris[current_p.p][current_p.rot][y][x])
                screen[current_p.pos->y - y][current_p.pos->x + x] = 0;
        }
    }
}

static int check_piece_overlap(void) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            if (tetris[current_p.p][current_p.rot][y][x]
                    && (current_p.pos->y - y >= Y
                        || current_p.pos->y - y < 0
                        || current_p.pos->x + x >= X
                        || current_p.pos->x + x < 0
                        || screen[current_p.pos->y - y][current_p.pos->x + x]))
                return 1;
        }
    }
    return 0;
}

void start_new_game(void) {
    memset(screen, 0, sizeof(screen));
    current_p = get_random_piece();
    next_p = get_random_piece();
    score = 0;
    level = 1;
    add_current_piece();
}

static int eliminate_line(void) {
    int lines_eliminated = 0;
    static const int points_per_line[] = {1, 40, 100, 300, 1200};

    int y, x, h, k;

    for (y = 0; y < Y; y++) {
        for (x = 0; x < X && screen[y][x]; x++);
        if (x != X)
            continue;
        for (h = y; h > 2; h--) {
            for (k = 0; k < X; k++)
                screen[h][k] = screen[h - 1][k];
        }

        lines_eliminated++;
    }

    return points_per_line[lines_eliminated];
}

static void handle_piece_bottom(void) {
    score += eliminate_line();
    level = 1 + score / 700;

    current_p = next_p;
    next_p = get_random_piece();

    if (check_piece_overlap()) {
        prompt_new_game();
        return;
    }

    add_current_piece();
}

static int do_move_down(void) {
    int bottom = 0;

    remove_current_piece();
    current_p.pos->y++;
    if (check_piece_overlap()) {
        bottom = 1;
        current_p.pos->y--;
    }
    add_current_piece();

    delay = 800 * pow(0.9, level);
    return bottom;
}

void move_down(void) {
    if (do_move_down())
        handle_piece_bottom();
}

void move_bottom(void) {
    while (!do_move_down())
        continue;
    handle_piece_bottom();
}

void move_left(void) {
    remove_current_piece();
    current_p.pos->x--;

    if (check_piece_overlap())
        current_p.pos->x++;

    add_current_piece();
}

void move_right(void) {
    remove_current_piece();
    current_p.pos->x++;

    if (check_piece_overlap())
        current_p.pos->x--;

    add_current_piece();
}

void rotate(void) {
    remove_current_piece();
    current_p.rot = (current_p.rot + 1) % 4;

    if (check_piece_overlap()) 
        current_p.rot = (current_p.rot - 1) % 4;

    add_current_piece();
}

void get_next_piece(char next[4][4]) {
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            next[y][x] = tetris[next_p.p][next_p.rot][y][x] ? next_p.p + 1 : 0;
        }
    }
}
