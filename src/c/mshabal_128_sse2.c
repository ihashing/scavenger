/*
 * Parallel implementation of Shabal, using the SSE2 unit. This code
 * compiles and runs on x86 architectures, in 32-bit or 64-bit mode,
 * which possess a SSE2-compatible SIMD unit.
 *
 *
 * (c) 2010 SAPHIR project. This software is provided 'as-is', without
 * any epxress or implied warranty. In no event will the authors be held
 * liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to no restriction.
 *
 * Technical remarks and questions can be addressed to:
 * <thomas.pornin@cryptolog.com>
 *
 * Routines have been optimized for and limited to burst mining. Deadline
 * generation only, no signature generation possible (use sph_shabal for the
 * deadline). Johnny
 */

#include <emmintrin.h>
#include <stddef.h>
#include <string.h>
#include "mshabal_128_sse2.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4146)
#endif

typedef mshabal_u32 u32;

#define C32(x) ((u32)x##UL)
#define T32(x) ((x)&C32(0xFFFFFFFF))
#define ROTL32(x, n) T32(((x) << (n)) | ((x) >> (32 - (n))))

static void simd128_sse2_mshabal_compress(mshabal_context* sc, const unsigned char* buf0,
                                     const unsigned char* buf1, const unsigned char* buf2,
                                     const unsigned char* buf3, size_t num) {
    union {
        u32 words[64];
        __m128i data[16];
    } u;
    size_t j;
    __m128i A[12], B[16], C[16];
    __m128i one;

    for (j = 0; j < 12; j++) A[j] = _mm_loadu_si128((__m128i*)sc->state + j);
    for (j = 0; j < 16; j++) {
        B[j] = _mm_loadu_si128((__m128i*)sc->state + j + 12);
        C[j] = _mm_loadu_si128((__m128i*)sc->state + j + 28);
    }
    one = _mm_set1_epi32(C32(0xFFFFFFFF));

#define M(i) _mm_load_si128(u.data + (i))

    while (num-- > 0) {
        for (j = 0; j < 64; j += 4) {
            u.words[j + 0] = *(u32*)(buf0 + j);
            u.words[j + 1] = *(u32*)(buf1 + j);
            u.words[j + 2] = *(u32*)(buf2 + j);
            u.words[j + 3] = *(u32*)(buf3 + j);
        }

        for (j = 0; j < 16; j++) B[j] = _mm_add_epi32(B[j], M(j));

        A[0] = _mm_xor_si128(A[0], _mm_set1_epi32(sc->Wlow));
        A[1] = _mm_xor_si128(A[1], _mm_set1_epi32(sc->Whigh));

        for (j = 0; j < 16; j++)
            B[j] = _mm_or_si128(_mm_slli_epi32(B[j], 17), _mm_srli_epi32(B[j], 15));

#define PP(xa0, xa1, xb0, xb1, xb2, xb3, xc, xm)                                                   \
    do {                                                                                           \
        __m128i tt;                                                                                \
        tt = _mm_or_si128(_mm_slli_epi32(xa1, 15), _mm_srli_epi32(xa1, 17));                       \
        tt = _mm_add_epi32(_mm_slli_epi32(tt, 2), tt);                                             \
        tt = _mm_xor_si128(_mm_xor_si128(xa0, tt), xc);                                            \
        tt = _mm_add_epi32(_mm_slli_epi32(tt, 1), tt);                                             \
        tt = _mm_xor_si128(_mm_xor_si128(tt, xb1), _mm_xor_si128(_mm_andnot_si128(xb3, xb2), xm)); \
        xa0 = tt;                                                                                  \
        tt = xb0;                                                                                  \
        tt = _mm_or_si128(_mm_slli_epi32(tt, 1), _mm_srli_epi32(tt, 31));                          \
        xb0 = _mm_xor_si128(tt, _mm_xor_si128(xa0, one));                                          \
    } while (0)

        PP(A[0x0], A[0xB], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M(0x0));
        PP(A[0x1], A[0x0], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M(0x1));
        PP(A[0x2], A[0x1], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M(0x2));
        PP(A[0x3], A[0x2], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M(0x3));
        PP(A[0x4], A[0x3], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M(0x4));
        PP(A[0x5], A[0x4], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M(0x5));
        PP(A[0x6], A[0x5], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M(0x6));
        PP(A[0x7], A[0x6], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M(0x7));
        PP(A[0x8], A[0x7], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M(0x8));
        PP(A[0x9], A[0x8], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M(0x9));
        PP(A[0xA], A[0x9], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M(0xA));
        PP(A[0xB], A[0xA], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M(0xB));
        PP(A[0x0], A[0xB], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M(0xC));
        PP(A[0x1], A[0x0], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M(0xD));
        PP(A[0x2], A[0x1], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M(0xE));
        PP(A[0x3], A[0x2], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M(0xF));

        PP(A[0x4], A[0x3], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M(0x0));
        PP(A[0x5], A[0x4], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M(0x1));
        PP(A[0x6], A[0x5], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M(0x2));
        PP(A[0x7], A[0x6], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M(0x3));
        PP(A[0x8], A[0x7], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M(0x4));
        PP(A[0x9], A[0x8], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M(0x5));
        PP(A[0xA], A[0x9], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M(0x6));
        PP(A[0xB], A[0xA], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M(0x7));
        PP(A[0x0], A[0xB], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M(0x8));
        PP(A[0x1], A[0x0], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M(0x9));
        PP(A[0x2], A[0x1], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M(0xA));
        PP(A[0x3], A[0x2], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M(0xB));
        PP(A[0x4], A[0x3], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M(0xC));
        PP(A[0x5], A[0x4], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M(0xD));
        PP(A[0x6], A[0x5], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M(0xE));
        PP(A[0x7], A[0x6], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M(0xF));

        PP(A[0x8], A[0x7], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M(0x0));
        PP(A[0x9], A[0x8], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M(0x1));
        PP(A[0xA], A[0x9], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M(0x2));
        PP(A[0xB], A[0xA], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M(0x3));
        PP(A[0x0], A[0xB], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M(0x4));
        PP(A[0x1], A[0x0], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M(0x5));
        PP(A[0x2], A[0x1], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M(0x6));
        PP(A[0x3], A[0x2], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M(0x7));
        PP(A[0x4], A[0x3], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M(0x8));
        PP(A[0x5], A[0x4], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M(0x9));
        PP(A[0x6], A[0x5], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M(0xA));
        PP(A[0x7], A[0x6], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M(0xB));
        PP(A[0x8], A[0x7], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M(0xC));
        PP(A[0x9], A[0x8], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M(0xD));
        PP(A[0xA], A[0x9], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M(0xE));
        PP(A[0xB], A[0xA], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M(0xF));

        A[0xB] = _mm_add_epi32(A[0xB], C[0x6]);
        A[0xA] = _mm_add_epi32(A[0xA], C[0x5]);
        A[0x9] = _mm_add_epi32(A[0x9], C[0x4]);
        A[0x8] = _mm_add_epi32(A[0x8], C[0x3]);
        A[0x7] = _mm_add_epi32(A[0x7], C[0x2]);
        A[0x6] = _mm_add_epi32(A[0x6], C[0x1]);
        A[0x5] = _mm_add_epi32(A[0x5], C[0x0]);
        A[0x4] = _mm_add_epi32(A[0x4], C[0xF]);
        A[0x3] = _mm_add_epi32(A[0x3], C[0xE]);
        A[0x2] = _mm_add_epi32(A[0x2], C[0xD]);
        A[0x1] = _mm_add_epi32(A[0x1], C[0xC]);
        A[0x0] = _mm_add_epi32(A[0x0], C[0xB]);
        A[0xB] = _mm_add_epi32(A[0xB], C[0xA]);
        A[0xA] = _mm_add_epi32(A[0xA], C[0x9]);
        A[0x9] = _mm_add_epi32(A[0x9], C[0x8]);
        A[0x8] = _mm_add_epi32(A[0x8], C[0x7]);
        A[0x7] = _mm_add_epi32(A[0x7], C[0x6]);
        A[0x6] = _mm_add_epi32(A[0x6], C[0x5]);
        A[0x5] = _mm_add_epi32(A[0x5], C[0x4]);
        A[0x4] = _mm_add_epi32(A[0x4], C[0x3]);
        A[0x3] = _mm_add_epi32(A[0x3], C[0x2]);
        A[0x2] = _mm_add_epi32(A[0x2], C[0x1]);
        A[0x1] = _mm_add_epi32(A[0x1], C[0x0]);
        A[0x0] = _mm_add_epi32(A[0x0], C[0xF]);
        A[0xB] = _mm_add_epi32(A[0xB], C[0xE]);
        A[0xA] = _mm_add_epi32(A[0xA], C[0xD]);
        A[0x9] = _mm_add_epi32(A[0x9], C[0xC]);
        A[0x8] = _mm_add_epi32(A[0x8], C[0xB]);
        A[0x7] = _mm_add_epi32(A[0x7], C[0xA]);
        A[0x6] = _mm_add_epi32(A[0x6], C[0x9]);
        A[0x5] = _mm_add_epi32(A[0x5], C[0x8]);
        A[0x4] = _mm_add_epi32(A[0x4], C[0x7]);
        A[0x3] = _mm_add_epi32(A[0x3], C[0x6]);
        A[0x2] = _mm_add_epi32(A[0x2], C[0x5]);
        A[0x1] = _mm_add_epi32(A[0x1], C[0x4]);
        A[0x0] = _mm_add_epi32(A[0x0], C[0x3]);

#define SWAP_AND_SUB(xb, xc, xm)    \
    do {                            \
        __m128i tmp;                \
        tmp = xb;                   \
        xb = _mm_sub_epi32(xc, xm); \
        xc = tmp;                   \
    } while (0)

        SWAP_AND_SUB(B[0x0], C[0x0], M(0x0));
        SWAP_AND_SUB(B[0x1], C[0x1], M(0x1));
        SWAP_AND_SUB(B[0x2], C[0x2], M(0x2));
        SWAP_AND_SUB(B[0x3], C[0x3], M(0x3));
        SWAP_AND_SUB(B[0x4], C[0x4], M(0x4));
        SWAP_AND_SUB(B[0x5], C[0x5], M(0x5));
        SWAP_AND_SUB(B[0x6], C[0x6], M(0x6));
        SWAP_AND_SUB(B[0x7], C[0x7], M(0x7));
        SWAP_AND_SUB(B[0x8], C[0x8], M(0x8));
        SWAP_AND_SUB(B[0x9], C[0x9], M(0x9));
        SWAP_AND_SUB(B[0xA], C[0xA], M(0xA));
        SWAP_AND_SUB(B[0xB], C[0xB], M(0xB));
        SWAP_AND_SUB(B[0xC], C[0xC], M(0xC));
        SWAP_AND_SUB(B[0xD], C[0xD], M(0xD));
        SWAP_AND_SUB(B[0xE], C[0xE], M(0xE));
        SWAP_AND_SUB(B[0xF], C[0xF], M(0xF));

        buf0 += 64;
        buf1 += 64;
        buf2 += 64;
        buf3 += 64;
        if (++sc->Wlow == 0) sc->Whigh++;
    }

    for (j = 0; j < 12; j++) _mm_storeu_si128((__m128i*)sc->state + j, A[j]);
    for (j = 0; j < 16; j++) {
        _mm_storeu_si128((__m128i*)sc->state + j + 12, B[j]);
        _mm_storeu_si128((__m128i*)sc->state + j + 28, C[j]);
    }
#undef M
}

