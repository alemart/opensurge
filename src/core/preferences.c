/*
 * Open Surge Engine
 * preferences.c - user preferences (saved in a file)
 * Copyright (C) 2010, 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "preferences.h"
#include "osspec.h"
#include "stringutil.h"
#include "logfile.h"
#include "lang.h"
#include "video.h"

/* constants */
#define PREFERENCES_FILE                            "preferences.dat"
#define PREFERENCES_SIGNATURE                       "OSPREF03"


/* file structure */
/* NOTE: if you change the file structure, you'll need to
 * change the signature as well (as that implies a new
 * version of this structure) */
struct {

    /* file signature */
    char signature[15];

    /* preferences */
    int videoresolution;
    int fullscreen;
    int smooth;
    int showfps;
    char languagepath[1024];
    int usegamepad;

} data;


/* private methods */
static const char* get_preferences_fullpath(); /* full filepath of the preferences file */
static void set_defaults(); /* set default preferences */
static int load(); /* load preferences from disk */
static void save(); /* save preferences to disk */



/*
 * preferences_init()
 * Initializes this module.
 * Returns TRUE if a previous preferences file exists, FALSE otherwise
 */
int preferences_init()
{
    logfile_message("preferences_init()");
    return load();
}


/* accessors and mutators */
int preferences_file_exists() { return filepath_exists(get_preferences_fullpath()); }
int preferences_get_videoresolution() { return data.videoresolution; }
void preferences_set_videoresolution(int resolution) { data.videoresolution = resolution; save(); }
int preferences_get_fullscreen() { return data.fullscreen; }
void preferences_set_fullscreen(int fullscreen) { data.fullscreen = fullscreen; save(); }
int preferences_get_smooth() { return data.smooth; }
void preferences_set_smooth(int smooth) { data.smooth = smooth; save(); }
int preferences_get_showfps() { return data.showfps; }
void preferences_set_showfps(int showfps) { data.showfps = showfps; save(); }
const char *preferences_get_languagepath() { return data.languagepath; }
void preferences_set_languagepath(const char *filepath) { str_cpy(data.languagepath, filepath, sizeof(data.languagepath)); save(); }
int preferences_get_usegamepad() { return data.usegamepad; }
void preferences_set_usegamepad(int usegamepad) { data.usegamepad = usegamepad; save(); }



/*
 * get_preferences_fullpath()
 * Returns the full filepath of the preferences file
 */
const char* get_preferences_fullpath()
{
    static char abs_path[1024] = "";

    /* we need WRITE privileges */
    if(strcmp(abs_path, "") == 0)
        resource_filepath(abs_path, PREFERENCES_FILE, sizeof(abs_path), RESFP_WRITE);

    return abs_path;
}



/*
 * set_defaults()
 * Sets defaults preferences
 */
void set_defaults()
{
    str_cpy(data.signature, PREFERENCES_SIGNATURE, sizeof(data.signature));
    data.videoresolution = VIDEORESOLUTION_2X;
    data.fullscreen = FALSE;
    data.smooth = FALSE;
    data.showfps = FALSE;
    data.usegamepad = FALSE;
    str_cpy(data.languagepath, DEFAULT_LANGUAGE_FILEPATH, sizeof(data.languagepath));
}


/*
 * load()
 * Load settings from disk. Returns TRUE on success, FALSE otherwise
 */
int load()
{
    FILE *fp = fopen(get_preferences_fullpath(), "rb");

    if(fp != NULL) {
        /* reading data */
        int n = fread(&data, sizeof(data), 1, fp);
        fclose(fp);

        /* invalid signature? */
        if(strcmp(data.signature, PREFERENCES_SIGNATURE) != 0) {
            logfile_message("ERROR: invalid file signature (preferences)");
            set_defaults();
            return FALSE;
        }
        else {
            /* success */
            logfile_message("Loaded user preferences - read %d field(s)", n);
            return TRUE;
        }
    }
    else {
        /* error: couldn't load settings */
        logfile_message("ERROR: couldn't open preferences file for reading. file=\"%s\"", get_preferences_fullpath());
        set_defaults();
        return FALSE;
    }
}


/*
 * save()
 * Save settings to disk
 */
void save()
{
    FILE *fp = fopen(get_preferences_fullpath(), "wb");

    if(fp != NULL) {
        /* file signature */
        str_cpy(data.signature, PREFERENCES_SIGNATURE, sizeof(data.signature));

        /* writing data */
        fwrite(&data, sizeof(data), 1, fp);
        fclose(fp);

        /* success */
    }
    else {
        /* error: couldn't save settings */
        logfile_message("ERROR: couldn't open preferences file for writing. file=\"%s\"", get_preferences_fullpath());
    }
}
