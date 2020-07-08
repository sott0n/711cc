#include <stdarg.h>
#include <stdio.h>

int ext1;
int *ext2;
int ext3 = 5;

int char_fn() { return 257; }
int static_fn() { return 5; }

int false_fn() { return 512; }
int true_fn() { return 513; }

int add_all1(int x, ...) {
    va_list ap;
    va_start(ap, x);

    for (;;) {
        int y = va_arg(ap, int);
        if (y == 0)
            return x;
        x += y;
    }
}

int add_all3(int x, int y, int z, ...) {
    va_list ap;
    va_start(ap, z);
    x = x + y + z;

    for (;;) {
        int y = va_arg(ap, int);
        if (y == 0)
            return x;
        x += y;
    }
}

float ret_float(float x) {
    return x;
}

float add_float(float x, float y) {
    return x + y;
}

double add_double(double x, double y) {
    return x + y;
}
