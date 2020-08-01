#include <stdio.h>
#include <stdlib.h>

#define CODE_SIZE 30000
#define BUFF_SIZE 30000

typedef unsigned char byte;

int main(int argc, char **argv) {
    FILE *fp;
    char code[CODE_SIZE];
    byte buff[BUFF_SIZE];
    int code_ptr = 0, code_len = 0, buff_ptr = 0, loop = 0;

    if (argc != 2) {
        printf("Error: must set a brainfuck program\n");
        exit(1);
    }

    if ((fp = fopen(argv[1], "r")) == NULL) {
        printf("file open error\n");
        exit(1);
    }

    for (; code_len < CODE_SIZE; code_len++) {
        char c = fgetc(fp);
        if (c == EOF)
            break;
        code[code_len] = c;
    }

    for (int i = 0; i < BUFF_SIZE; i++)
        buff[i] = 0;

    for(; code_ptr < code_len; code_ptr++) {
        switch(code[code_ptr]) {
        case '>':
            buff_ptr++;
            break;
        case '<':
            buff_ptr--;
            break;
        case '+':
            buff[buff_ptr]++;
            break; 
        case '-':
            buff[buff_ptr]--;
            break;
        case '.':
            putchar(buff[buff_ptr]);
            break;
        case ',':
            buff[buff_ptr] = getchar();
            break;
        case '[':
            if (buff[buff_ptr] == 0) {
                for (code_ptr++; loop > 0 || code[code_ptr] != ']'; code_ptr++) {
                    if (code[code_ptr] == '[')
                        loop++;
                    if (code[code_ptr] == ']')
                        loop--;
                }
            }
            break;
        case ']':
            if (buff[buff_ptr] != 0) {
                for (code_ptr--; loop > 0 || code[code_ptr] != '['; code_ptr--) {
                    if(code[code_ptr] == ']')
                        loop++;
                    if(code[code_ptr] == '[')
                        loop--;
                }
                code_ptr--;
            }
            break;
        }
    }

    fclose(fp);

    return 0;
}
