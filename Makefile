CFLAGS=-std=c11 -g -static -fno-common
SRCROOT=./src
SRCDIRS:=$(shell find $(SRCROOT) -type d)
SRCS=$(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))
OBJS=$(SRCS:.c=.o)

711cc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(SRCROOT)/711cc.h

test: 711cc tests/extern.c
	./711cc -o tmp.s tests/tests.c
	gcc -static -o tmp tmp.s
	./tmp

clean:
	rm -rf 711cc *.o *~ tmp* tests/*~ tests/*.o

.PHONY: test clean
