CFLAGS=-std=c11 -g -fno-common -Wall -Wno-switch
RISCV_CC?=riscv64-linux-gnu-gcc
RISCV_CFLAGS?=
RISCV_QEMU?=qemu-riscv64
AARCH64_CC?=aarch64-linux-gnu-gcc
AARCH64_CFLAGS?=
AARCH64_QEMU?=qemu-aarch64
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
	(cd tests; ../711cc -std=c11 -fno-strict-overflow -fvisibility=hidden -funroll-loops -ftree-vectorize -pthread -I. -c -o ../tmp.o -DANSWER=42 tests.c)
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

check-aarch64-tools:
	@command -v "$(AARCH64_CC)" >/dev/null 2>&1 || { \
		echo "missing AArch64 cross compiler: $(AARCH64_CC)" >&2; \
		echo "install gcc-aarch64-linux-gnu or set AARCH64_CC" >&2; \
		exit 1; \
	}
	@command -v "$(AARCH64_QEMU)" >/dev/null 2>&1 || { \
		echo "missing AArch64 emulator: $(AARCH64_QEMU)" >&2; \
		echo "install qemu-user or set AARCH64_QEMU" >&2; \
		exit 1; \
	}

test-aarch64-smoke: 711cc check-aarch64-tools
	(cd tests; ../711cc --feature=aarch64 -I. -S -c -o ../tmp-aarch64.s aarch64_smoke.c)
	$(AARCH64_CC) $(AARCH64_CFLAGS) -static -o tmp-aarch64 tmp-aarch64.s
	$(AARCH64_QEMU) ./tmp-aarch64

test-all: test test-nopic test-stage2 test-stage3 test-riscv test-aarch64-smoke

compat-git: 711cc
	./tests/compat/git/run.sh "$(GIT_SOURCE_DIR)"

compat-cpython: 711cc
	./tests/compat/cpython/run.sh "$(CPYTHON_SOURCE_DIR)"

compat-postgresql: 711cc
	./tests/compat/postgresql/run.sh "$(POSTGRESQL_SOURCE_DIR)"

clean:
	rm -rf 711cc 711cc-stage* $(SRCROOT)/*.o *~ tmp* tests/*~ tests/*.o examples/*.o examples/tmp*

.PHONY: test test-riscv check-riscv-tools test-aarch64-smoke check-aarch64-tools compat-git compat-cpython compat-postgresql clean
