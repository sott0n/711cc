#!/bin/bash
set -e

TMP=$1
CC=$2
OUTPUT=$3

rm -rf $TMP
mkdir -p $TMP

711cc() {
    $CC -c -o $TMP/${1%.c}.o -Isrc src/$1
}

cc() {
    gcc -c -o $TMP/${1%.c}.o src/$1
}

711cc main.c
711cc type.c
711cc parse.c
711cc codegen.c
711cc codegen_riscv.c
711cc tokenize.c
711cc preprocess.c

(cd $TMP; gcc -o ../$OUTPUT *.o)
