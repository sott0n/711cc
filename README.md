# 711cc
A small c compiler named 711cc.

## Feature

- Target architecture is x86_64
- AT&T syntax
- Goal is a self-hosting compiler

## Build

Run make to build:

```shell
$ make
```

## Usage
You can compile a `.c` source to an assembly file with args `-o` and output file path:

```shell
# Compile .c source code to tmp.s assembly file
$ ./711cc -o tmp.s [file-path].c

# Link and make a exectable file by gcc
$ gcc -o tmp tmp.s

# Execute
$ ./tmp
```

If `-E` is given, you can see preprocessed tokens:

```shell
$ ./711cc -E [file-path].c
```

There are two args, `-fpic` and `-fno-pic` to select ways of computing a variable. The `-fpic` is a default setting in this compiler, so it means that `-fno-pic` isn't given. If `-fno-pic` is given, the ELF module doesn't have to be position-independent, meaning compiler assume that code and data will be loaded at a fixed memory location below 4GiB. If `-fno-pic` is not given, the ELF module may be loaded anywhere in the 64-bit address space.

```shell
# Add `-fpic` or `-fPIC`
$ ./711cc -o tmp.s -fpic [file-path].c
$ gcc -o tmp tmp.s
$ ./tmp

# Add `-fno-pic` or `-fno-PIC`
$ ./711cc -o tmp.s -fno-pic [file-path].c
$ gcc -static -o tmp tmp.s
$ ./tmp
```

## Test

711cc comes with tests. To run the tests, give "test" as an argument:

```shell
$ make test
```

For test a self hosting, give "test-stage2" and "test-stage3" as an argument:

```shell
# Test for self hosting with a test
$ make test-stage2

# Check a diff between stage2 compiler and stage3 compiler
$ make test-stage3
```
