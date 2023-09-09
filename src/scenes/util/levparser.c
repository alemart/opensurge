/*
 * Open Surge Engine
 * levparser.c - parser for level files (.lev)
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

#include <allegro5/allegro.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "levparser.h"
#include "../../core/asset.h"
#include "../../util/util.h"
#include "../../util/stringutil.h"
#include "../../util/djb2.h"

/* helpers */
#define LINE_MAXLEN 1024
#define MAX_PARAMS 16
static bool parse_line(const char* filepath, int fileline, char* line, void* data, levparser_callback_t callback);
static inline levparser_command_t find_command(const char* command_name);

/* identifiers */
static const uint64_t NAME = DJB2("name");
static const uint64_t AUTHOR = DJB2("author");
static const uint64_t VERSION = DJB2("version");
static const uint64_t LICENSE = DJB2("license");
static const uint64_t REQUIRES = DJB2("requires");
static const uint64_t ACT = DJB2("act");
static const uint64_t READONLY = DJB2("readonly");
static const uint64_t THEME = DJB2("theme");
static const uint64_t BGTHEME = DJB2("bgtheme");
static const uint64_t MUSIC = DJB2("music");
static const uint64_t WATERLEVEL = DJB2("waterlevel");
static const uint64_t WATERCOLOR = DJB2("watercolor");
static const uint64_t SPAWNPOINT = DJB2("spawn_point");
static const uint64_t PLAYERS = DJB2("players");
static const uint64_t SETUP = DJB2("setup");
static const uint64_t BRICK = DJB2("brick");
static const uint64_t ENTITY = DJB2("entity");

/* legacy identifiers */
static const uint64_t STARTUP = DJB2("startup"); /* retro-compatibility */
static const uint64_t OBJECT = DJB2("object");
static const uint64_t ENEMY = DJB2("enemy");
static const uint64_t ITEM = DJB2("item");
static const uint64_t GROUPTHEME = DJB2("grouptheme");
static const uint64_t DIALOGBOX = DJB2("dialogbox");



/*
 * levelparser_parse()
 * Reads each line of a .lev file, invoking a callback for each of them
 * If the callback returns false, the reading will stop
 * Returns true on success
 */
bool levparser_parse(const char* path_to_lev_file, void* data, levparser_callback_t callback)
{
    const char* fullpath = asset_path(path_to_lev_file);
    char line[LINE_MAXLEN];
    int ln = 0;

    /* open the level file */
    ALLEGRO_FILE* fp = al_fopen(fullpath, "r");
    if(!fp)
        return false; /* error */

    /* read and parse */
    while(al_fgets(fp, line, sizeof(line))) {

        /* line has a '\n' at the end, which we keep */
        if(!parse_line(fullpath, ++ln, line, data, callback))
            break;

    }

    /* close the level file */
    al_fclose(fp);

    /* success! */
    return true;
}




/*
 *
 * private
 *
 */

/* parse a line from the .lev file */
bool parse_line(const char* filepath, int fileline, char* line, void* data, levparser_callback_t callback)
{
    char *p, *identifier, *param[MAX_PARAMS];

    /* skip spaces */
    for(p = line; *p && isspace((int)*p); p++);
    if(*p == '\0')
        return true; /* the line is an empty string */

    /* reading the identifier */
    for(identifier = p; *p && !isspace((int)*p); p++);
    if(*p)
        *(p++) = '\0';

    if((identifier[0] == '/' && identifier[1] == '/') || identifier[0] == '#')
        return true; /* the line is a comment */

    /* skip spaces */
    for(; *p && isspace((int)*p); p++);

    /* read the arguments */
    int param_count = 0;
    while(*p && param_count < MAX_PARAMS) {
        /* read an argument */
        bool quotes = (*p == '"') && (p++); /* advance p if *p is '"' */

        char* arg = p;
        while(*p && (
            (!quotes && !isspace((int)*p)) ||
            (quotes && !(*p == '"' && *(p-1) != '\\')) /* simplistic: what if \\" ? */
        )) p++;

        if(*p)
            *(p++) = '\0';

        #if 0
        /* debug */
        printf("|%s|\n", identifier);
        printf("|%s|\n", arg);
        puts("----");
        #endif

        /* store the argument */
        param[param_count++] = arg;

        /* skip spaces */
        for(; *p && isspace((int)*p); p++);
    }

    /* interpret the line */
    return callback(filepath, fileline, find_command(identifier), identifier, param_count, (const char**)param, data);
}

/* map a command string to a command enum */
levparser_command_t find_command(const char* command_name)
{
    /* quickly map the djb2 hash to a command enum */
    switch(djb2(command_name)) {
        case NAME:
            return LEVCOMMAND_NAME;

        case AUTHOR:
            return LEVCOMMAND_AUTHOR;

        case VERSION:
            return LEVCOMMAND_VERSION;

        case LICENSE:
            return LEVCOMMAND_LICENSE;

        case REQUIRES:
            return LEVCOMMAND_REQUIRES;

        case ACT:
            return LEVCOMMAND_ACT;

        case READONLY:
            return LEVCOMMAND_READONLY;

        case THEME:
            return LEVCOMMAND_THEME;

        case BGTHEME:
            return LEVCOMMAND_BGTHEME;

        case MUSIC:
            return LEVCOMMAND_MUSIC;

        case WATERLEVEL:
            return LEVCOMMAND_WATERLEVEL;

        case WATERCOLOR:
            return LEVCOMMAND_WATERCOLOR;

        case SPAWNPOINT:
            return LEVCOMMAND_SPAWNPOINT;

        case PLAYERS:
            return LEVCOMMAND_PLAYERS;

        case SETUP:
        case STARTUP:
            return LEVCOMMAND_SETUP;

        case BRICK:
            return LEVCOMMAND_BRICK;

        case ENTITY:
            return LEVCOMMAND_ENTITY;

        /* deprecated */
        case OBJECT:
        case ENEMY:
            return LEVCOMMAND_LEGACYOBJECT;

        case ITEM:
            return LEVCOMMAND_LEGACYITEM;

        case GROUPTHEME:
            return LEVCOMMAND_GROUPTHEME;

        case DIALOGBOX:
            return LEVCOMMAND_DIALOGBOX;

        /* unknown */
        default:
            return LEVCOMMAND_UNKNOWN;
    }
}