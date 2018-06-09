/*
 * Open Surge Engine
 * util.h - utilities
 * Copyright (C) 2008-2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <time.h>
#include <stdlib.h>
#include "global.h"
#include "v2d.h"



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

#ifdef swap
#undef swap
#endif


/* Useful macros */
#define randomize()             (srand(time(NULL)))
#define random(n)               ((int)(((float)rand() / ((float)(RAND_MAX)+(float)(1)))*(n)))
#define min(a,b)                ((a)<(b)?(a):(b))
#define max(a,b)                ((a)>(b)?(a):(b))
#define sign(x)                 (((x)>=0.0f)?(1.0f):(-1.0f))
#define round(x)                ((int)(((x)>0.0f)?((x)+0.5f):((x)-0.5f)))
#define clip(val,a,b)           ( ((val)<(a) && (val)<(b)) ? min((a),(b)) : ( ((val)>(a) && (val)>(b)) ? max((a),(b)) : (val)  ) )
#define swap(a,b)               swap_ex(&(a), &(b), sizeof(a))
#define atob(str)               ((str_icmp(str, "true") == 0) || (str_icmp(str, "yes") == 0))


/* Game routines */
void game_quit(void); /* quit */
int game_is_over(); /* game over? */
int game_version_compare(int version, int sub_version, int wip_version); /* compare to this version of the game */



/* Memory management */
void* mallocx(size_t bytes);
void* reallocx(void *ptr, size_t bytes);




/* Misc */
int bounding_box(float a[4], float b[4]); /* rect[4] = x1, y1, x2(=x1+w), y2(=y1+h) */
int circular_collision(v2d_t a, float r_a, v2d_t b, float r_b);
void swap_ex(void *a, void *b, size_t size);
void fatal_error(const char *fmt, ...);
float old_school_angle(float angle);
void merge_sort(void *base, size_t num, size_t size, int (*comparator)(const void*,const void*)); /* similar to stdlib's qsort, but merge_sort is a stable sorting algorithm */


#endif
