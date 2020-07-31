#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

int nlines = 0;
int nwords = 0;
int nbytes = 0;

static void wc(FILE *in, char *filename) {
    char buf[4096];
    bool inword = false;
    int nl = 0;
    int nw = 0;
    int nb = 0;

    size_t nread;
    while ((nread = fread(buf, 1, sizeof(buf), in)) > 0) {
        nb += nread;

        for (size_t i = 0; i < nread; i++) {
            char c = buf[i];
            if (c == '\n')
                nl++;
            
            if (inword && !isalpha(c)) {
                inword = false;
            } else if (!inword && isalpha(c)) {
                inword = true;
                nw++;
            }
        }
    }
    
    if (filename)
        printf("% 8d% 8d% 8d %s\n", nl, nw, nb, filename);
    else
        printf("% 8d% 8d% 8d\n", nl, nw, nb);

    nlines += nl;
    nwords += nw;
    nbytes += nb;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        wc(stdin, NULL);
        exit(0);
    }

    for (char **p = argv + 1; *p; p++) {
        FILE *in = fopen(*p, "r");
        if (!in) {
            perror("fopen");
            exit(1);
        }
        wc(in, *p);
    }

    if (argc > 2)
        printf("% 8d% 8d% 8d total\n", nlines, nwords, nbytes);
    return 0;
}
