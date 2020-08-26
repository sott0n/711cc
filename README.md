# 711cc C Compiler
[![C/C++ CI](https://github.com/sott0n/711cc/workflows/C/C++%20CI/badge.svg?branch=master)](https://github.com/sott0n/711cc/actions?query=workflow%3A%22C%2FC%2B%2B+CI%22) ![x86-64/riscv-64](https://img.shields.io/badge/Feature-x86--64%2Friscv--64-orange) [![](http://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)

A small C compiler named 711cc as toy programming.

## Feature

- Target architecture are x86_64(AT&T syntax) and RISC-V(**Working**)
- The parser is a hand-written recursive descendent parser
- Support the preprocesser for macro
- Support multibyte UTF-8 character in identifier
- This compiler is able to compiler itself (self-hosting)

## Status
- For x86-64, This compiler could compile [all example programs](https://github.com/sott0n/711cc/tree/master/examples)
- For RISC-V, Not yet supported variadic
- For RISC-V, Not yet supported floating points

This project started for fun and understanding C-Compiler deeply.

## Build

Run make to build this compiler:

```shell
$ make
```

For self-hosting, add args to build:

```shell
# 711cc compile self code
$ make 711cc-stage2

# Stage2 compiler compile self code as stage3
$ make 711cc-stage3
```

## Usage
You can compile a `.c` source to an object file with args `-o` and output file path:

```shell
# Compile .c source code to tmp.o object file
$ ./711cc -o tmp.o [file].c

# Create an executable file using gcc
$ gcc -o tmp tmp.o

# Execute
$ ./tmp
```

## Support Options

General options below:  

| Option             | Detail                               |
| ------------------ | ------------------------------------ |
| --help             | Show a help text                     |
| --feature=[target] | Target architecture, x86-64/riscv-64 |
| -S                 | Outputs as assembly                  |
| -E                 | Show preprocessed tokens             |
| -I[path]           | Add include path                     |
| -D[Macro]          | Set an origin macro                  |

Instead of outputting the result of preprocessing, output a rule suitable for make describing the dependencies of the main source file. The preprocessor outputs one make rule containing the object file name for that source file, a colon, and the names of all the included files.

| Option      | Detail |
| ----------- | --------------------------------------------------------------- |
| -M          | Show a list of include path of the main file                    |
| -MD         | Show a list of include path, except that `-E` is not implied    |
| -MP         | Add a phony target for each dependency other than the main file |
| -MT[target] | Change the target for `-M`                                      |
| -MF[file]   | When used with `-M`, specifies a file to write the dependencies |


There are two args, `-fpic` and `-fno-pic` to select ways of computing a variable. The `-fpic` is a default setting in this compiler, so it means that `-fno-pic` isn't given. If `-fno-pic` is given, the ELF module doesn't have to be position-independent, meaning compiler assume that code and data will be loaded at a fixed memory location below 4GiB. If `-fno-pic` is not given, the ELF module may be loaded anywhere in the 64-bit address space.

```shell
# Add `-fpic` or `-fPIC`
$ ./711cc -o tmp.s -fpic [file].c
$ gcc -o tmp tmp.s
$ ./tmp

# Add `-fno-pic` or `-fno-PIC`
$ ./711cc -o tmp.s -fno-pic [file].c
$ gcc -static -o tmp tmp.s
$ ./tmp
```

## Test

711cc comes with tests. To run the tests, give "test" as an argument:

```shell
# Test for x86-64
$ make test

# Test for riscv-64
$ make test-riscv
```

For test a self hosting, give "test-stage2" and "test-stage3" as an argument:

```shell
# Test for self hosting with a test
$ make test-stage2

# Check a diff between stage2 compiler and stage3 compiler
$ make test-stage3
```

## Example
Compile a program of `cat` UNIX command:
```
root@a9a608a739a4:/home# make
cc -std=c11 -g -fno-common -Wall -Wno-switch   -c -o src/tokenize.o src/tokenize.c
cc -std=c11 -g -fno-common -Wall -Wno-switch   -c -o src/type.o src/type.c
cc -std=c11 -g -fno-common -Wall -Wno-switch   -c -o src/main.o src/main.c
cc -std=c11 -g -fno-common -Wall -Wno-switch   -c -o src/parse.o src/parse.c
cc -std=c11 -g -fno-common -Wall -Wno-switch   -c -o src/codegen.o src/codegen.c
cc -std=c11 -g -fno-common -Wall -Wno-switch   -c -o src/preprocess.o src/preprocess.c
cc -o 711cc ./src/tokenize.o ./src/type.o ./src/main.o ./src/parse.o ./src/codegen.o ./src/preprocess.o
root@a9a608a739a4:/home# ./711cc -o examples/cat.o examples/cat.c
root@a9a608a739a4:/home# gcc -o tmp examples/cat.o
root@a9a608a739a4:/home# ./tmp examples/fib.c
#include <stdio.h>

int fib(int n) {
     if (n == 0) return 0;
     if (n == 1) return 1;
     return fib(n - 1) + fib(n - 2);
}

int main() {
    int c = 20;
    for (int i = 0; i < c; i++)
        printf("fib(%d): %d\n", i, fib(i));
}
```

Please compile other programs in [examples directory](https://github.com/sott0n/711cc/tree/master/examples).

## Future works

- Wasm support in backend
- RISC-V support in backend
- LLVM IR support in backend
- Add optimization passes
