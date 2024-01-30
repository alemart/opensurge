/*
 * Open Surge Engine
 * logfile.c - logfile module
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

#include <allegro5/allegro.h>
#include <stdio.h>
#include <stdarg.h>
#include "logfile.h"
#include "global.h"
#include "asset.h"
#include "../util/util.h"



#if defined(__ANDROID__)

#include <allegro5/allegro_android.h>
#include <android/log.h>
#include <unistd.h>

#elif defined(_WIN32)

#include <io.h>
#define LINE_BREAK "\r\n"

#else

#include <unistd.h>
#define LINE_BREAK "\n"

#endif



/* name of the logfile */
#define LOGFILE_NAME            "logfile.txt"

/* error macro */
#define ERROR(...)              fprintf(stderr, __VA_ARGS__)

/* this macro calls a function of unknown arity given as its first parameter,
   removing the preceding comma of __VA_ARGS__ when it expands to nothing */
#define CALL(...) KALL(__VA_ARGS__, CALLX, CALLX, CALL0)(__VA_ARGS__)
#define CALL0(fn) do { \
    if(logfile != NULL) { (fn)(logfile); } \
    if(console != NULL) { (fn)(console); } \
} while(0)
#define CALLX(fn, ...) do { \
    if(logfile != NULL) { (fn)(logfile, __VA_ARGS__); } \
    if(console != NULL) { (fn)(console, __VA_ARGS__); } \
} while(0)
#define KALL(_0, _1, _2, fn, ...) fn

/* private stuff ;) */
static ALLEGRO_FILE* logfile = NULL;
static bool open_logfile();
static void close_logfile();

static ALLEGRO_FILE* console = NULL;
static bool open_console();
static void close_console();

static ALLEGRO_MUTEX* mutex = NULL;





/* public */


/*
 * logfile_init()
 * Initializes the logfile module.
 */
void logfile_init(int flags)
{
    /* create a mutex */
    if(mutex == NULL)
        mutex = al_create_mutex();

#if !defined(__ANDROID__)
    /* open the output streams */
    if(flags & LOGFILE_TXT)
        open_logfile();

    if(flags & LOGFILE_CONSOLE)
        open_console();
#else
    (void)open_logfile;
    (void)open_console;
#endif

    /* initial messages */
    logfile_message("%s version %s", GAME_TITLE, GAME_VERSION_STRING);
    logfile_message("Using Allegro version %s", allegro_version_string());
    logfile_message("Using SurgeScript version %s", surgescript_version_string());
    logfile_message("Using PhysicsFS version %s", physfs_version_string());
#if defined(__ANDROID__)
    logfile_message("Android platform: %s", al_android_get_os_version());
#endif

    /* asset directories */
    if(asset_is_init()) {
        char tmp_path[4096];

        logfile_message("Asset directory: %s", asset_shared_datadir(tmp_path, sizeof(tmp_path)));
        logfile_message("User directory: %s", asset_user_datadir(tmp_path, sizeof(tmp_path)));
    }
}



/*
 * logfile_message()
 * Prints a message to the logfile (printf format)
 */
void logfile_message(const char* fmt, ...)
{
#if !defined(__ANDROID__)
#define LOCK(mutex)     do { if((mutex) != NULL) al_lock_mutex(mutex); } while(0)
#define UNLOCK(mutex)   do { if((mutex) != NULL) al_unlock_mutex(mutex); } while(0)

    LOCK(mutex);
    {
        va_list args;

        /* print the message */
        va_start(args, fmt);
        CALL(al_vfprintf, fmt, args);
        va_end(args);

        /* break line */
        CALL(al_fputs, LINE_BREAK);
        CALL(al_fflush);

        /* "PhysFS does not support the text-mode reading and writing,
            which means that Windows-style newlines will not be preserved."
            https://liballeg.org/a5docs/trunk/physfs.html */
    }
    UNLOCK(mutex);

#undef UNLOCK
#undef LOCK
#else

    va_list args;

    va_start(args, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, GAME_UNIXNAME, fmt, args);
    va_end(args);

    /* logging functions from the Android NDK are atomic according to:
       https://groups.google.com/g/android-ndk/c/lRG-wp1gQV0/m/cnpXcpjOBAAJ */

#endif
}



/* 
 * logfile_release()
 * Releases the logfile module
 */
void logfile_release(int flags)
{
    logfile_message("tchau!");

    if(flags & LOGFILE_TXT)
        close_logfile();

    if(flags & LOGFILE_CONSOLE)
        close_console();

    if(mutex != NULL) {
        al_destroy_mutex(mutex);
        mutex = NULL;
    }
}






/* private */


/*
 * open_logfile()
 * Opens the file to which we'll write the logs
 * The asset manager must be initialized
 * Returns true on success
 */
bool open_logfile()
{
    /* nothing to do */
    if(logfile != NULL)
        return false;

    /* check if the asset manager is initialized */
    if(!asset_is_init()) {
        ERROR("Can't open logfile: no virtual filesystem");
        return false;
    }

    /* open file */
    const char* fullpath = asset_path(LOGFILE_NAME);
    if(NULL == (logfile = al_fopen(fullpath, "wb"))) { /* physfs uses binary mode */
        ERROR("Can't open logfile at %s. errno = %d\n", fullpath, al_get_errno());
        return false;
    }

    /* success! */
    return true;
}

/*
 * close_logfile()
 * Closes the file to which we wrote the logs
 */
void close_logfile()
{
    if(logfile != NULL) {
        al_fclose(logfile);
        logfile = NULL;
    }
}

/*
 * open_console()
 * Initializes the console output
 * Returns true on success
 */
bool open_console()
{
    /* nothing to do */
    if(console != NULL)
        return false;

    /* a negative file descriptor may be returned in a Windows application without a console window;
       see https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fileno?view=msvc-170 */
    int fd = fileno(stdout);
    if(fd < 0)
        return false;

    /* duplicate the file descriptor and open a stream */
    fd = dup(fd); /* al_fclose() will close this file descriptor */
    if(NULL == (console = al_fopen_fd(fd, "wb"))) { /* physfs uses binary mode */
        ERROR("Can't use the console as the logfile. errno = %d\n", al_get_errno());
        close(fd);
        return false;
    }

    /* success! */
    return true;
}

/*
 * close_console()
 * Releases the console output
 */
void close_console()
{
    if(console != NULL) {
        al_fclose(console);
        console = NULL;
    }
}
