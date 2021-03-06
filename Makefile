CFLAGS=-std=c11 -g -fno-common -Wall -Wno-switch
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
	(cd tests; ../711cc -I. -c -o ../tmp.o -DANSWER=42 tests.c)
	$(CC) -o tmp tmp.o tests/extern.c
	./tmp

test-nopic: 711cc tests/extern.o
	(cd tests; ../711cc -I. -c -o ../tmp.o -DANSWER=42 -fno-pic tests.c)
	$(CC) -static -o tmp tmp.o tests/extern.c
	./tmp

test-stage2: 711cc-stage2 tests/extern.o
	(cd tests; ../711cc-stage2 -I. -c -o ../tmp.o -DANSWER=42 tests.c)
	$(CC) -static -o tmp tmp.o tests/extern.c
	./tmp

test-stage3: 711cc-stage3
	diff 711cc-stage2 711cc-stage3

test-riscv: 711cc
	(cd tests; ../711cc --feature=riscv64 -I. -S -c -o ../tmp.s -DANSWER=42 tests_riscv.c)
	riscv64-linux-gnu-gcc -static -o tmp tmp.s tests/extern.c
	qemu-riscv64 ./tmp

test-all: test test-nopic test-stage2 test-stage3 test-riscv

clean:
	rm -rf 711cc 711cc-stage* $(SRCROOT)/*.o *~ tmp* tests/*~ tests/*.o examples/*.o examples/tmp*

.PHONY: test clean
