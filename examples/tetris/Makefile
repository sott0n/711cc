CFLAGS=-O2 -Wall -Wextra -pedantic -Wno-vla -std=c11
SRCS=tetris.c screen.c score.c
HEADS=tetris.h
OBJS=$(SRCS:.c=.o)
LDFLAGS=-lncurses -lm

tetris: $(SRCS) $(HEADS)
	../../711cc -o tetris.o -I. tetris.c
	../../711cc -o screen.o -I. screen.c
	../../711cc -o score.o -I. score.c
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

clean:
	rm -rf tetris *.o

.PHONY: test clean
