/*
 * Open Surge Engine
 * levparser.c - parser for level files (.lev)
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
#include <string.h>
#include <ctype.h>
#include "levparser.h"
#include "../../core/asset.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"

#define LINE_MAXLEN 1024
#define MAX_PARAMS 16
static bool parse_line(const char* filepath, int fileline, char* line, void* data, levparser_callback_t callback);

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
 * parse_line()
 * Parse a line from the .lev file
 */
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
    return callback(filepath, fileline, identifier, param_count, (const char**)param, data);
}