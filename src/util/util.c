/*
 * Open Surge Engine
 * util.c - utilities
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <surgescript.h>
#include <physfs.h>
#include "util.h"
#include "stringutil.h"
#include "v2d.h"
#include "../core/global.h"
#include "../core/timer.h"
#include "../core/video.h"
#include "../core/logfile.h"
#include "../core/resourcemanager.h"

#if defined(__ANDROID__)
#define ALLEGRO_UNSTABLE /* required for al_android_get_jni_env(), al_android_get_activity() */
#include <allegro5/allegro.h>
#include <allegro5/allegro_android.h>
#include <android/log.h>
#else
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#include <wchar.h>
#endif

/* private stuff */
static void merge_sort_recursive(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q);
static inline void merge_sort_mix(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q, int m);

#if defined(__ANDROID__)
static void android_show_alert_dialog(const char* title, const char* message);
#endif




/* Memory management */


/*
 * __mallocx()
 * Similar to malloc(), but aborts the
 * program if it does not succeed.
 */
void *__mallocx(size_t bytes, const char* location, int line)
{
    void *p = malloc(bytes);

    if(!p)
        fatal_error("Out of memory in %s(%u) at %s:%d", __func__, bytes, location, line);

    return p;
}


/*
 * __relloacx()
 * Similar to realloc(), but abots the
 * program if it does not succeed.
 */
void* __reallocx(void *ptr, size_t bytes, const char* location, int line)
{
    void *p = realloc(ptr, bytes);

    if(!p)
        fatal_error("Out of memory in %s(%u) at %s:%d", __func__, bytes, location, line);

    return p;
}



/* General utilities */


/*
 * game_version_compare()
 * Compares the given parameters to the version
 * of the game. Returns:
 * < 0 (game version is less than),
 * = 0 (same version),
 * > 0 (game version is greater than)
 */
int game_version_compare(int sup_version, int sub_version, int wip_version)
{
    int version_code = VERSION_CODE(max(0, sup_version), max(0, sub_version), max(0, wip_version));
    return GAME_VERSION_CODE - version_code;
}

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
    logfile_message("----- crash -----");
    logfile_message("%s", buf);
    fprintf(stderr, "%s\n", buf);

#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_FATAL, GAME_UNIXNAME, "Surgexception Error: %s", buf);
    android_show_alert_dialog("Ooops... Surgexception!", buf);
#else
    /* al_show_native_message_box may be called without Allegro being initialized.
       https://liballeg.org/a5docs/trunk/native_dialog.html#al_show_native_message_box */
    al_show_native_message_box(al_get_current_display(),
        "Surgexception Error",
        "Ooops... Surgexception!",
        buf,
    NULL, ALLEGRO_MESSAGEBOX_ERROR);
#endif

    /* clear up resources */
    if(resourcemanager_is_initialized()) {
        /* this must only be called from the main thread,
           as it releases OpenGL textures */
        resourcemanager_release();
    }

    /* release Allegro */
    al_uninstall_system();

    /* exit */
#if defined(__ANDROID__)
    abort(); /* won't call atexit functions */
#else
    exit(1);
#endif
}


/*
 * random64()
 * xorshift random number generator
 */
