CFLAGS=-std=c11 -g -static -fno-common
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

711cc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): 711cc.h

test: 711cc
	./tests/test.sh

clean:
	rm -f 711cc *.o *~ tmp*

.PHONY: test clean
