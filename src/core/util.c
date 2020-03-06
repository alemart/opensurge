/*
 * Open Surge Engine
 * util.c - utilities
 * Copyright (C) 2008-2010, 2018-2020  Alexandre Martins <alemartf@gmail.com>
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "v2d.h"
#include "util.h"
#include "timer.h"
#include "logfile.h"
#include "resourcemanager.h"

#if defined(A5BUILD)
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#elif defined(_WIN32)
#include <allegro.h>
#include <winalleg.h>
#else
#include <allegro.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#include <wchar.h>
#endif


/* private stuff */
#if defined(A5BUILD)
static bool game_over = false;
#else
static volatile int game_over = FALSE;
#endif
static void merge_sort_recursive(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q);
static inline void merge_sort_mix(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q, int m);




/* Memory management */


/*
 * __mallocx()
 * Similar to malloc(), but aborts the
 * program if it does not succeed.
 */
void *__mallocx(size_t bytes, const char* location)
{
    void *p = malloc(bytes);

    if(!p)
        fatal_error("Out of memory in mallocx(%u) at %s", bytes, location);

    return p;
}


/*
 * __relloacx()
 * Similar to realloc(), but abots the
 * program if it does not succeed.
 */
void* __reallocx(void *ptr, size_t bytes, const char* location)
{
    void *p = realloc(ptr, bytes);

    if(!p)
        fatal_error("Out of memory in reallocx(%u) at %s", bytes, location);

    return p;
}



/* Game routines */

/*
 * game_quit()
 * Quit game?
 */
#if defined(A5BUILD)
void game_quit(void)
{
    game_over = true;
}
#else
void game_quit(void)
{
    game_over = TRUE;
}
END_OF_FUNCTION(game_quit)
#endif


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
int game_version_compare(int sup_version, int sub_version, int wip_version)
{
    int version_code = VERSION_CODE(max(0, sup_version), max(0, sub_version), max(0, wip_version));
    return GAME_VERSION_CODE - version_code;
}


/* Misc */

/*
 * fatal_error()
 * Displays a fatal error and exits the application
 * (printf() format)
 */
void fatal_error(const char *fmt, ...)
{
    char buf[1024];
    va_list args;

    /* format message */
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* display an error */
    logfile_message("----- crash -----\n%s", buf);
#if defined(A5BUILD)
    al_show_native_message_box(al_get_current_display(),
        "Surgexception Error",
        "Ooops... Surgexception!",
        buf,
    NULL, ALLEGRO_MESSAGEBOX_ERROR);
#else
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
#ifdef _WIN32
    MessageBoxA(NULL, buf, GAME_TITLE, MB_OK | MB_ICONERROR);
#else
    allegro_message("%s", buf);
#endif
#endif

    /* clear up resources */
    if(resourcemanager_is_initialized())
        resourcemanager_release();

    /* exit */
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
    v2d_t a = v2d_new(cosf(alpha), sinf(alpha));
    v2d_t b = v2d_new(cosf(beta), sinf(beta));
    v2d_t c = v2d_lerp(a, b, t);
    return atan2f(c.y, c.x);
}

/*
 * random64()
 * xorshift random number generator
 */
uint64_t random64()
{
    static uint64_t state = 0;

    /* generate seed: wang hash */
    if(!state) {
        state = time(NULL);
        state = (~state) + (state << 21);
        state ^= state >> 24;
        state = (state + (state << 3)) + (state << 8);
        state ^= state >> 14;
        state = (state + (state << 2)) + (state << 4);
        state ^= state >> 28;
        state += state << 31;
    }

    /* xorshift */ 
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

/*
 * fopen_utf8()
 * fopen() with UTF-8 support for filenames
 */
FILE* fopen_utf8(const char* filepath, const char* mode)
{
#if defined(_WIN32)
    FILE* fp;
    int wpath_size = MultiByteToWideChar(CP_UTF8, 0, filepath, -1, NULL, 0);
    int wmode_size = MultiByteToWideChar(CP_UTF8, 0, mode, -1, NULL, 0);

    if(wpath_size > 0 && wmode_size > 0) {
        wchar_t* wpath = mallocx(wpath_size * sizeof(*wpath));
        wchar_t* wmode = mallocx(wmode_size * sizeof(*wmode));

        MultiByteToWideChar(CP_UTF8, 0, filepath, -1, wpath, wpath_size);
        MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, wmode_size);
        fp = _wfopen(wpath, wmode);

        free(wmode);
        free(wpath);
    }
    else {
        logfile_message("%s(\"%s\", \"%s\") ERROR %d", __func__, filepath, mode, GetLastError());
        fp = fopen(filepath, mode);
    }

    return fp;
#else
    return fopen(filepath, mode);
#endif
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
    /* due to the static array declared as an optimization below,
       merge_sort() isn't thread-safe */
    static uint8_t tmp[16384]; size_t bytes = (q-p+1) * size;
    uint8_t *arr = bytes > sizeof(tmp) ? mallocx(bytes) : tmp;
    uint8_t *i = arr;
    uint8_t *j = arr + (m+1-p) * size;
    int k = p;

    memcpy(arr, (uint8_t*)base + p * size, (q-p+1) * size);

    while(i < arr + (m+1-p) * size && j <= arr + (q-p) * size) {
        if(comparator((const void*)i, (const void*)j) <= 0) {
            memcpy((uint8_t*)base + (k++) * size, i, size);
            i += size;
        }
        else {
            memcpy((uint8_t*)base + (k++) * size, j, size);
            j += size;
        }
    }

    while(i < arr + (m+1-p) * size) {
        memcpy((uint8_t*)base + (k++) * size, i, size);
        i += size;
    }

    while(j <= arr + (q-p) * size) {
        memcpy((uint8_t*)base + (k++) * size, j, size);
        j += size;
    }

    if(bytes > sizeof(tmp))
        free(arr);
}

