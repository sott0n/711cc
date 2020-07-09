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

## Test

711cc comes with tests. To run the tests, give "test" as an argument:

```shell
$ make test
```

For test a self hosting, give "test-stage2" and "test-stage3" as an argument:

```shell
// Test for self hosting with a test
$ make test-stage2

// Check a diff between stage2 compiler and stage3 compiler
$ make test-stage3
```
