/*
 * Open Surge Engine
 * global.h - global definitions
 * Copyright (C) 2008-2021  Alexandre Martins <alemartf@gmail.com>
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
#define GAME_MAIN_VERSION       0
#define GAME_MAJOR_VERSION      5
#define GAME_MINOR_VERSION      2
#define GAME_PATCH_VERSION      0
#define GAME_WEBSITE            "opensurge2d.org"
#define GAME_YEAR               "2008-2021"

/* Scripting */
#define SURGESCRIPT_MIN_VERSION "0.5.4.3"

/* if the following is defined, this is a development build */
/*#define GAME_BUILD_VERSION      1337-dev*/

/* Data folder (game assets) */
#if !defined(GAME_DATADIR)
#define GAME_DATADIR            "/usr/share/games/opensurge"
#endif

/* Utilities */
#define VERSION_CODE(X,Y,Z)     ((X) * 10000 + (Y) * 100 + (Z))
#define VERSION_STRING(X,Y,Z)   STRINGIFY(X) "." STRINGIFY(Y) "." STRINGIFY(Z)
#define STRINGIFY(x)            _STRINGIFY(x)
#define _STRINGIFY(x)           #x

/* Version code */
#define GAME_VERSION_CODE       VERSION_CODE(GAME_MAIN_VERSION, GAME_MAJOR_VERSION, GAME_MINOR_VERSION) /* must not include GAME_PATCH_VERSION (preserve compatibility) */

/* Version string */
#if !defined(GAME_BUILD_VERSION) && GAME_PATCH_VERSION == 0 /* stable version */
#define GAME_VERSION_STRING     VERSION_STRING(GAME_MAIN_VERSION, GAME_MAJOR_VERSION, GAME_MINOR_VERSION)
#elif !defined(GAME_BUILD_VERSION) && GAME_PATCH_VERSION != 0 /* stable version with patch */
#define GAME_VERSION_STRING     VERSION_STRING(GAME_MAIN_VERSION, GAME_MAJOR_VERSION, GAME_MINOR_VERSION) "." STRINGIFY(GAME_PATCH_VERSION)
#elif defined(GAME_BUILD_VERSION) && GAME_PATCH_VERSION == 0 /* development version */
#define GAME_VERSION_STRING     VERSION_STRING(GAME_MAIN_VERSION, GAME_MAJOR_VERSION, GAME_MINOR_VERSION) "-" STRINGIFY(GAME_BUILD_VERSION)
#else /* development version */
#define GAME_VERSION_STRING     VERSION_STRING(GAME_MAIN_VERSION, GAME_MAJOR_VERSION, GAME_MINOR_VERSION) "." STRINGIFY(GAME_PATCH_VERSION) "-" STRINGIFY(GAME_BUILD_VERSION)
#endif

/* Legacy constants */
#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define TRUE                    1
#define FALSE                   0

#endif
