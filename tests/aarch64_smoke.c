struct Pair {
    int left;
    long right;
};

struct SmallPair {
    int left;
    int right;
};

struct Bits {
    unsigned int first : 3;
    signed int second : 5;
    unsigned int third : 8;
};

struct Wide {
    long values[3];
    char tag;
};

static int global_value = 5;
static int global_zero;
static int global_values[3] = {2, 4, 6};
static struct Pair global_pair = {7, 8};
static struct Bits global_bits = {5, -3, 200};

long external_sum10(long, long, long, long, long,
                    long, long, long, long, long);
long external_call_sum10(long (*)(long, long, long, long, long,
                                  long, long, long, long, long));
long external_pair_sum(int, struct Pair, int);
long external_small_pair_sum(int, struct SmallPair, int);
long external_call_pair(long (*)(int, struct Pair, int));
long external_stacked_pair_sum(long, long, long, long, long, long, long,
                               struct Pair, long);
long external_call_stacked_pair(long (*)(long, long, long, long, long, long,
                                          long, struct Pair, long));

static int add2(int a, int b) {
    return a + b;
}

static long mix8(char a, unsigned char b, short c, unsigned short d,
                 int e, unsigned int f, long g, unsigned long h) {
    return a + b + c + d + e + f + g + h;
}

static long sum10(long a, long b, long c, long d, long e,
                  long f, long g, long h, long i, long j) {
    return a + b + c + d + e + f + g + h + i + j;
}

static long narrow_stack_args(long a, long b, long c, long d, long e,
                              long f, long g, long h, char i,
                              unsigned short j) {
    return a + b + c + d + e + f + g + h + i + j;
}

static long pair_sum(int prefix, struct Pair pair, int suffix) {
    return prefix + pair.left + pair.right + suffix;
}

static long small_pair_sum(int prefix, struct SmallPair pair, int suffix) {
    return prefix + pair.left + pair.right + suffix;
}

static long stacked_pair_sum(long a, long b, long c, long d, long e,
                             long f, long g, struct Pair pair, long tail) {
    return a + b + c + d + e + f + g + pair.left + pair.right + tail;
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

static int select_value(int value) {
    int result = 0;
    switch (value) {
    case -1:
        result = 10;
        break;
    case 2:
        result = 20;
    case 3:
        result += 3;
        break;
    default:
        result = 40;
        break;
    }
    return result;
}

static int goto_flow(void) {
    int result = 0;
    goto second;
first:
    result += 2;
    goto done;
second:
    result += 40;
    goto first;
done:
    return result;
}

static int switch_in_loop(void) {
    int sum = 0;
    for (int i = 0; i < 4; i += 1) {
        switch (i) {
        case 1:
            continue;
        case 2:
            break;
        default:
            break;
        }
        sum += i;
    }
    return sum;
}

static int computed_goto_flow(void) {
    void *target = &&matched;
    goto *target;
    return 0;
matched:
    return 42;
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
    int values[3] = {10, 20, 30};
    struct Pair pair = {11, 31};
    struct SmallPair small_pair = {20, 21};
    struct Pair *pair_ptr = &pair;
    struct Pair pair_copy = pair;
    struct Pair pair_assigned = {0, 0};
    struct Pair pair_indirect = {0, 0};
    struct Pair *pair_indirect_ptr = &pair_indirect;
    struct Bits bits = {1, -2, 3};
    struct Wide wide = {{10, 20, 30}, 40};
    struct Wide wide_copy = wide;

    x += 2;
    *p = *p + 1;
    y = (x = x - 1) + y;
    values[1] += values[0];
    pair.left = pair.left + 1;
    pair_ptr->right += 1;
    pair_assigned = pair;
    *pair_indirect_ptr = pair_assigned;
    wide.values[1] = 99;
    global_zero = 9;
    global_values[1] += global_value;
    bits.first = 7;
    bits.second = -7;
    bits.third = 250;

    return (x != 42) +
           (y != 33) +
           (c != -3) +
           (uc != 250) +
           (s != -1234) +
           (us != 60000) +
           (ux != 4000000000U) +
           (values[0] != 10) +
           (values[1] != 30) +
           (values[2] != 30) +
           (pair.left != 12) +
           (pair.right != 32) +
           (pair_ptr->left != 12) +
           (pair_copy.left != 11) +
           (pair_copy.right != 31) +
           ((pair_assigned = pair).right != 32) +
           (pair_indirect.left != 12) +
           (pair_indirect.right != 32) +
           (wide.values[1] != 99) +
           (wide_copy.values[0] != 10) +
           (wide_copy.values[1] != 20) +
           (wide_copy.values[2] != 30) +
           (wide_copy.tag != 40) +
           (global_zero != 9) +
           (global_values[0] != 2) +
           (global_values[1] != 9) +
           (global_pair.left != 7) +
           (global_pair.right != 8) +
           (bits.first != 7) +
           (bits.second != -7) +
           (bits.third != 250) +
           (global_bits.first != 5) +
           (global_bits.second != -3) +
           (global_bits.third != 200) +
           (add2(20, 22) != 42) +
           (fn(19, 23) != 42) +
           (add2(add2(10, 11), add2(9, 12)) != 42) +
           (mix8(-1, 2, -3, 4, -5, 6, -7, 46) != 42) +
           (sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) != 55) +
           (narrow_stack_args(1, 2, 3, 4, 5, 6, 7, 8, -1, 7) != 42) +
           (pair_sum(1, pair, 2) != 47) +
           (small_pair_sum(1, small_pair, 2) != 44) +
           (stacked_pair_sum(1, 2, 3, 4, 5, 6, 7, pair, 8) != 80) +
           (external_sum10(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) != 55) +
           (external_call_sum10(sum10) != 55) +
           (external_pair_sum(1, pair, 2) != 47) +
           (external_small_pair_sum(1, small_pair, 2) != 44) +
           (external_call_pair(pair_sum) != 44) +
           (external_stacked_pair_sum(1, 2, 3, 4, 5, 6, 7, pair, 8) != 80) +
           (external_call_stacked_pair(stacked_pair_sum) != 77) +
           (control_flow(10) != 18) +
           (control_flow(5) != 0) +
           (nested_flow() != 13) +
           (select_value(-1) != 10) +
           (select_value(2) != 23) +
           (select_value(3) != 3) +
           (select_value(9) != 40) +
           (goto_flow() != 42) +
           (switch_in_loop() != 5) +
           (computed_goto_flow() != 42) +
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
