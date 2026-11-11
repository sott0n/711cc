static int add2(int a, int b) {
    return a + b;
}

static long mix8(char a, unsigned char b, short c, unsigned short d,
                 int e, unsigned int f, long g, unsigned long h) {
    return a + b + c + d + e + f + g + h;
}

static int control_flow(int limit) {
    int sum = 0;
    for (int i = 0; i < limit; i += 1) {
        if (i == 2)
            continue;
        if (i == 6)
            break;
        sum += i;
    }

    int j = 0;
    do {
        j += 1;
    } while (j < 3);
    sum += j;

    while (j > 0) {
        j -= 1;
        if (j == 1)
            continue;
        sum += j;
    }

    return (sum == 18 && limit > 6) ? sum : 0;
}

static int nested_flow(void) {
    int sum = 0;
    for (int i = 0; i < 3; i += 1) {
        for (int j = 0; j < 4; j += 1) {
            if (j == 1)
                continue;
            if (i == 2)
                break;
            sum += i + j;
        }
    }
    return sum;
}

int main(void) {
    signed char c = -3;
    unsigned char uc = 250;
    short s = -1234;
    unsigned short us = 60000;
    int x = 40;
    unsigned int ux = 4000000000U;
    long y = -9;
    int *p = &x;
    int (*fn)(int, int) = add2;

    x += 2;
    *p = *p + 1;
    y = (x = x - 1) + y;

    return (x != 42) +
           (y != 33) +
           (c != -3) +
           (uc != 250) +
           (s != -1234) +
           (us != 60000) +
           (ux != 4000000000U) +
           (add2(20, 22) != 42) +
           (fn(19, 23) != 42) +
           (add2(add2(10, 11), add2(9, 12)) != 42) +
           (mix8(-1, 2, -3, 4, -5, 6, -7, 46) != 42) +
           (control_flow(10) != 18) +
           (control_flow(5) != 0) +
           (nested_flow() != 13) +
           (!(0 || (1 && 2))) +
           (6 * 7 != 42) +
           (17 % 5 != 2) +
           (9 / 3 != 3) +
           ((1 << 5) != 32) +
           ((8 >> 2) != 2) +
           ((15 & 6) != 6) +
           ((8 | 1) != 9) +
           ((7 ^ 3) != 4) +
           (-8 / 2 != -4) +
           (!(3 < 4));
}
