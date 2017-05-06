// The MIT License (MIT)

// Copyright (c) 2016 Daniel Fu

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

//
// Created by 理 傅 on 2017/1/6.
//

#ifndef KCP_ENCODING_H
#define KCP_ENCODING_H
#include <stdint.h>
typedef unsigned char byte;
//---------------------------------------------------------------------
// WORD ORDER
//---------------------------------------------------------------------
#ifndef IWORDS_BIG_ENDIAN
#ifdef _BIG_ENDIAN_
#if _BIG_ENDIAN_
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#ifndef IWORDS_BIG_ENDIAN
#if defined(__hppa__) || defined(__m68k__) || defined(mc68000) ||              \
    defined(_M_M68K) || (defined(__MIPS__) && defined(__MISPEB__)) ||          \
    defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) ||             \
    defined(__sparc__) || defined(__powerpc__) || defined(__mc68000__) ||      \
    defined(__s390x__) || defined(__s390__)
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN 0
#endif
#endif

/* encode 16 bits unsigned int (lsb) */
inline byte *encode16u(byte *p, uint16_t w) {
#if IWORDS_BIG_ENDIAN
    *(byte *)(p + 0) = (w & 255);
    *(byte *)(p + 1) = (w >> 8);
#else
    *(uint16_t *)(p) = w;
#endif
    p += 2;
    return p;
}

/* Decode 16 bits unsigned int (lsb) */
inline byte *decode16u(byte *p, uint16_t *w) {
#if IWORDS_BIG_ENDIAN
    *w = *(const unsigned char *)(p + 1);
    *w = *(const unsigned char *)(p + 0) + (*w << 8);
#else
    *w = *(const unsigned short *)p;
#endif
    p += 2;
    return p;
}

/* encode 32 bits unsigned int (lsb) */
inline byte *encode32u(byte *p, uint32_t l) {
#if IWORDS_BIG_ENDIAN
    *(unsigned char *)(p + 0) = (unsigned char)((l >> 0) & 0xff);
    *(unsigned char *)(p + 1) = (unsigned char)((l >> 8) & 0xff);
    *(unsigned char *)(p + 2) = (unsigned char)((l >> 16) & 0xff);
    *(unsigned char *)(p + 3) = (unsigned char)((l >> 24) & 0xff);
#else
    *(uint32_t *)p = l;
#endif
    p += 4;
    return p;
}

/* Decode 32 bits unsigned int (lsb) */
inline byte *decode32u(byte *p, uint32_t *l) {
#if IWORDS_BIG_ENDIAN
    *l = *(const unsigned char *)(p + 3);
    *l = *(const unsigned char *)(p + 2) + (*l << 8);
    *l = *(const unsigned char *)(p + 1) + (*l << 8);
    *l = *(const unsigned char *)(p + 0) + (*l << 8);
#else
    *l = *(const uint32_t *)p;
#endif
    p += 4;
    return p;
}

#endif // KCP_ENCODING_H
