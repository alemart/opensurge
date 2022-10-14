/*
 * Open Surge Engine
 * global.h - global definitions
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

#ifndef _GLOBAL_H
#define _GLOBAL_H

/* Game data */
#define GAME_UNIXNAME           "opensurge"
#define GAME_TITLE              "Open Surge Engine"
#define GAME_VERSION_SUP        0
#define GAME_VERSION_SUB        6
#define GAME_VERSION_WIP        1
#define GAME_VERSION_FIX        0
#define GAME_WEBSITE            "opensurge2d.org"
#define GAME_YEAR               "2008-2022"

/* If the following constant is defined, then this is a development build */
/*#define GAME_BUILD_VERSION    "1337-dev"*/

/* Minimum version of SurgeScript */
#define SURGESCRIPT_MIN_SUP     0
#define SURGESCRIPT_MIN_SUB     5
#define SURGESCRIPT_MIN_WIP     7
#define SURGESCRIPT_MIN_VERSION STRINGIFY(SURGESCRIPT_MIN_SUP) "." STRINGIFY(SURGESCRIPT_MIN_SUB) "." STRINGIFY(SURGESCRIPT_MIN_WIP)

/* Minimum version of Allegro */
#define ALLEGRO_MIN_SUP         5
#define ALLEGRO_MIN_SUB         2
#define ALLEGRO_MIN_WIP         7
#define ALLEGRO_MIN_VERSION_INT ((ALLEGRO_MIN_SUP << 24) | (ALLEGRO_MIN_SUB << 16) | (ALLEGRO_MIN_WIP << 8))
#define ALLEGRO_MIN_VERSION_STR STRINGIFY(ALLEGRO_MIN_SUP) "." STRINGIFY(ALLEGRO_MIN_SUB) "." STRINGIFY(ALLEGRO_MIN_WIP)

/* Version code & string */
#define GAME_VERSION_CODE       VERSION_CODE(GAME_VERSION_SUP, GAME_VERSION_SUB, GAME_VERSION_WIP) /* must not include GAME_VERSION_FIX (preserve compatibility) */
#ifndef GAME_BUILD_VERSION
#define GAME_VERSION_STRING     STRINGIFY(GAME_VERSION_SUP) "." STRINGIFY(GAME_VERSION_SUB) "." STRINGIFY(GAME_VERSION_WIP) "." STRINGIFY(GAME_VERSION_FIX) /* stable version */
#else
#define GAME_VERSION_STRING     STRINGIFY(GAME_VERSION_SUP) "." STRINGIFY(GAME_VERSION_SUB) "." STRINGIFY(GAME_VERSION_WIP) "." STRINGIFY(GAME_VERSION_FIX) "-" GAME_BUILD_VERSION /* development build */
#endif

/* Utilities */
#define STRINGIFY(x)            _STRINGIFY(x)
#define _STRINGIFY(x)           #x
#define VERSION_CODE(x,y,z)     ((x) * 10000 + (y) * 100 + (z))

/* Legacy constants */
#undef TRUE
#undef FALSE
#define TRUE                    1
#define FALSE                   0

#endif