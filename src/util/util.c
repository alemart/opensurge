/*
 * Open Surge Engine
 * util.c - utilities
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

#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <surgescript.h>
#include <physfs.h>
#include "util.h"
#include "stringutil.h"
#include "v2d.h"
#include "../core/global.h"
#include "../core/timer.h"
#include "../core/video.h"
#include "../core/logfile.h"
#include "../core/config.h"
#include "../core/asset.h"
#include "../core/lang.h"
#include "../core/resourcemanager.h"

#if defined(__ANDROID__)
#define ALLEGRO_UNSTABLE /* required for al_android_get_jni_env(), al_android_get_activity() */
#include <allegro5/allegro.h>
#include <allegro5/allegro_android.h>
#include <allegro5/allegro_native_dialog.h>
#include <android/log.h>
#include <setjmp.h>
#else
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#endif

#if defined(_WIN32)
#include <windows.h>
#include <wchar.h>
#include <sys/types.h>
#include <sys/stat.h> /* _stat */
#include <direct.h> /* _mkdir */
#else
#include <sys/stat.h>
#endif

/* require alloca */
#if !(defined(__APPLE__) || defined(MACOSX) || defined(macintosh) || defined(Macintosh))
#include <malloc.h>
#if defined(__linux__) || defined(__linux) || defined(__EMSCRIPTEN__)
#include <alloca.h>
#endif
#endif

#if 1
#define ALLOCA_THRESHOLD 4096
#else
#define ALLOCA_THRESHOLD 1024 /* _ALLOCA_S_THRESHOLD */
#endif

/* private stuff */
static void merge_sort_recursive(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q, uint8_t *tmp, size_t tmp_size);
static inline void merge_sort_mix(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q, int m, uint8_t *tmp, size_t tmp_size);
static int wrapped_mkdir(const char* path, mode_t mode);




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
#endif

    /* al_show_native_message_box may be called without Allegro being initialized.
       https://liballeg.org/a5docs/trunk/native_dialog.html#al_show_native_message_box */
    al_show_native_message_box(al_get_current_display(),
        "Surgexception Error",
        "Ooops... Surgexception!",
        buf,
    NULL, ALLEGRO_MESSAGEBOX_ERROR);

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
    extern jmp_buf main_jump_buffer;
    longjmp(main_jump_buffer, 1);
#else
    exit(1);
#endif
}

/*
 * alert()
 * Displays a message box with an OK button
 */
void alert(const char* fmt, ...)
{
    char buf[1024];
    va_list args;

    /* format message */
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* log */
    logfile_message("<< %s >> %s", __func__, buf);

    /* show message box */
    /* al_show_native_message_box may be called without Allegro being initialized.
       https://liballeg.org/a5docs/trunk/native_dialog.html#al_show_native_message_box */
    al_show_native_message_box(al_get_current_display(),
        GAME_TITLE, GAME_TITLE, buf, NULL, ALLEGRO_MESSAGEBOX_WARN);
}

/*
 * confirm()
 * Displays a message box with Yes/No buttons
 */
