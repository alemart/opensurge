/*
 * Open Surge Engine
 * logfile.c - logfile module
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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
#include "osspec.h"


/* private stuff ;) */
#define LOGFILE_PATH        "logfile.txt" /* default log file */
static FILE *logfile;


/*
 * logfile_init()
 * Initializes the logfile module.
 */
void logfile_init()
{
    char abs_path[1024];

    resource_filepath(abs_path, LOGFILE_PATH, sizeof(abs_path), RESFP_WRITE);

    if(NULL == (logfile = fopen(abs_path, "w")))
        logfile_message("WARNING: couldn't open %s for writing.\n", LOGFILE_PATH);
    else {
        logfile_message("%s version %s", GAME_TITLE, GAME_VERSION_STRING);
        logfile_message("logfile_init()");
    }
}


/*
 * logfile_message()
 * Prints a message on the logfile
 * (printf() format)
 */
void logfile_message(const char *fmt, ...)
{
    char buf[2048];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

    fprintf(logfile ? logfile : stderr, "%s\n", buf);
    fflush(logfile ? logfile : stderr);
}



/* 
 * logfile_release()
 * Releases the logfile module
 */
void logfile_release()
{
    logfile_message("logfile_release()\ntchau!");
    if(logfile)
        fclose(logfile);
}

