/*
 * Open Surge Engine
 * global.h - global definitions
 * Copyright (C) 2008-2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#define GAME_VERSION            0
#define GAME_SUB_VERSION        5
#define GAME_WIP_VERSION        0
#define GAME_WEBSITE            "http://opensurge2d.org"
#define GAME_YEAR               "2008-2018"

/* if the following is defined, this is a development build */
/*#define GAME_BUILD_VERSION      1337*/

/* Data folder (game assets) */
#ifndef GAME_DATADIR
#define GAME_DATADIR            "/usr/local/share/games/" GAME_UNIXNAME
#endif

/* Magic ;) */
#define _STRINGIFY(x)           #x
#define STRINGIFY(x)            _STRINGIFY(x)

/* Version code */
#define GAME_VERSION_CODE       ((GAME_VERSION) * 10000 + (GAME_SUB_VERSION) * 100 + (GAME_WIP_VERSION))
#ifndef GAME_BUILD_VERSION
#define GAME_VERSION_STRING     STRINGIFY(GAME_VERSION) "." STRINGIFY(GAME_SUB_VERSION) "." STRINGIFY(GAME_WIP_VERSION)
#else
#define GAME_VERSION_STRING     STRINGIFY(GAME_VERSION) "." STRINGIFY(GAME_SUB_VERSION) "." STRINGIFY(GAME_WIP_VERSION) " - " STRINGIFY(GAME_BUILD_VERSION)
#endif

/* Deprecated stuff */
#ifndef GAME_UNIX_INSTALLDIR
#define GAME_UNIX_INSTALLDIR    "/usr/share/opensurge"
#endif
#ifndef GAME_UNIX_EXECDIR
#define GAME_UNIX_EXECDIR       "/usr/bin"
#endif

/* Global definitions and constants */
#ifdef INFINITY
#undef INFINITY
#endif

#ifdef INFINITY_FLT
#undef INFINITY_FLT
#endif

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#ifdef PI
#undef PI
#endif

#ifdef EPSILON
#undef EPSILON
#endif

#define TRUE                    1
#define FALSE                   0
#define EPSILON                 1e-5
#define PI                      3.1415926535
#define INFINITY                (1<<30)

#ifdef __MSVC__
static const double __ZERO = 0.0;
#define INFINITY_FLT            (1.0/__ZERO)
#else
#define INFINITY_FLT            (1.0/0.0)
#endif

#include <stdint.h>
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#endif
