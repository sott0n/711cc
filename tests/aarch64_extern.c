long external_sum10(long a, long b, long c, long d, long e,
                    long f, long g, long h, long i, long j) {
    return a + b + c + d + e + f + g + h + i + j;
}

long external_call_sum10(long (*fn)(long, long, long, long, long,
                                     long, long, long, long, long)) {
    return fn(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
}

struct Pair {
    int left;
    long right;
};

struct SmallPair {
    int left;
    int right;
};

struct Wide {
    long values[3];
    char tag;
};

long external_pair_sum(int prefix, struct Pair pair, int suffix) {
    return prefix + pair.left + pair.right + suffix;
}

long external_small_pair_sum(int prefix, struct SmallPair pair, int suffix) {
    return prefix + pair.left + pair.right + suffix;
}

long external_call_pair(long (*fn)(int, struct Pair, int)) {
    struct Pair pair = {20, 21};
    return fn(1, pair, 2);
}

long external_stacked_pair_sum(long a, long b, long c, long d, long e,
                               long f, long g, struct Pair pair, long tail) {
    return a + b + c + d + e + f + g + pair.left + pair.right + tail;
}

long external_call_stacked_pair(long (*fn)(long, long, long, long, long, long,
                                            long, struct Pair, long)) {
    struct Pair pair = {20, 21};
    return fn(1, 2, 3, 4, 5, 6, 7, pair, 8);
}

struct Pair external_make_pair(int left, long right) {
    struct Pair result = {left, right};
    return result;
}

struct SmallPair external_make_small_pair(int left, int right) {
    struct SmallPair result = {left, right};
    return result;
}

long external_call_pair_maker(struct Pair (*fn)(int, long)) {
    struct Pair result = fn(20, 22);
    return result.left + result.right;
}

long external_wide_sum(int prefix, struct Wide value, int suffix) {
    value.values[1] += 1;
    return prefix + value.values[0] + value.values[1] + value.values[2] +
           value.tag + suffix;
}

long external_call_wide_sum(long (*fn)(int, struct Wide, int)) {
    struct Wide value = {{10, 20, 30}, 40};
    long result = fn(1, value, 2);
    return value.values[1] == 20 ? result : -1;
}

long external_stacked_wide_sum(long a, long b, long c, long d, long e,
                               long f, long g, long h, struct Wide value,
                               long tail) {
    value.values[1] += 1;
    return a + b + c + d + e + f + g + h + value.values[0] +
           value.values[1] + value.values[2] + value.tag + tail;
}

long external_call_stacked_wide_sum(
    long (*fn)(long, long, long, long, long, long, long, long,
               struct Wide, long)) {
    struct Wide value = {{10, 20, 30}, 40};
    long result = fn(1, 2, 3, 4, 5, 6, 7, 8, value, 9);
    return value.values[1] == 20 ? result : -1;
}
