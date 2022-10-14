/*
 * Open Surge Engine
 * logfile.c - logfile module
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

#include <allegro5/allegro.h>
#include <surgescript.h>
#include <stdio.h>
#include <stdarg.h>
#include "logfile.h"
#include "global.h"
#include "asset.h"


/* private stuff ;) */
static const char* a5_version_string();
static const char* LOGFILE_PATH = "logfile.txt"; /* default log file */
static ALLEGRO_FILE* logfile = NULL;


/*
 * logfile_init()
 * Initializes the logfile module.
 */
void logfile_init()
{
    const char* fullpath = asset_path(LOGFILE_PATH);
    char tmp_path[4096];

    if(NULL == (logfile = al_fopen(fullpath, "wb"))) { /* physfs uses binary mode */
        fprintf(stderr, "Can't open logfile at %s\n", fullpath);
        return;
    }

    /* initial messages */
    logfile_message("%s version %s", GAME_TITLE, GAME_VERSION_STRING);
    logfile_message("Using Allegro version %s", a5_version_string());
    logfile_message("Using SurgeScript version %s", surgescript_util_version());
    logfile_message("Asset directory: %s", asset_shared_datadir(tmp_path, sizeof(tmp_path)));
    logfile_message("User directory: %s", asset_user_datadir(tmp_path, sizeof(tmp_path)));
}


/*
 * logfile_message()
 * Prints a message to the logfile (printf format)
 */
void logfile_message(const char* fmt, ...)
{
    va_list args;

    if(logfile == NULL)
        return;

    va_start(args, fmt);
    al_vfprintf(logfile, fmt, args);
    va_end(args);

#ifdef _WIN32
    /* "PhysFS does not support the text-mode reading and writing,
       which means that Windows-style newlines will not be preserved."
       https://liballeg.org/a5docs/trunk/physfs.html */
    al_fputs(logfile, "\r\n");
#else
    al_fputs(logfile, "\n");
#endif

    al_fflush(logfile);
}



/* 
 * logfile_release()
 * Releases the logfile module
 */
void logfile_release()
{
    logfile_message("tchau!");
    
    if(logfile != NULL)
        al_fclose(logfile);

    logfile = NULL;
}






/* private */



/*
 * a5_version_string()
 * Returns the compiled Allegro version as a static char[]
 */
const char* a5_version_string()
{
    static char str[17];
    uint32_t version = al_get_allegro_version();

    snprintf(str, sizeof(str), "%u.%u.%u-%u",
        (version & 0xFF000000) >> 24,
        (version & 0xFF0000) >> 16,
        (version & 0xFF00) >> 8,
        (version & 0xFF)
    );

    return str;
}