bool confirm(const char* fmt, ...)
{
    char buf[1024];
    int result;
    va_list args;

    /* format message */
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* log */
    logfile_message("<< %s >> %s", __func__, buf);

    /* translate yes/no buttons */
    char buttons[32] = "";
    if(lang_haskey("OPTIONS_YES") && lang_haskey("OPTIONS_NO")) {
        char yes[16], no[16];
        lang_getstring("OPTIONS_YES", yes, sizeof(yes));
        lang_getstring("OPTIONS_NO", no, sizeof(no));
        snprintf(buttons, sizeof(buttons), "%s|%s", yes, no);
    }

    /* show message box */
    /* al_show_native_message_box may be called without Allegro being initialized.
       https://liballeg.org/a5docs/trunk/native_dialog.html#al_show_native_message_box */
    result = al_show_native_message_box(al_get_current_display(),
        GAME_TITLE, GAME_TITLE, buf, *buttons ? buttons : NULL, ALLEGRO_MESSAGEBOX_YES_NO | ALLEGRO_MESSAGEBOX_WARN);

    /* log result */
    logfile_message("<< %s >> result: %d", __func__, result);

    /* done! */
    return 1 == result;
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
 * fopen() with support for UTF-8 filenames
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
 * file_exists()
 * Checks if a regular file exists, given its absolute path
 */
bool file_exists(const char *filepath)
{
#if !defined(_WIN32)

    struct stat st;
    return (stat(filepath, &st) == 0) && S_ISREG(st.st_mode);

#elif 1

    struct _stat st;
    return (_stat(filepath, &st) == 0) && ((st.st_mode & _S_IFMT) == _S_IFREG);

#else

    FILE* fp = fopen(filepath, "rb");
    bool valid = (fp != NULL);
    if(fp != NULL)
        fclose(fp);
    return valid;

#endif
}

/*
 * directory_exists()
 * Checks if a directory exists, given its absolute path
 */
bool directory_exists(const char* dirpath)
{
    bool exists;
    char* tmp = NULL;

    /* There must not be a trailing directory separator on the path */
    int len = strlen(dirpath);
    if(len > 0 && strspn(dirpath + (len - 1), "/\\") > 0) {
        tmp = str_dup(dirpath);
        tmp[len-1] = '\0';
        dirpath = tmp;
    }

#if !defined(_WIN32)

    struct stat st;
    exists = (stat(dirpath, &st) == 0) && S_ISDIR(st.st_mode);

#else

    /*

    If path contains the location of a directory, it cannot contain a trailing
    backslash. If it does, -1 will be returned and errno will be set to ENOENT.

    https://learn.microsoft.com/en-us/previous-versions/visualstudio/visual-studio-2013/14h5k7ff(v=vs.120)

    */
    struct _stat st;
    exists = (_stat(dirpath, &st) == 0) && ((st.st_mode & _S_IFMT) == _S_IFDIR);

#endif

    if(tmp != NULL)
        free(tmp);

    /* done! */
    return exists;
}

/*
 * mkpath()
 * A variant of mkdir() that creates a path by creating directories as needed.
 * filepath is an absolute path. If the path to a directory is specified, then
 * that path must be terminated with a directory separator '/' (or with a '\\'
 * on Windows). Any file name is ignored. Returns 0 on success, -1 on failure.
 */
int mkpath(const char* filepath, uint32_t mode)
{
    const char SLASH = ALLEGRO_NATIVE_PATH_SEP;
    char path[4096], *p;

    /* sanity check */
    if(filepath == NULL || *filepath == '\0')
        return 0;

    /* copy the filepath to a local variable */
    size_t len = strlen(filepath);
    if(len+1 > sizeof(path)) {
        /*errno = ENAMETOOLONG;*/
        logfile_message("Can't mkpath \"%s\": name too long", filepath);
        return -1;
    }
    memcpy(path, filepath, len+1);

#if defined(_WIN32)
    /* skip volume; begin as \folder1\folder2\file... */
    p = strstr(path, ":\\"); /* try a traditional DOS path */
    if(p == NULL) {
        if(strncmp(path, "\\\\", 2) == 0 && isalnum(path[2])) { /* try a UNC path */
            p = strchr(path+2, '\\');
            if(p == NULL) {
                logfile_message("Can't mkpath \"%s\": invalid path", path);
                return -1;
            }
        }
        else {
            logfile_message("Can't mkpath \"%s\": not an absolute path", path);
            return -1;
        }
    }
    else
        p = p+1;
#else
    /* ensure we have an absolute filepath */
    if(path[0] != '/') {
        logfile_message("Can't mkpath \"%s\": not an absolute path", path);
        return -1;
    }

    /* begin as /folder1/folder2/file... */
    p = path;
#endif

    /* make path */
    for(p = strchr(p+1, SLASH); p != NULL; p = strchr(p+1, SLASH)) {
        *p = '\0';
        if(!directory_exists(path)) {
            if(wrapped_mkdir(path, (mode_t)mode) != 0) {
                *p = SLASH;
                logfile_message("Can't mkpath \"%s\": %s", path, strerror(errno));
                return -1;
            }
        }
        *p = SLASH;
    }

    /* success */
    return 0;
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

    return VERSION_CODE_EX(a, b, c, d);
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
    static char _buf[32];

    if(buffer == NULL) {
        buffer = _buf;
        buffer_size = sizeof(_buf);
    }

    if(version_code < 0)
        version_code = 0;
    else if(version_code > MAX_VERSION)
        version_code = MAX_VERSION;

    x = version_code / 1000000;
    y = (version_code - x * 1000000) / 10000;
    z = (version_code - x * 1000000 - y * 10000) / 100;
    w = (version_code - x * 1000000 - y * 10000 - z * 100);
    assertx(version_code == VERSION_CODE_EX(x, y, z, w));

    if(w != 0)
        snprintf(buffer, buffer_size, "%d.%d.%d.%d", x, y, z, w);
    else
        snprintf(buffer, buffer_size, "%d.%d.%d", x, y, z);

    return buffer;
}

/*
 * opensurge_game_name()
 * The name of the game / MOD that is running on the engine
 */
const char* opensurge_game_name()
{
    return config_game_title("Untitled game");
}

/*
 * opensurge_game_version()
 * The version of the game / MOD that is running on the engine
 */
const char* opensurge_game_version()
{
    return config_game_version("0.0.0");
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
    uint8_t* stack_mem = NULL;
    uint8_t *heap_mem = NULL;
    uint8_t *tmp = NULL;
    size_t total_size = (size_t)num * size;

    if(total_size > ALLOCA_THRESHOLD)
        tmp = heap_mem = mallocx(total_size);
    else if(total_size > 0)
        tmp = stack_mem = alloca(total_size);

    merge_sort_recursive(base, size, comparator, 0, num-1, tmp, total_size);

    if(heap_mem != NULL)
        free(heap_mem);
}





/*
 *
 * private stuff
 *
 */

void merge_sort_recursive(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q, uint8_t *tmp, size_t tmp_size)
{
    if(q > p) {
        int m = (p+q) / 2;

        merge_sort_recursive(base, size, comparator, p, m, tmp, tmp_size);
        merge_sort_recursive(base, size, comparator, m+1, q, tmp, tmp_size);
        merge_sort_mix(base, size, comparator, p, q, m, tmp, tmp_size);
    }
}

void merge_sort_mix(void *base, size_t size, int (*comparator)(const void*,const void*), int p, int q, int m, uint8_t *tmp, size_t tmp_size)
{
    uint8_t *arr = tmp;
    uint8_t *i = arr;
    uint8_t *j = arr + (m+1-p) * size;
    int k = p;

    assertx(tmp_size >= (q-p+1) * size); /* <-- alloca() */
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
}

int wrapped_mkdir(const char* path, mode_t mode)
{
#if defined(_WIN32)

    (void)mode;
    return _mkdir(path);

#elif defined(__ANDROID__)

    /* got permission denied errors when using standard mkdir() on the
       application cache on Android, so we resort to the File API on Java */
    struct stat st;
    (void)mode;

    if(stat(path, &st) == 0) {
        errno = EEXIST;
        return -1;
    }

    JNIEnv* env = al_android_get_jni_env();
    jobject activity = al_android_get_activity();

    jclass class_id = (*env)->GetObjectClass(env, activity);
    jmethodID method_id = (*env)->GetMethodID(env, class_id, "mkdir", "(Ljava/lang/String;)Z");

    jstring jpath = (*env)->NewStringUTF(env, path);
    jboolean jsuccess = (*env)->CallBooleanMethod(env, activity, method_id, jpath);
    (*env)->DeleteLocalRef(env, jpath);

    (*env)->DeleteLocalRef(env, class_id);

    if(!jsuccess) {
        errno = EACCES; /* XXX? */
        return -1;
    }

    return 0;

#else

    return mkdir(path, mode);

#endif
}

/* Are we in a Smart TV? */
bool is_tv_device()
{
#if defined(__ANDROID__)

    JNIEnv* env = al_android_get_jni_env();
    jobject activity = al_android_get_activity();

    jclass class_id = (*env)->GetObjectClass(env, activity);
    jmethodID method_id = (*env)->GetMethodID(env, class_id, "isTVDevice", "()Z");

    jboolean result = (*env)->CallBooleanMethod(env, activity, method_id);

    (*env)->DeleteLocalRef(env, class_id);
    return result;

#else

    return false;

#endif
}