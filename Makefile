CFLAGS=-std=c11 -g -static -fno-common

711cc: main.o
	$(CC) -o $@ $? $(LDFLAGS)

test: 711cc
	./tests/test.sh

clean:
	rm -f 711cc *.o *~ tmp*

.PHONY: test clean
