long external_sum10(long a, long b, long c, long d, long e,
                    long f, long g, long h, long i, long j) {
    return a + b + c + d + e + f + g + h + i + j;
}

long external_call_sum10(long (*fn)(long, long, long, long, long,
                                     long, long, long, long, long)) {
    return fn(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
}
