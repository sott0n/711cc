#!/bin/bash
set -e

TMP=$1
CC=$2
OUTPUT=$3

rm -rf $TMP
mkdir -p $TMP

711cc() {
    (cd $TMP; ../$CC -o ${1%.c}.s -I../src ../src/$1)
    gcc -c -o $TMP/${1%.c}.o $TMP/${1%.c}.s
}

cc() {
    gcc -c -o $TMP/${1%.c}.c $1
}

711cc main.c
711cc type.c
711cc parse.c
711cc codegen.c
711cc tokenize.c
711cc preprocess.c

(cd $TMP; gcc -o ../$OUTPUT *.o)
