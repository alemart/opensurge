/*
 * Open Surge Engine
 * util.c - utilities
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

#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include "v2d.h"
#include "util.h"
#include "timer.h"
#include "logfile.h"



/* private stuff */
static volatile int game_over = FALSE;
static void merge_sort_recursive(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q);
static void merge_sort_mix(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q, int m);




/* Memory management */


/*
 * mallocx()
 * Similar to malloc(), but aborts the
 * program if it does not succeed.
 */
void *mallocx(size_t bytes)
{
    void *p = malloc(bytes);

    if(!p)
        fatal_error("FATAL ERROR: mallocx(%u) failed.\n", bytes);

    return p;
}


/*
 * relloacx()
 * Similar to realloc(), but abots the
 * program if it does not succeed.
 */
void* reallocx(void *ptr, size_t bytes)
{
    void *p = realloc(ptr, bytes);

    if(!p)
        fatal_error("FATAL ERROR: reallocx() failed.\n");

    return p;
}



/* Game routines */

/*
 * game_quit()
 * Quit game?
 */
void game_quit(void)
{
    game_over = TRUE;
}
END_OF_FUNCTION(game_quit)


/*
 * game_is_over()
 * Game over?
 */
int game_is_over()
{
    return game_over;
}


/*
 * game_version_compare()
 * Compares the given parameters to the version
 * of the game. Returns:
 * < 0 (game version is inferior),
 * = 0 (same version),
 * > 0 (game version is superior)
 */
int game_version_compare(int version, int sub_version, int wip_version)
{
    int game_version = (GAME_VERSION*10000 + GAME_SUB_VERSION*100 + GAME_WIP_VERSION);
    int other_version = (version*10000 + sub_version*100 + wip_version);

    return game_version - other_version;
}


/* Other */


/*
 * bounding_box()
 * bounding box collision method
 * r[4] = x1, y1, x2(=x1+w), y2(=y1+h)
 */
int bounding_box(float a[4], float b[4])
{
    return (a[0]<b[2] && a[2]>b[0] && a[1]<b[3] && a[3]>b[1]);
}



/*
 * circular_collision()
 * Circular collision method
 * a, b = points to test
 * r_a = radius of a
 * r_b = radius of b
 */
int circular_collision(v2d_t a, float r_a, v2d_t b, float r_b)
{
    return ( v2d_magnitude(v2d_subtract(a,b)) <= r_a + r_b );
}



/*
 * swap_ex()
 * Swaps two variables. Use the
 * swap() macro instead of this.
 */
void swap_ex(void *a, void *b, size_t size)
{
    uint8 *sa=a, *sb=b, c;
    size_t i;

    for(i=0; i<size; i++) {
        c = sa[i];
        sa[i] = sb[i];
        sb[i] = c;
    }
}





/*
 * fatal_error()
 * Displays a fatal error and aborts the application
 * (printf() format)
 */
void fatal_error(const char *fmt, ...)
{
    char buf[2048];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    logfile_message(buf);
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
    allegro_message("%s", buf);
    exit(1);
}


/*
 * lerp()
 * Linear interpolation from a to b
 * 0 <= t <= 1
 */
float lerp(float a, float b, float t)
{
    t = clip(t, 0.0f, 1.0f);
    return a + (b - a) * t;
}

/*
 * lerp_angle()
 * Linear interpolation from alpha to beta, both given in radians
 * 0 <= t <= 1
 */
float lerp_angle(float alpha, float beta, float t)
{
    v2d_t a = v2d_new(cos(alpha), sin(alpha));
    v2d_t b = v2d_new(cos(beta), sin(beta));
    v2d_t c = v2d_lerp(a, b, t);
    return atan2(c.y, c.x);
}


/*
 * merge_sort()
 * Similar to stdlib's qsort, but merge_sort is
 * a stable sorting algorithm
 *
 * base       - Pointer to the first element of the array to be sorted
 * num        - Number of elements in the array pointed by base
 * size       - Size in bytes of each element in the array
 * comparator - The return value of this function should represent
 *              whether elem1 is considered less than, equal to,
 *              or greater than elem2 by returning, respectively,
 *              a negative value, zero or a positive value
 *              int comparator(const void *elem1, const void *elem2)
 */
void merge_sort(void *base, size_t num, size_t size, int (*comparator)(const void*,const void*))
{
    merge_sort_recursive(base, size, comparator, 0, num-1);
}



/* private stuff */
void merge_sort_recursive(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q)
{
    if(q > p) {
        int m = (p+q)/2;
        merge_sort_recursive(base, size, comparator, p, m);
        merge_sort_recursive(base, size, comparator, m+1, q);
        merge_sort_mix(base, size, comparator, p, q, m);
    }
}

void merge_sort_mix(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q, int m)
{
    uint8 *arr = mallocx((q-p+1) * size);
    uint8 *i = arr;
    uint8 *j = arr + (m+1-p) * size;
    int k = p;

    memcpy(arr, (uint8*)base + p * size, (q-p+1) * size);

    while(i < arr + (m+1-p) * size && j <= arr + (q-p) * size) {
        if(comparator((const void*)i, (const void*)j) <= 0) {
            memcpy((uint8*)base + (k++) * size, i, size);
            i += size;
        }
        else {
            memcpy((uint8*)base + (k++) * size, j, size);
            j += size;
        }
    }

    while(i < arr + (m+1-p) * size) {
        memcpy((uint8*)base + (k++) * size, i, size);
        i += size;
    }

    while(j <= arr + (q-p) * size) {
        memcpy((uint8*)base + (k++) * size, j, size);
        j += size;
    }

    free(arr);
}

