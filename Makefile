CFLAGS=-std=c11 -g -fno-common
SRCROOT=./src
SRCDIRS:=$(shell find $(SRCROOT) -type d)
SRCS=$(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.c))
OBJS=$(SRCS:.c=.o)

711cc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): $(SRCROOT)/711cc.h

711cc-stage2: 711cc $(SRCS) $(SRCROOT)/711cc.h scripts/self.sh
	./scripts/self.sh tmp-stage2 ./711cc 711cc-stage2

711cc-stage3: 711cc-stage2
	./scripts/self.sh tmp-stage3 ./711cc-stage2 711cc-stage3

test: 711cc
	(cd tests; ../711cc -I. -o- tests.c) > tmp.s
	gcc -o tmp tmp.s tests/extern.o
	./tmp

test-nopic: 711cc tests/extern.o
	(cd tests; ../711cc -I. -o- -fno-pic tests.c) > tmp.s
	gcc -static -o tmp tmp.s tests/extern.o
	./tmp

test-stage2: 711cc-stage2 tests/extern.o
	(cd tests; ../711cc-stage2 -I. tests.c) > tmp.s
	gcc -static -o tmp tmp.s tests/extern.o
	./tmp

test-stage3: 711cc-stage3
	diff 711cc-stage2 711cc-stage3

test-all: test test-nopic test-stage2 test-stage3

clean:
	rm -rf 711cc 711cc-stage* $(SRCROOT)/*.o *~ tmp* tests/*~ tests/*.o

.PHONY: test clean
