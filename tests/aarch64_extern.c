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
