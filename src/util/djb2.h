/*
 * Open Surge Engine
 * djb2.h - a djb2 hash function utility with compile-time implementations
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
 * http://opensurge2d.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _DJB2_H
#define _DJB2_H

#include <stdint.h>

/*
 * djb2()
 * djb2 hash function
 */
static inline uint64_t djb2(const char *str)
{
    int c; uint64_t hash = 5381;

    while((c = *((unsigned char*)(str++))))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/*
 * DJB2()
 * Compile-time djb2 hashing for short strings (up to length 31).
 * Returns an unsigned 64-bit integer.
 * Usage: DJB2("string literal")
 */
#define DJB2(s)             (DJB2_STRLEN(s) < 0x1F ? DJB2_1F(DJB2_STRLEN(s), DJB2_PREPARE(s)) : DJB2_1F(0x1F, DJB2_PREPARE(s)))
#define DJB2_STRLEN(s)      (s DJB2_COUNTER)[31]
#define DJB2_PREPARE(s)     (DJB2_COUNTER s DJB2_COUNTER) /* -Warray-bounds */
#define DJB2_COUNTER        "\x1f\x1e\x1d\x1c\x1b\x1a\x19\x18\x17\x16\x15\x14\x13\x12\x11\x10\x0f\x0e\x0d\x0c\x0b\x0a\x09\x08\x07\x06\x05\x04\x03\x02\x01\x00"

#define DJB2_00(l, s)       UINT64_C(5381) /* DJB_L(l, s) := djb2 hash of a string s of length l <= L */
#define DJB2_01(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_00((l)-1, s) : DJB2_00(0, s))
#define DJB2_02(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_01((l)-1, s) : DJB2_00(0, s))
#define DJB2_03(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_02((l)-1, s) : DJB2_00(0, s))
#define DJB2_04(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_03((l)-1, s) : DJB2_00(0, s))
#define DJB2_05(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_04((l)-1, s) : DJB2_00(0, s))
#define DJB2_06(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_05((l)-1, s) : DJB2_00(0, s))
#define DJB2_07(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_06((l)-1, s) : DJB2_00(0, s))
#define DJB2_08(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_07((l)-1, s) : DJB2_00(0, s))
#define DJB2_09(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_08((l)-1, s) : DJB2_00(0, s))
#define DJB2_0A(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_09((l)-1, s) : DJB2_00(0, s))
#define DJB2_0B(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_0A((l)-1, s) : DJB2_00(0, s))
#define DJB2_0C(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_0B((l)-1, s) : DJB2_00(0, s))
#define DJB2_0D(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_0C((l)-1, s) : DJB2_00(0, s))
#define DJB2_0E(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_0D((l)-1, s) : DJB2_00(0, s))
#define DJB2_0F(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_0E((l)-1, s) : DJB2_00(0, s))
#define DJB2_10(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_0F((l)-1, s) : DJB2_00(0, s))
#define DJB2_11(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_10((l)-1, s) : DJB2_00(0, s))
#define DJB2_12(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_11((l)-1, s) : DJB2_00(0, s))
#define DJB2_13(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_12((l)-1, s) : DJB2_00(0, s))
#define DJB2_14(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_13((l)-1, s) : DJB2_00(0, s))
#define DJB2_15(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_14((l)-1, s) : DJB2_00(0, s))
#define DJB2_16(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_15((l)-1, s) : DJB2_00(0, s))
#define DJB2_17(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_16((l)-1, s) : DJB2_00(0, s))
#define DJB2_18(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_17((l)-1, s) : DJB2_00(0, s))
#define DJB2_19(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_18((l)-1, s) : DJB2_00(0, s))
#define DJB2_1A(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_19((l)-1, s) : DJB2_00(0, s))
#define DJB2_1B(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_1A((l)-1, s) : DJB2_00(0, s))
#define DJB2_1C(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_1B((l)-1, s) : DJB2_00(0, s))
#define DJB2_1D(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_1C((l)-1, s) : DJB2_00(0, s))
#define DJB2_1E(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_1D((l)-1, s) : DJB2_00(0, s))
#define DJB2_1F(l, s)       ((l) > 0 ? s[31+(l)] + UINT64_C(33) * DJB2_1E((l)-1, s) : DJB2_00(0, s))

/*
 * DJB2_CONST()
 * Compile-time djb2 hashing for short strings (up to length 15).
 * Returns a constant unsigned 64-bit integer.
 * Usage: DJB2_CONST('s','t','r','i','n','g')
 */
#define DJB2_CONST(...)     (DJB2_CONST_FN(__VA_ARGS__)(__VA_ARGS__))
#define DJB2_CONST_FN(...)  DJB2_CONST_X(__VA_ARGS__, DJB2_CONST_F, DJB2_CONST_E, DJB2_CONST_D, DJB2_CONST_C, DJB2_CONST_B, DJB2_CONST_A, DJB2_CONST_9, DJB2_CONST_8, DJB2_CONST_7, DJB2_CONST_6, DJB2_CONST_5, DJB2_CONST_4, DJB2_CONST_3, DJB2_CONST_2, DJB2_CONST_1, DJB2_CONST_0)
#define DJB2_CONST_X(_1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, _C, _D, _E, _F, FN, ...) FN
#define DJB2_CONST_PICK_0(_0, ...) _0
#define DJB2_CONST_PICK_1(_0, _1, ...) _1
#define DJB2_CONST_PICK_2(_0, _1, _2, ...) _2
#define DJB2_CONST_PICK_3(_0, _1, _2, _3, ...) _3
#define DJB2_CONST_PICK_4(_0, _1, _2, _3, _4, ...) _4
#define DJB2_CONST_PICK_5(_0, _1, _2, _3, _4, _5, ...) _5
#define DJB2_CONST_PICK_6(_0, _1, _2, _3, _4, _5, _6, ...) _6
#define DJB2_CONST_PICK_7(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7
#define DJB2_CONST_PICK_8(_0, _1, _2, _3, _4, _5, _6, _7, _8, ...) _8
#define DJB2_CONST_PICK_9(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define DJB2_CONST_PICK_A(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, ...) _A
#define DJB2_CONST_PICK_B(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, ...) _B
#define DJB2_CONST_PICK_C(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, _C, ...) _C
#define DJB2_CONST_PICK_D(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, _C, _D, ...) _D
#define DJB2_CONST_PICK_E(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, _C, _D, _E, ...) _E
#define DJB2_CONST_PICK_F(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _A, _B, _C, _D, _E, _F, ...) _F
#define DJB2_CONST_F(...)           (DJB2_CONST_PICK_E(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_E(__VA_ARGS__))
#define DJB2_CONST_E(...)           (DJB2_CONST_PICK_D(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_D(__VA_ARGS__))
#define DJB2_CONST_D(...)           (DJB2_CONST_PICK_C(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_C(__VA_ARGS__))
#define DJB2_CONST_C(...)           (DJB2_CONST_PICK_B(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_B(__VA_ARGS__))
#define DJB2_CONST_B(...)           (DJB2_CONST_PICK_A(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_A(__VA_ARGS__))
#define DJB2_CONST_A(...)           (DJB2_CONST_PICK_9(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_9(__VA_ARGS__))
#define DJB2_CONST_9(...)           (DJB2_CONST_PICK_8(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_8(__VA_ARGS__))
#define DJB2_CONST_8(...)           (DJB2_CONST_PICK_7(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_7(__VA_ARGS__))
#define DJB2_CONST_7(...)           (DJB2_CONST_PICK_6(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_6(__VA_ARGS__))
#define DJB2_CONST_6(...)           (DJB2_CONST_PICK_5(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_5(__VA_ARGS__))
#define DJB2_CONST_5(...)           (DJB2_CONST_PICK_4(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_4(__VA_ARGS__))
#define DJB2_CONST_4(...)           (DJB2_CONST_PICK_3(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_3(__VA_ARGS__))
#define DJB2_CONST_3(...)           (DJB2_CONST_PICK_2(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_2(__VA_ARGS__))
#define DJB2_CONST_2(...)           (DJB2_CONST_PICK_1(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_1(__VA_ARGS__))
#define DJB2_CONST_1(...)           (DJB2_CONST_PICK_0(__VA_ARGS__) + UINT64_C(33) * DJB2_CONST_0(__VA_ARGS__))
#define DJB2_CONST_0(...)           UINT64_C(5381)

/* testing */
#if DJB2_CONST('l','e','f','t') != UINT64_C(0x17C9A03B0) || \
    DJB2_CONST('r','i','g','h','t') != UINT64_C(0x3110494163) || \
    DJB2_CONST('m','i','d','d','l','e') != UINT64_C(0x6530DC5EBD4)
#error DJB2
#endif

#endif
