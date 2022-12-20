/*
 * Open Surge Engine
 * util.h - utilities
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _UTIL_H
#define _UTIL_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "global.h"

/* redefinitions */
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifdef round
#undef round
#endif

#ifdef PI
#undef PI
#endif

#define PI                      3.14159265358979323846
#define LARGE_INT               (1 << 30)

/* Useful macros */
#define random(n)               (int)(rand()/(((double)RAND_MAX+1)/(n)))
#define min(a,b)                ((a)<(b)?(a):(b))
#define max(a,b)                ((a)>(b)?(a):(b))
#define clip(val,a,b)           (((val)<(a) && (val)<(b)) ? min((a),(b)) : (((val)>(a) && (val)>(b)) ? max((a),(b)) : (val)))
#define clip01(val)             ((val) - ((val)<0.0f)*(val) + ((val)>1.0f)*(1.0f-(val))) /* clip((val), 0.0f, 1.0f) */
#define atob(str)               ((str_icmp((str), "true") == 0) || (str_icmp((str), "yes") == 0))
#define bounding_box(a,b)       ((a)[0]<(b)[2] && (a)[2]>(b)[0] && (a)[1]<(b)[3] && (a)[3]>(b)[1]) /* a[4],b[4] = (x,y,x+w,y+h) */
#define sign(x)                 copysignf(1.0f, (x))
#define nearly_zero(x)          (fabs(x)<=0.00001f)
#define nearly_equal(a,b)       (fabs((a)-(b))<=0.00001f*max(fabs(a),fabs(b)))
#define mallocx(bytes)          __mallocx((bytes), __FILE__ ":" STRINGIFY(__LINE__))
#define reallocx(ptr,bytes)     __reallocx((ptr), (bytes), __FILE__ ":" STRINGIFY(__LINE__))

/* Memory management */
void* __mallocx(size_t bytes, const char* location);
void* __reallocx(void *ptr, size_t bytes, const char* location);

/* General utilities */
int game_version_compare(int sup_version, int sub_version, int wip_version); /* compare to this version of the game engine */
void fatal_error(const char *fmt, ...); /* crash the program with a message */
void merge_sort(void *base, int num, size_t size, int (*comparator)(const void*,const void*)); /* similar to stdlib's qsort, but merge_sort is a stable sorting algorithm */
float lerp(float a, float b, float t); /* linear interpolation */
float lerp_angle(float alpha, float beta, float t); /* alpha, beta in radians */
uint64_t random64(); /* pseudo-random 64-bit number */
FILE* fopen_utf8(const char* filepath, const char* mode); /* fopen() with UTF-8 filename support */

#endif