uint64_t random64()
{
    static uint64_t state = 0;
    static bool seeded = false;

    /* generate seed: wang hash */
    if(!seeded) {
        state = time(NULL);
        state = (~state) + (state << 21);
        state ^= state >> 24;
        state = (state + (state << 3)) + (state << 8);
        state ^= state >> 14;
        state = (state + (state << 2)) + (state << 4);
        state ^= state >> 28;
        state += state << 31;
        seeded = true;
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
 * allegro_version_string()
 * Returns the compiled Allegro version as a string
 */
const char* allegro_version_string()
{
    static char str[16];
    uint32_t version = al_get_allegro_version();

    snprintf(str, sizeof(str), "%u.%u.%u-%u",
        (version & 0xFF000000) >> 24,
        (version & 0xFF0000) >> 16,
        (version & 0xFF00) >> 8,
        (version & 0xFF)
    );

    return str;
}

/*
 * surgescript_version_string()
 * The compiled version of SurgeScript as a string
 */
const char* surgescript_version_string()
{
    return surgescript_util_version();
}

/*
 * physfs_version_string()
 * The compiled version of PhysicsFS as a string
 */
const char* physfs_version_string()
{
    static char str[16];
    PHYSFS_Version version;

    PHYSFS_getLinkedVersion(&version);
    snprintf(str, sizeof(str), "%u.%u.%u", version.major, version.minor, version.patch);

    return str;
}

/*
 * parse_version_number()
 * Convert a "x.y.z[.w]" version string to a version code, which is
 * a comparable integer.
 */
int parse_version_number(const char* version_string)
{
    return parse_version_number_ex(version_string, NULL, NULL, NULL, NULL);
}

/*
 * parse_version_number_ex()
 * Convert a "x.y.z[.w]" version string to a version code, which is
 * a comparable integer. Output parameters x, y, w and w may be NULL
 */
int parse_version_number_ex(const char* version_string, int* x, int* y, int* z, int* w)
{
    char ver[16];
    int code[4] = { 0, 0, 0, 0 };

    /* copy the version string to a temporary buffer */
    str_cpy(ver, version_string, sizeof(ver));

    /* the string format may be of the form x.y.z[.w][-some_string] */
    for(char* h = ver; *h; h++) {
        if(!((*h >= '0' && *h <= '9') || (*h == '.'))) {
            *h = '\0';
            break;
        }
    }

    /* this will accept string format [x[.y[.z[.w]]]] */
    char* s = strtok(ver, ".");
    for(int j = 0; s != NULL && j < 4; ) {
        code[j++] = *s ? atoi(s) : 0;
        s = strtok(NULL, ".");
    }

    int a = clip(code[0], 0, 99);
    int b = clip(code[1], 0, 99);
    int c = clip(code[2], 0, 99);
    int d = clip(code[3], 0, 99);

    if(x != NULL) *x = a;
    if(y != NULL) *y = b;
    if(z != NULL) *z = c;
    if(w != NULL) *w = d;

    return a * 1000000 + b * 10000 + c * 100 + d;
}

/*
 * stringify_version_number()
 * Convert a version code to a version string of
 * the form x.y.z[.w]
 */
char* stringify_version_number(int version_code, char* buffer, size_t buffer_size)
{
    const int MAX_VERSION = 99 * 1000000 + 99 * 10000 + 99 * 100 + 99;
    int x, y, z, w;

    if(version_code < 0)
        version_code = 0;
    else if(version_code > MAX_VERSION)
        version_code = MAX_VERSION;

    x = version_code / 1000000;
    y = (version_code - x * 1000000) / 10000;
    z = (version_code - x * 1000000 - y * 10000) / 100;
    w = (version_code - x * 1000000 - y * 10000 - z * 100);

    if(w != 0)
        snprintf(buffer, buffer_size, "%d.%d.%d.%d", x, y, z, w);
    else
        snprintf(buffer, buffer_size, "%d.%d.%d", x, y, z);

    return buffer;
}

/*
 * opensurge_game_name()
 * The sanitized name of the game / MOD that is being run in the engine
 */
const char* opensurge_game_name()
{
    static char buffer[48];

    /* FIXME: is the title of the window always equal to the name of the game? */
    str_cpy(buffer, video_get_window_title(), sizeof(buffer));

    /* get rid of newlines */
    for(char* p = buffer; *p; p++) {
        if(*p == '\n' || *p == '\r')
            *p = ' ';
    }

    /* done! */
    return buffer;
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
void merge_sort(void *base, int num, size_t size, int (*comparator)(const void*,const void*))
{
    merge_sort_recursive(base, size, comparator, 0, num-1);
}





/*
 *
 * private stuff
 *
 */

void merge_sort_recursive(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q)
{
    if(q > p) {
        int m = (p+q) / 2;

        merge_sort_recursive(base, size, comparator, p, m);
        merge_sort_recursive(base, size, comparator, m+1, q);
        merge_sort_mix(base, size, comparator, p, q, m);
    }
}

void merge_sort_mix(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q, int m)
{
    /* due to the static array declared as an optimization below,
       merge_sort() isn't thread-safe */
    static uint8_t tmp[65536];
    size_t bytes = (q-p+1) * size;
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

    if(arr != tmp)
        free(arr);
}



#if defined(__ANDROID__)

/* Show a native alert dialog on Android */
void android_show_alert_dialog(const char* title, const char* message)
{
    /* See https://liballeg.org/a5docs/trunk/platform.html#al_android_get_jni_env */
    JNIEnv* env = al_android_get_jni_env();
    jobject activity = al_android_get_activity();

    jclass class_id = (*env)->GetObjectClass(env, activity);
    jmethodID method_id = (*env)->GetMethodID(env, class_id, "showAlertDialog", "(Ljava/lang/String;Ljava/lang/String;)V");

    jstring jtitle = (*env)->NewStringUTF(env, title);
    jstring jmessage = (*env)->NewStringUTF(env, message);
    (*env)->CallVoidMethod(env, activity, method_id, jtitle, jmessage);
    (*env)->DeleteLocalRef(env, jmessage);
    (*env)->DeleteLocalRef(env, jtitle);

    (*env)->DeleteLocalRef(env, class_id);
}

#endif
