# 711cc
A small c compiler named 711cc.

## Feature

- Target architecture is x86_64
- AT&T syntax
- Support multibyte UTF-8 character in identifier
- Goal is a self-hosting compile

## Build

Run make to build:

```shell
$ make
```

## Usage
You can compile a `.c` source to an object file with args `-o` and output file path:

```shell
# Compile .c source code to tmp.o object file
$ ./711cc -o tmp.o [file-path].c

# Create an executable file using gcc
$ gcc -o tmp tmp.o

# Execute
$ ./tmp
```

## Support Options

General options below:  

| Option | Detail |
| ------------- | ------------- |
| --help | Show a help text |
| -S | Outputs as assembly |
| -E | Show preprocessed tokens |
| -I [path] | Add include path |
| -D [Macro] | Set an origin macro |

Instead of outputting the result of preprocessing, output a rule suitable for make describing the dependencies of the main source file. The preprocessor outputs one make rule containing the object file name for that source file, a colon, and the names of all the included files.

| Option | Detail |
| ------------- | ------------- |
| -M | Show a list of include path of the main file |
| -MD | Show a list of include path, except that `-E` is not implied |
| -MP | Add a phony target for each dependency other than the main file |
| -MT [target] | Change the target for `-M` |
| -MF [file] | When used with `-M`, specifies a file to write the dependencies |


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

## Future works

- Wasm support in backend
- RISC-V support in backend
- LLVM IR support in backend
- Add optimization passes
- UTF-8 support