/* see shabal_small.h */

void simd128_sse2_mshabal_init(mshabal_context* sc, unsigned out_size) {
    unsigned u;

    // for (u = 0; u < 176; u++)  sc->state[u] = 0;
    memset(sc->state, 0, sizeof sc->state);
    memset(sc->buf0, 0, sizeof sc->buf0);
    memset(sc->buf1, 0, sizeof sc->buf1);
    memset(sc->buf2, 0, sizeof sc->buf2);
    memset(sc->buf3, 0, sizeof sc->buf3);
    for (u = 0; u < 16; u++) {
        sc->buf0[4 * u + 0] = (out_size + u);
        sc->buf0[4 * u + 1] = (out_size + u) >> 8;
        sc->buf1[4 * u + 0] = (out_size + u);
        sc->buf1[4 * u + 1] = (out_size + u) >> 8;
        sc->buf2[4 * u + 0] = (out_size + u);
        sc->buf2[4 * u + 1] = (out_size + u) >> 8;
        sc->buf3[4 * u + 0] = (out_size + u);
        sc->buf3[4 * u + 1] = (out_size + u) >> 8;
    }
    sc->Whigh = sc->Wlow = C32(0xFFFFFFFF);
    simd128_sse2_mshabal_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, 1);
    for (u = 0; u < 16; u++) {
        sc->buf0[4 * u + 0] = (out_size + u + 16);
        sc->buf0[4 * u + 1] = (out_size + u + 16) >> 8;
        sc->buf1[4 * u + 0] = (out_size + u + 16);
        sc->buf1[4 * u + 1] = (out_size + u + 16) >> 8;
        sc->buf2[4 * u + 0] = (out_size + u + 16);
        sc->buf2[4 * u + 1] = (out_size + u + 16) >> 8;
        sc->buf3[4 * u + 0] = (out_size + u + 16);
        sc->buf3[4 * u + 1] = (out_size + u + 16) >> 8;
    }
    simd128_sse2_mshabal_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, 1);
    sc->ptr = 0;
    sc->out_size = out_size;
}

