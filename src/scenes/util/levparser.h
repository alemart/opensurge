/*
 * Open Surge Engine
 * levparser.h - parser for level files (.lev)
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

#ifndef _LEVPARSER_H
#define _LEVPARSER_H

#include <stdbool.h>

typedef enum levparser_command_t levparser_command_t;
typedef bool (*levparser_callback_t)(const char *filepath, int fileline, levparser_command_t command, const char *command_name, int param_count, const char **param, void* data);

bool levparser_parse(const char* path_to_lev_file, void* data, levparser_callback_t callback);

enum levparser_command_t
{
    LEVCOMMAND_NAME,
    LEVCOMMAND_AUTHOR,
    LEVCOMMAND_VERSION,
    LEVCOMMAND_LICENSE,
    LEVCOMMAND_REQUIRES,
    LEVCOMMAND_ACT,
    LEVCOMMAND_READONLY,
    LEVCOMMAND_THEME,
    LEVCOMMAND_BGTHEME,
    LEVCOMMAND_MUSIC,
    LEVCOMMAND_WATERLEVEL,
    LEVCOMMAND_WATERCOLOR,
    LEVCOMMAND_SPAWNPOINT,
    LEVCOMMAND_PLAYERS,
    LEVCOMMAND_SETUP,
    LEVCOMMAND_BRICK,
    LEVCOMMAND_ENTITY,

    /* deprecated: */
    LEVCOMMAND_LEGACYOBJECT,
    LEVCOMMAND_LEGACYITEM,
    LEVCOMMAND_GROUPTHEME,
    LEVCOMMAND_DIALOGBOX,

    /* end of list */
    LEVCOMMAND_UNKNOWN = -1,
};

#endif