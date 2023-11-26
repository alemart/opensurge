/*
 * Open Surge Engine
 * util.h - utilities
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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
#include <stdbool.h>

/* redefinitions */
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#ifndef STRINGIFY
#define STRINGIFY(x)            _STRINGIFY(x)
#define _STRINGIFY(x)           #x
#endif

#define LARGE_INT               (1 << 30)

/* Useful macros */
#define random(n)               (int)(rand()/(((double)RAND_MAX+1)/(n)))
#define min(a,b)                ((a)<(b)?(a):(b))
#define max(a,b)                ((a)>(b)?(a):(b))
#define clip(val,a,b)           (((val)<(a))?(a):(((val)>(b))?(b):(val))) /* we assume a <= b */
#define clip01(val)             ((val) - ((val)<0.0f)*(val) + ((val)>1.0f)*(1.0f-(val))) /* clip((val), 0.0f, 1.0f) */
#define bounding_box(a,b)       ((a)[0]<(b)[2] && (a)[2]>(b)[0] && (a)[1]<(b)[3] && (a)[3]>(b)[1]) /* legacy macro; a[4],b[4] = (x,y,x+w,y+h) */
#define mallocx(bytes)          __mallocx((bytes), __FILE__, __LINE__)
#define reallocx(ptr,bytes)     __reallocx((ptr), (bytes), __FILE__, __LINE__)
#define assertx(expr, ...)      do { if(!(expr)) fatal_error("In %s:%d (%s): assertion `%s` failed. %s", __FILE__, __LINE__, __func__, STRINGIFY(expr), STRINGIFY(__VA_ARGS__)); } while(0)

/* Memory management */
void* __mallocx(size_t bytes, const char* location, int line);
void* __reallocx(void *ptr, size_t bytes, const char* location, int line);

/* General utilities */
int game_version_compare(int sup_version, int sub_version, int wip_version); /* compare to this version of the game engine */
void fatal_error(const char *fmt, ...); /* crash the program with a message */
void alert(const char* fmt, ...); /* display a message box */
void merge_sort(void *base, int num, size_t size, int (*comparator)(const void*,const void*)); /* similar to stdlib's qsort, but merge_sort is a stable sorting algorithm */
uint64_t random64(); /* pseudo-random 64-bit number */
FILE* fopen_utf8(const char* filepath, const char* mode); /* fopen() with UTF-8 filename support */
bool file_exists(const char* filepath); /* checks if an absolute filepath exists */
const char* allegro_version_string(); /* version of the Allegro library */
const char* surgescript_version_string(); /* version of the SurgeScript runtime */
const char* physfs_version_string(); /* version of the PhysFS library */
int parse_version_number(const char* version_string); /* convert a "x.y.z[.w]" version string to a version code, which is a comparable integer */
int parse_version_number_ex(const char* version_string, int* x, int* y, int* z, int* w); /* convert a "x.y.z[.w]" version string to a version code and output (x,y,z,w) */
char* stringify_version_number(int version_code, char* buffer, size_t buffer_size); /* convert a version code to a version string of the form x.y.z[.w] */
const char* opensurge_game_name(); /* the sanitized name of the game / MOD that is being run in the engine */

#endif