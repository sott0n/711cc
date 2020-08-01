## Conway's game of life.

### Build by 711cc C Compiler

```
$ ../../711cc -o life.o life.c
$ gcc -o life life.o
```

### Run a game
```
$ ./life pulsar
or
$ ./life glidergun
```

### Commands

- n: run the (n)ext iteration. The Return key performs the same action.
- t: (t)oggle the state of a given cell. Further input will be needed for the coordinates in the form x,y.
- i: display (i)nformation about a given cell
- h: display this (h)elp text.
- q: (q)uit the game.