/* see shabal_small.h */
void simd128_sse2_mshabal(mshabal_context* sc, const void* data0, const void* data1, const void* data2,
                     const void* data3, size_t len) {
    size_t ptr, num;
    ptr = sc->ptr;
    if (ptr != 0) {
        size_t clen = (sizeof sc->buf0 - ptr);
        if (clen > len) {
            memcpy(sc->buf0 + ptr, data0, len);
            memcpy(sc->buf1 + ptr, data1, len);
            memcpy(sc->buf2 + ptr, data2, len);
            memcpy(sc->buf3 + ptr, data3, len);
            sc->ptr = ptr + len;
            return;
        } else {
            memcpy(sc->buf0 + ptr, data0, clen);
            memcpy(sc->buf1 + ptr, data1, clen);
            memcpy(sc->buf2 + ptr, data2, clen);
            memcpy(sc->buf3 + ptr, data3, clen);
            simd128_sse2_mshabal_compress(sc, sc->buf0, sc->buf1, sc->buf2, sc->buf3, 1);
            data0 = (const unsigned char*)data0 + clen;
            data1 = (const unsigned char*)data1 + clen;
            data2 = (const unsigned char*)data2 + clen;
            data3 = (const unsigned char*)data3 + clen;
            len -= clen;
        }
    }

    num = 1;
    if (num != 0) {
        simd128_sse2_mshabal_compress(sc, (const unsigned char*)data0, (const unsigned char*)data1,
                                 (const unsigned char*)data2, (const unsigned char*)data3, num);
        sc->xbuf0 = (unsigned char*)data0 + (num << 6);
        sc->xbuf1 = (unsigned char*)data1 + (num << 6);
        sc->xbuf2 = (unsigned char*)data2 + (num << 6);
        sc->xbuf3 = (unsigned char*)data3 + (num << 6);
    }
    len &= 63;
    sc->ptr = len;
}

