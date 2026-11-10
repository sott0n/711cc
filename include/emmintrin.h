#ifndef _EMMINTRIN_H_INCLUDED
#define _EMMINTRIN_H_INCLUDED

typedef struct {
    unsigned long __lo;
    unsigned long __hi;
} __m128i;

static inline __m128i _mm_loadu_si128(const __m128i *p) {
    __m128i result;
    const unsigned char *src = (const unsigned char *)p;
    unsigned char *dst = (unsigned char *)&result;

    for (int i = 0; i < 16; i++)
        dst[i] = src[i];
    return result;
}

static inline __m128i _mm_set1_epi8(char value) {
    unsigned long repeated = (unsigned char)value;
    repeated *= 0x0101010101010101UL;
    return (__m128i){repeated, repeated};
}

static inline __m128i _mm_set1_epi32(int value) {
    unsigned long repeated = (unsigned int)value;
    repeated |= repeated << 32;
    return (__m128i){repeated, repeated};
}

static inline __m128i _mm_or_si128(__m128i lhs, __m128i rhs) {
    return (__m128i){lhs.__lo | rhs.__lo, lhs.__hi | rhs.__hi};
}

static inline int _mm_movemask_epi8(__m128i value) {
    const unsigned char *bytes = (const unsigned char *)&value;
    int result = 0;

    for (int i = 0; i < 16; i++)
        result |= (bytes[i] >> 7) << i;
    return result;
}

static inline __m128i _mm_cmpeq_epi8(__m128i lhs, __m128i rhs) {
    __m128i result;
    const unsigned char *a = (const unsigned char *)&lhs;
    const unsigned char *b = (const unsigned char *)&rhs;
    unsigned char *out = (unsigned char *)&result;

    for (int i = 0; i < 16; i++)
        out[i] = a[i] == b[i] ? 0xff : 0;
    return result;
}

static inline __m128i _mm_cmpeq_epi32(__m128i lhs, __m128i rhs) {
    __m128i result;
    const unsigned int *a = (const unsigned int *)&lhs;
    const unsigned int *b = (const unsigned int *)&rhs;
    unsigned int *out = (unsigned int *)&result;

    for (int i = 0; i < 4; i++)
        out[i] = a[i] == b[i] ? 0xffffffffU : 0;
    return result;
}

static inline __m128i _mm_min_epu8(__m128i lhs, __m128i rhs) {
    __m128i result;
    const unsigned char *a = (const unsigned char *)&lhs;
    const unsigned char *b = (const unsigned char *)&rhs;
    unsigned char *out = (unsigned char *)&result;

    for (int i = 0; i < 16; i++)
        out[i] = a[i] < b[i] ? a[i] : b[i];
    return result;
}

static inline __m128i _mm_subs_epu8(__m128i lhs, __m128i rhs) {
    __m128i result;
    const unsigned char *a = (const unsigned char *)&lhs;
    const unsigned char *b = (const unsigned char *)&rhs;
    unsigned char *out = (unsigned char *)&result;

    for (int i = 0; i < 16; i++)
        out[i] = a[i] > b[i] ? a[i] - b[i] : 0;
    return result;
}

#endif
