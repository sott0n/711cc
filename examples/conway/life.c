#include<stdio.h>
#include<stdlib.h>

#define WIDTH 40
#define HEIGHT 20
#define ACTIVE 1
#define INACTIVE 0
#define ACTIVE_SYMBOL '*'
#define INACTIVE_SYMBOL ' '
                
int count_neighbors(int board[HEIGHT][WIDTH], int x, int y) {
    int i, j, count = 0;
    
    for (i = -1; i <= 1; i++)
        for (j = -1; j <= 1; j++)
            if (!(i == 0 && j == 0) &&
                (x + i >= 0 && x + i < HEIGHT) &&
                (y + j >= 0 && y + j < WIDTH) &&
                (board[x + i][y + j] == ACTIVE))
                count++;
    return count;
}

int count_active(int board[HEIGHT][WIDTH]) {
    int i, j, count = 0;
    
    for (i = 0; i < HEIGHT; i++)
        for (j = 0; j < WIDTH; j++)
            if (board[i][j] == ACTIVE) count++;

    return count;
}

void iter_cell(int board[HEIGHT][WIDTH], int next[HEIGHT][WIDTH], int x, int y) {
    int current_state = board[x][y];
    int n_neighbors = count_neighbors(board, x, y);

    if (current_state == ACTIVE && n_neighbors < 2)
        next[x][y] = INACTIVE;
    else if (current_state == ACTIVE && n_neighbors > 3)
        next[x][y] = INACTIVE;
    else if (current_state == ACTIVE && (n_neighbors == 2 || n_neighbors == 3))
        next[x][y] = ACTIVE;
    else if (current_state == INACTIVE && n_neighbors == 3)
        next[x][y] = ACTIVE;
    else
        next[x][y] = INACTIVE;
}

void toggle(int board[HEIGHT][WIDTH], int x, int y) {
    if ((x > 0 && x <= WIDTH) && (y > 0 && y <= HEIGHT))
        board[HEIGHT - y][x - 1] = !(board[HEIGHT - y][x - 1]);
}

void init_board(int board[HEIGHT][WIDTH]) {
    int i, j;
    
    for (i = 0; i < HEIGHT; i++)
        for (j = 0; j < WIDTH; j++)
            board[i][j] = INACTIVE;
}

void copy_board(int source[HEIGHT][WIDTH], int destination[HEIGHT][WIDTH]) {
    int i, j;
    
    for (i = 0; i < HEIGHT; i++)
        for (j = 0; j < WIDTH; j++)
            destination[i][j] = source[i][j];
}

void next_iteration(int board[HEIGHT][WIDTH]) {
    int i, j;
    int next[HEIGHT][WIDTH];
    init_board(next);
    
    for (i = 0; i < HEIGHT; i++)
        for (j = 0; j < WIDTH; j++)
            iter_cell(board, next, i, j);
    copy_board(next, board);
}

void print_board(int board[HEIGHT][WIDTH]) {
    int i, j;
    
    for (i = 0; i < HEIGHT; i++) {
        for (j = 0; j < WIDTH; j++) {
            if (board[i][j] == INACTIVE)
                printf("%c ", INACTIVE_SYMBOL);
            else
                printf("%c ", ACTIVE_SYMBOL);
        }
        printf("\n");
    }
}

void system_clear() {
    system("clear");
}

void redraw(int board[HEIGHT][WIDTH], char *shell, int gen_count, int active_count) {
    system_clear();
    printf("\n%d generations; %d active cells (%dx%d board)\n", gen_count, 
                                                                active_count, 
                                                                WIDTH, HEIGHT);
    print_board(board);
    printf("%s: ", shell);
}

void show_help() {
    char *HELP = "\
This is a very simple version of the Conway's Game of Life.\n\
The command line is located at the bottom. There are four posible commands:\n\
    - n: run the (n)ext iteration. The Return key performs the same action.\n\
    - t: (t)oggle the state of a given cell. Further input will be needed for\n\
         the coordinates in the form x,y.\n\
    - i: display (i)nformation about a given cell\n\
    - h: display this (h)elp text.\n\
    - q: (q)uit the game.\n";

    system_clear();
    printf("%s", HELP);
    getchar();
}

void show_cell_info(int board[HEIGHT][WIDTH], int x, int y) {
    int internal_x = (HEIGHT - y), internal_y = (x- 1);
    
    system_clear();
    printf("-- Cell information --\n");
    printf("User coordinates:     %d,%d\n", x, y);
    printf("Internal coordinates: %d,%d\n", internal_x, internal_y);
    printf("State (1 is active):  %d\n", board[internal_x][internal_y]);
    printf("Active neighbors:     %d\n", count_neighbors(board, internal_x,
                                                         internal_y));
    getchar();
}

int main(int argc, char *argv[]) {
    int x = 0, y = 0;
    int gen_count = 0, active_count = 0;
    int board[HEIGHT][WIDTH];
    char command;
    char *filename;
    FILE *ifile;

    init_board(board);
    if (argc > 1) {
        filename = argv[1];
        ifile = fopen(filename, "r");
        if (ifile != NULL) {
            while (fscanf(ifile, "%d,%d", &x, &y) != EOF)
                toggle(board, x, y);
            fclose(ifile);
        }
    }
    active_count = count_active(board);
    do {    
        redraw(board, "", gen_count, active_count);
        command = getchar(); getchar();
        
        if (command == 'n' || command == '\n') {
            printf("\n");
            next_iteration(board);
            active_count = count_active(board);
            gen_count++;
        } else if (command == 't') {
            redraw(board, "x,y", gen_count, active_count);
            scanf("%d,%d", &x, &y); getchar();
            toggle(board, x, y);
        } else if (command == 'i') {
            redraw(board, "x,y", gen_count, active_count);
            scanf("%d,%d", &x, &y); getchar();
            show_cell_info(board, x, y);
        } else if (command == 'h') {
            show_help();   
        }
    } while (command != 'q');
    
    return 0;
}
