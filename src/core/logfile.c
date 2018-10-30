/*
 * Open Surge Engine
 * logfile.c - logfile module
 * Copyright (C) 2010, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include <stdarg.h>
#include "logfile.h"
#include "global.h"
#include "assetfs.h"


/* private stuff ;) */
#define LOGFILE_PATH        "logfile.txt" /* default log file */
static FILE *logfile = NULL;


/*
 * logfile_init()
 * Initializes the logfile module.
 */
void logfile_init()
{
    const char* fullpath = assetfs_create_cache_file(LOGFILE_PATH);
    if(NULL != (logfile = fopen(fullpath, "w"))) {
        logfile_message("%s version %s", GAME_TITLE, GAME_VERSION_STRING);
        logfile_message("logfile_init()");
    }
    else {
        logfile_message("%s version %s", GAME_TITLE, GAME_VERSION_STRING);
        logfile_message("logfile_init(): couldn't open \"%s\" for writing.", LOGFILE_PATH);
    }
}


/*
 * logfile_message()
 * Prints a message to the logfile (printf format)
 */
void logfile_message(const char *fmt, ...)
{
    if(logfile != NULL) {
        va_list args;

        va_start(args, fmt);
        vfprintf(logfile, fmt, args);
        va_end(args);

        fputc('\n', logfile);
        fflush(logfile);
    }
}



/* 
 * logfile_release()
 * Releases the logfile module
 */
void logfile_release()
{
    logfile_message("logfile_release()");
    logfile_message("tchau!");
    if(logfile != NULL)
        fclose(logfile);
}

