/*
 * Open Surge Engine
 * global.h - global definitions
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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
#define GAME_VERSION_FIX        1
#define GAME_WEBSITE            "opensurge2d.org"
#define GAME_URL                "http://" GAME_WEBSITE
#define GAME_YEAR               "2008-2024"

/* Build date */
#ifndef GAME_BUILD_DATE
#define GAME_BUILD_DATE         "undefined"
#endif

/* Build version */
#ifdef GAME_BUILD_VERSION
#define _GAME_BUILD_VERSION     "-" GAME_BUILD_VERSION
#else
#define GAME_BUILD_VERSION      ""
#define _GAME_BUILD_VERSION     ""
#endif

/* "Fix" version */
#if GAME_VERSION_FIX != 0
#define _GAME_VERSION_FIX       "." STRINGIFY(GAME_VERSION_FIX)
#else
#define _GAME_VERSION_FIX       ""
#endif

/* Version code & string */
#define GAME_VERSION_STRING     STRINGIFY(GAME_VERSION_SUP) "." STRINGIFY(GAME_VERSION_SUB) "." STRINGIFY(GAME_VERSION_WIP) _GAME_VERSION_FIX _GAME_BUILD_VERSION /* must include GAME_BUILD_VERSION (to help differentiate between builds at runtime) */
#define GAME_VERSION_CODE       VERSION_CODE(GAME_VERSION_SUP, GAME_VERSION_SUB, GAME_VERSION_WIP) /* must not include GAME_VERSION_FIX (to preserve compatibility) */
#define VERSION_CODE(x,y,z)     VERSION_CODE_EX((x), (y), (z), 0)
#define VERSION_CODE_EX(x,y,z,w) ((x) * 1000000 + (y) * 10000 + (z) * 100 + (w))

/* Platform name */
#if defined(_WIN32)
#define GAME_PLATFORM_NAME      "Windows"
#elif defined(__APPLE__) && defined(__MACH__)
#define GAME_PLATFORM_NAME      "macOS"
#elif defined(__ANDROID__)
#define GAME_PLATFORM_NAME      "Android"
#elif defined(__linux__) || defined(__linux)
#define GAME_PLATFORM_NAME      "Linux"
#elif defined(__unix__) || defined(__unix)
#define GAME_PLATFORM_NAME      "Unix"
#else
#define GAME_PLATFORM_NAME      "Unknown"
#endif

/* Copyright text */
#define GAME_COPYRIGHT "" \
    "Open Surge Engine\n" \
    "Copyright (C) " GAME_YEAR " Alexandre Martins " \
    "< " GAME_URL " >"

/* Minimum version of SurgeScript */
#define SURGESCRIPT_MIN_SUP     0
#define SURGESCRIPT_MIN_SUB     6
#define SURGESCRIPT_MIN_WIP     0
#define SURGESCRIPT_MIN_VERSION STRINGIFY(SURGESCRIPT_MIN_SUP) "." STRINGIFY(SURGESCRIPT_MIN_SUB) "." STRINGIFY(SURGESCRIPT_MIN_WIP)

/* Minimum version of Allegro */
#if !defined(__ANDROID__)
#define ALLEGRO_MIN_SUP         5
#define ALLEGRO_MIN_SUB         2
#define ALLEGRO_MIN_WIP         7
#else
#define ALLEGRO_MIN_SUP         5
#define ALLEGRO_MIN_SUB         2
#define ALLEGRO_MIN_WIP         9
#endif
#define ALLEGRO_MIN_VERSION_INT ((ALLEGRO_MIN_SUP << 24) | (ALLEGRO_MIN_SUB << 16) | (ALLEGRO_MIN_WIP << 8))
#define ALLEGRO_MIN_VERSION_STR STRINGIFY(ALLEGRO_MIN_SUP) "." STRINGIFY(ALLEGRO_MIN_SUB) "." STRINGIFY(ALLEGRO_MIN_WIP)

/* Utilities */
#define STRINGIFY(x)            _STRINGIFY(x)
#define _STRINGIFY(x)           #x

/* Legacy constants */
#undef TRUE
#undef FALSE
#define TRUE                    1
#define FALSE                   0

#endif