// Johnnys double pointer no memmove no register buffering no simd unpack/repack no inital state change burst mining only
// optimisation functions (tm) :-p
void simd128_sse2_mshabal_openclose_fast(mshabal_context_fast* sc, void* u1, void* u2, void* dst0, void* dst1, void* dst2, 
                                          void* dst3) {
    union input {
        u32 words[64];
        __m128i data[16];
    };
    size_t j;
    __m128i A[12], B[16], C[16];
    __m128i one;

    for (j = 0; j < 12; j++) A[j] = _mm_loadu_si128((__m128i*)sc->state + j);
    for (j = 0; j < 16; j++) {
        B[j] = _mm_loadu_si128((__m128i*)sc->state + j + 12);
        C[j] = _mm_loadu_si128((__m128i*)sc->state + j + 28);
    }
    one = _mm_set1_epi32(C32(0xFFFFFFFF));

// Round 1/5
#define M(i) _mm_load_si128((*(union input*)u1).data + (i))

        for (j = 0; j < 16; j++) B[j] = _mm_add_epi32(B[j], M(j));

        A[0] = _mm_xor_si128(A[0], _mm_set1_epi32(sc->Wlow));
        A[1] = _mm_xor_si128(A[1], _mm_set1_epi32(sc->Whigh));

        for (j = 0; j < 16; j++)
            B[j] = _mm_or_si128(_mm_slli_epi32(B[j], 17), _mm_srli_epi32(B[j], 15));

#define PP(xa0, xa1, xb0, xb1, xb2, xb3, xc, xm)                                                   \
    do {                                                                                           \
        __m128i tt;                                                                                \
        tt = _mm_or_si128(_mm_slli_epi32(xa1, 15), _mm_srli_epi32(xa1, 17));                       \
        tt = _mm_add_epi32(_mm_slli_epi32(tt, 2), tt);                                             \
        tt = _mm_xor_si128(_mm_xor_si128(xa0, tt), xc);                                            \
        tt = _mm_add_epi32(_mm_slli_epi32(tt, 1), tt);                                             \
        tt = _mm_xor_si128(_mm_xor_si128(tt, xb1), _mm_xor_si128(_mm_andnot_si128(xb3, xb2), xm)); \
        xa0 = tt;                                                                                  \
        tt = xb0;                                                                                  \
        tt = _mm_or_si128(_mm_slli_epi32(tt, 1), _mm_srli_epi32(tt, 31));                          \
        xb0 = _mm_xor_si128(tt, _mm_xor_si128(xa0, one));                                          \
    } while (0)

        PP(A[0x0], A[0xB], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M(0x0));
        PP(A[0x1], A[0x0], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M(0x1));
        PP(A[0x2], A[0x1], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M(0x2));
        PP(A[0x3], A[0x2], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M(0x3));
        PP(A[0x4], A[0x3], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M(0x4));
        PP(A[0x5], A[0x4], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M(0x5));
        PP(A[0x6], A[0x5], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M(0x6));
        PP(A[0x7], A[0x6], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M(0x7));
        PP(A[0x8], A[0x7], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M(0x8));
        PP(A[0x9], A[0x8], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M(0x9));
        PP(A[0xA], A[0x9], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M(0xA));
        PP(A[0xB], A[0xA], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M(0xB));
        PP(A[0x0], A[0xB], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M(0xC));
        PP(A[0x1], A[0x0], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M(0xD));
        PP(A[0x2], A[0x1], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M(0xE));
        PP(A[0x3], A[0x2], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M(0xF));

        PP(A[0x4], A[0x3], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M(0x0));
        PP(A[0x5], A[0x4], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M(0x1));
        PP(A[0x6], A[0x5], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M(0x2));
        PP(A[0x7], A[0x6], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M(0x3));
        PP(A[0x8], A[0x7], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M(0x4));
        PP(A[0x9], A[0x8], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M(0x5));
        PP(A[0xA], A[0x9], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M(0x6));
        PP(A[0xB], A[0xA], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M(0x7));
        PP(A[0x0], A[0xB], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M(0x8));
        PP(A[0x1], A[0x0], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M(0x9));
        PP(A[0x2], A[0x1], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M(0xA));
        PP(A[0x3], A[0x2], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M(0xB));
        PP(A[0x4], A[0x3], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M(0xC));
        PP(A[0x5], A[0x4], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M(0xD));
        PP(A[0x6], A[0x5], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M(0xE));
        PP(A[0x7], A[0x6], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M(0xF));

        PP(A[0x8], A[0x7], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M(0x0));
        PP(A[0x9], A[0x8], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M(0x1));
        PP(A[0xA], A[0x9], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M(0x2));
        PP(A[0xB], A[0xA], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M(0x3));
        PP(A[0x0], A[0xB], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M(0x4));
        PP(A[0x1], A[0x0], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M(0x5));
        PP(A[0x2], A[0x1], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M(0x6));
        PP(A[0x3], A[0x2], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M(0x7));
        PP(A[0x4], A[0x3], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M(0x8));
        PP(A[0x5], A[0x4], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M(0x9));
        PP(A[0x6], A[0x5], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M(0xA));
        PP(A[0x7], A[0x6], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M(0xB));
        PP(A[0x8], A[0x7], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M(0xC));
        PP(A[0x9], A[0x8], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M(0xD));
        PP(A[0xA], A[0x9], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M(0xE));
        PP(A[0xB], A[0xA], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M(0xF));

        A[0xB] = _mm_add_epi32(A[0xB], C[0x6]);
        A[0xA] = _mm_add_epi32(A[0xA], C[0x5]);
        A[0x9] = _mm_add_epi32(A[0x9], C[0x4]);
        A[0x8] = _mm_add_epi32(A[0x8], C[0x3]);
        A[0x7] = _mm_add_epi32(A[0x7], C[0x2]);
        A[0x6] = _mm_add_epi32(A[0x6], C[0x1]);
        A[0x5] = _mm_add_epi32(A[0x5], C[0x0]);
        A[0x4] = _mm_add_epi32(A[0x4], C[0xF]);
        A[0x3] = _mm_add_epi32(A[0x3], C[0xE]);
        A[0x2] = _mm_add_epi32(A[0x2], C[0xD]);
        A[0x1] = _mm_add_epi32(A[0x1], C[0xC]);
        A[0x0] = _mm_add_epi32(A[0x0], C[0xB]);
        A[0xB] = _mm_add_epi32(A[0xB], C[0xA]);
        A[0xA] = _mm_add_epi32(A[0xA], C[0x9]);
        A[0x9] = _mm_add_epi32(A[0x9], C[0x8]);
        A[0x8] = _mm_add_epi32(A[0x8], C[0x7]);
        A[0x7] = _mm_add_epi32(A[0x7], C[0x6]);
        A[0x6] = _mm_add_epi32(A[0x6], C[0x5]);
        A[0x5] = _mm_add_epi32(A[0x5], C[0x4]);
        A[0x4] = _mm_add_epi32(A[0x4], C[0x3]);
        A[0x3] = _mm_add_epi32(A[0x3], C[0x2]);
        A[0x2] = _mm_add_epi32(A[0x2], C[0x1]);
        A[0x1] = _mm_add_epi32(A[0x1], C[0x0]);
        A[0x0] = _mm_add_epi32(A[0x0], C[0xF]);
        A[0xB] = _mm_add_epi32(A[0xB], C[0xE]);
        A[0xA] = _mm_add_epi32(A[0xA], C[0xD]);
        A[0x9] = _mm_add_epi32(A[0x9], C[0xC]);
        A[0x8] = _mm_add_epi32(A[0x8], C[0xB]);
        A[0x7] = _mm_add_epi32(A[0x7], C[0xA]);
        A[0x6] = _mm_add_epi32(A[0x6], C[0x9]);
        A[0x5] = _mm_add_epi32(A[0x5], C[0x8]);
        A[0x4] = _mm_add_epi32(A[0x4], C[0x7]);
        A[0x3] = _mm_add_epi32(A[0x3], C[0x6]);
        A[0x2] = _mm_add_epi32(A[0x2], C[0x5]);
        A[0x1] = _mm_add_epi32(A[0x1], C[0x4]);
        A[0x0] = _mm_add_epi32(A[0x0], C[0x3]);

#define SWAP_AND_SUB(xb, xc, xm)    \
    do {                            \
        __m128i tmp;                \
        tmp = xb;                   \
        xb = _mm_sub_epi32(xc, xm); \
        xc = tmp;                   \
    } while (0)

        SWAP_AND_SUB(B[0x0], C[0x0], M(0x0));
        SWAP_AND_SUB(B[0x1], C[0x1], M(0x1));
        SWAP_AND_SUB(B[0x2], C[0x2], M(0x2));
        SWAP_AND_SUB(B[0x3], C[0x3], M(0x3));
        SWAP_AND_SUB(B[0x4], C[0x4], M(0x4));
        SWAP_AND_SUB(B[0x5], C[0x5], M(0x5));
        SWAP_AND_SUB(B[0x6], C[0x6], M(0x6));
        SWAP_AND_SUB(B[0x7], C[0x7], M(0x7));
        SWAP_AND_SUB(B[0x8], C[0x8], M(0x8));
        SWAP_AND_SUB(B[0x9], C[0x9], M(0x9));
        SWAP_AND_SUB(B[0xA], C[0xA], M(0xA));
        SWAP_AND_SUB(B[0xB], C[0xB], M(0xB));
        SWAP_AND_SUB(B[0xC], C[0xC], M(0xC));
        SWAP_AND_SUB(B[0xD], C[0xD], M(0xD));
        SWAP_AND_SUB(B[0xE], C[0xE], M(0xE));
        SWAP_AND_SUB(B[0xF], C[0xF], M(0xF));
        if (++sc->Wlow == 0) sc->Whigh++;


// Round 2-5
#define M2(i) _mm_load_si128((*(union input*)u2).data + (i))

    for (int k = 0; k < 4; k++) {
        for (j = 0; j < 16; j++) B[j] = _mm_add_epi32(B[j], M2(j));

        A[0] = _mm_xor_si128(A[0], _mm_set1_epi32(sc->Wlow));
        A[1] = _mm_xor_si128(A[1], _mm_set1_epi32(sc->Whigh));

        for (j = 0; j < 16; j++)
            B[j] = _mm_or_si128(_mm_slli_epi32(B[j], 17), _mm_srli_epi32(B[j], 15));

        PP(A[0x0], A[0xB], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M2(0x0));
        PP(A[0x1], A[0x0], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M2(0x1));
        PP(A[0x2], A[0x1], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M2(0x2));
        PP(A[0x3], A[0x2], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M2(0x3));
        PP(A[0x4], A[0x3], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M2(0x4));
        PP(A[0x5], A[0x4], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M2(0x5));
        PP(A[0x6], A[0x5], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M2(0x6));
        PP(A[0x7], A[0x6], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M2(0x7));
        PP(A[0x8], A[0x7], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M2(0x8));
        PP(A[0x9], A[0x8], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M2(0x9));
        PP(A[0xA], A[0x9], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M2(0xA));
        PP(A[0xB], A[0xA], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M2(0xB));
        PP(A[0x0], A[0xB], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M2(0xC));
        PP(A[0x1], A[0x0], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M2(0xD));
        PP(A[0x2], A[0x1], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M2(0xE));
        PP(A[0x3], A[0x2], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M2(0xF));

        PP(A[0x4], A[0x3], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M2(0x0));
        PP(A[0x5], A[0x4], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M2(0x1));
        PP(A[0x6], A[0x5], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M2(0x2));
        PP(A[0x7], A[0x6], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M2(0x3));
        PP(A[0x8], A[0x7], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M2(0x4));
        PP(A[0x9], A[0x8], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M2(0x5));
        PP(A[0xA], A[0x9], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M2(0x6));
        PP(A[0xB], A[0xA], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M2(0x7));
        PP(A[0x0], A[0xB], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M2(0x8));
        PP(A[0x1], A[0x0], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M2(0x9));
        PP(A[0x2], A[0x1], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M2(0xA));
        PP(A[0x3], A[0x2], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M2(0xB));
        PP(A[0x4], A[0x3], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M2(0xC));
        PP(A[0x5], A[0x4], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M2(0xD));
        PP(A[0x6], A[0x5], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M2(0xE));
        PP(A[0x7], A[0x6], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M2(0xF));

        PP(A[0x8], A[0x7], B[0x0], B[0xD], B[0x9], B[0x6], C[0x8], M2(0x0));
        PP(A[0x9], A[0x8], B[0x1], B[0xE], B[0xA], B[0x7], C[0x7], M2(0x1));
        PP(A[0xA], A[0x9], B[0x2], B[0xF], B[0xB], B[0x8], C[0x6], M2(0x2));
        PP(A[0xB], A[0xA], B[0x3], B[0x0], B[0xC], B[0x9], C[0x5], M2(0x3));
        PP(A[0x0], A[0xB], B[0x4], B[0x1], B[0xD], B[0xA], C[0x4], M2(0x4));
        PP(A[0x1], A[0x0], B[0x5], B[0x2], B[0xE], B[0xB], C[0x3], M2(0x5));
        PP(A[0x2], A[0x1], B[0x6], B[0x3], B[0xF], B[0xC], C[0x2], M2(0x6));
        PP(A[0x3], A[0x2], B[0x7], B[0x4], B[0x0], B[0xD], C[0x1], M2(0x7));
        PP(A[0x4], A[0x3], B[0x8], B[0x5], B[0x1], B[0xE], C[0x0], M2(0x8));
        PP(A[0x5], A[0x4], B[0x9], B[0x6], B[0x2], B[0xF], C[0xF], M2(0x9));
        PP(A[0x6], A[0x5], B[0xA], B[0x7], B[0x3], B[0x0], C[0xE], M2(0xA));
        PP(A[0x7], A[0x6], B[0xB], B[0x8], B[0x4], B[0x1], C[0xD], M2(0xB));
        PP(A[0x8], A[0x7], B[0xC], B[0x9], B[0x5], B[0x2], C[0xC], M2(0xC));
        PP(A[0x9], A[0x8], B[0xD], B[0xA], B[0x6], B[0x3], C[0xB], M2(0xD));
        PP(A[0xA], A[0x9], B[0xE], B[0xB], B[0x7], B[0x4], C[0xA], M2(0xE));
        PP(A[0xB], A[0xA], B[0xF], B[0xC], B[0x8], B[0x5], C[0x9], M2(0xF));

        A[0xB] = _mm_add_epi32(A[0xB], C[0x6]);
        A[0xA] = _mm_add_epi32(A[0xA], C[0x5]);
        A[0x9] = _mm_add_epi32(A[0x9], C[0x4]);
        A[0x8] = _mm_add_epi32(A[0x8], C[0x3]);
        A[0x7] = _mm_add_epi32(A[0x7], C[0x2]);
        A[0x6] = _mm_add_epi32(A[0x6], C[0x1]);
        A[0x5] = _mm_add_epi32(A[0x5], C[0x0]);
        A[0x4] = _mm_add_epi32(A[0x4], C[0xF]);
        A[0x3] = _mm_add_epi32(A[0x3], C[0xE]);
        A[0x2] = _mm_add_epi32(A[0x2], C[0xD]);
        A[0x1] = _mm_add_epi32(A[0x1], C[0xC]);
        A[0x0] = _mm_add_epi32(A[0x0], C[0xB]);
        A[0xB] = _mm_add_epi32(A[0xB], C[0xA]);
        A[0xA] = _mm_add_epi32(A[0xA], C[0x9]);
        A[0x9] = _mm_add_epi32(A[0x9], C[0x8]);
        A[0x8] = _mm_add_epi32(A[0x8], C[0x7]);
        A[0x7] = _mm_add_epi32(A[0x7], C[0x6]);
        A[0x6] = _mm_add_epi32(A[0x6], C[0x5]);
        A[0x5] = _mm_add_epi32(A[0x5], C[0x4]);
        A[0x4] = _mm_add_epi32(A[0x4], C[0x3]);
        A[0x3] = _mm_add_epi32(A[0x3], C[0x2]);
        A[0x2] = _mm_add_epi32(A[0x2], C[0x1]);
        A[0x1] = _mm_add_epi32(A[0x1], C[0x0]);
        A[0x0] = _mm_add_epi32(A[0x0], C[0xF]);
        A[0xB] = _mm_add_epi32(A[0xB], C[0xE]);
        A[0xA] = _mm_add_epi32(A[0xA], C[0xD]);
        A[0x9] = _mm_add_epi32(A[0x9], C[0xC]);
        A[0x8] = _mm_add_epi32(A[0x8], C[0xB]);
        A[0x7] = _mm_add_epi32(A[0x7], C[0xA]);
        A[0x6] = _mm_add_epi32(A[0x6], C[0x9]);
        A[0x5] = _mm_add_epi32(A[0x5], C[0x8]);
        A[0x4] = _mm_add_epi32(A[0x4], C[0x7]);
        A[0x3] = _mm_add_epi32(A[0x3], C[0x6]);
        A[0x2] = _mm_add_epi32(A[0x2], C[0x5]);
        A[0x1] = _mm_add_epi32(A[0x1], C[0x4]);
        A[0x0] = _mm_add_epi32(A[0x0], C[0x3]);

        SWAP_AND_SUB(B[0x0], C[0x0], M2(0x0));
        SWAP_AND_SUB(B[0x1], C[0x1], M2(0x1));
        SWAP_AND_SUB(B[0x2], C[0x2], M2(0x2));
        SWAP_AND_SUB(B[0x3], C[0x3], M2(0x3));
        SWAP_AND_SUB(B[0x4], C[0x4], M2(0x4));
        SWAP_AND_SUB(B[0x5], C[0x5], M2(0x5));
        SWAP_AND_SUB(B[0x6], C[0x6], M2(0x6));
        SWAP_AND_SUB(B[0x7], C[0x7], M2(0x7));
        SWAP_AND_SUB(B[0x8], C[0x8], M2(0x8));
        SWAP_AND_SUB(B[0x9], C[0x9], M2(0x9));
        SWAP_AND_SUB(B[0xA], C[0xA], M2(0xA));
        SWAP_AND_SUB(B[0xB], C[0xB], M2(0xB));
        SWAP_AND_SUB(B[0xC], C[0xC], M2(0xC));
        SWAP_AND_SUB(B[0xD], C[0xD], M2(0xD));
        SWAP_AND_SUB(B[0xE], C[0xE], M2(0xE));
        SWAP_AND_SUB(B[0xF], C[0xF], M2(0xF));

        if (++sc->Wlow == 0) sc->Whigh++;

        if (sc->Wlow-- == 0) sc->Whigh--;
    }

    // transfer resuts to ram and quick state reset 
    // download deadlines simd aligned
    u32 simd_dst[8];
    _mm_storeu_si128((__m128i*)&simd_dst[0], C[8]);
    _mm_storeu_si128((__m128i*)&simd_dst[4], C[9]);
   
    // simd unpack
    unsigned z;
    for (z = 0; z < 2; z++) {
        unsigned y = (z << 2);
        ((u32*)dst0)[z] = simd_dst[y + 0];
        ((u32*)dst1)[z] = simd_dst[y + 1];
        ((u32*)dst2)[z] = simd_dst[y + 2];
        ((u32*)dst3)[z] = simd_dst[y + 3];
    }
    
    // reset Wlow & Whigh
    sc->Wlow = 1;
    sc->Whigh = 0;
}

#ifdef __cplusplus
extern "C" {
#endif
