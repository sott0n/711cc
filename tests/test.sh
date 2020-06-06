#!/bin/bash

assert() {
    expected="$1";
    input="$2";

    ./711cc "$input" > tmp.s || exit
    gcc -static -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 0
assert 52 52
assert 21 '5+20-4'

echo OK
