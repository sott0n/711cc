CFLAGS=-std=c11 -g -fno-common -Wall -Wno-switch
RISCV_CC?=riscv64-linux-gnu-gcc
RISCV_CFLAGS?=
RISCV_QEMU?=qemu-riscv64
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

check-riscv-tools:
	@command -v "$(RISCV_CC)" >/dev/null 2>&1 || { \
		echo "missing RISC-V cross compiler: $(RISCV_CC)" >&2; \
		echo "install gcc-riscv64-linux-gnu or set RISCV_CC" >&2; \
		exit 1; \
	}
	@command -v "$(RISCV_QEMU)" >/dev/null 2>&1 || { \
		echo "missing RISC-V emulator: $(RISCV_QEMU)" >&2; \
		echo "install qemu-user or set RISCV_QEMU" >&2; \
		exit 1; \
	}

test-riscv: 711cc check-riscv-tools
	(cd tests; ../711cc --feature=riscv64 -I. -S -c -o ../tmp.s -DANSWER=42 tests_riscv.c)
	$(RISCV_CC) $(RISCV_CFLAGS) -static -o tmp tmp.s tests/extern.c
	$(RISCV_QEMU) ./tmp

test-all: test test-nopic test-stage2 test-stage3 test-riscv

compat-git: 711cc
	./tests/compat/git/run.sh "$(GIT_SOURCE_DIR)"

clean:
	rm -rf 711cc 711cc-stage* $(SRCROOT)/*.o *~ tmp* tests/*~ tests/*.o examples/*.o examples/tmp*

.PHONY: test test-riscv check-riscv-tools compat-git clean
