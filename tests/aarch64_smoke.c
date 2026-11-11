int main(void) {
    signed char c = -3;
    unsigned char uc = 250;
    short s = -1234;
    unsigned short us = 60000;
    int x = 40;
    unsigned int ux = 4000000000U;
    long y = -9;
    int *p = &x;

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
