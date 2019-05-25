/*
 * Open Surge Engine
 * lang.h - language/translation module
 * Copyright (C) 2009  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _LANG_H
#define _LANG_H

#include <stdbool.h>
#include <stdlib.h>

/* public functions */
void lang_init();
void lang_release();
void lang_readstring(const char *filepath, const char *desired_key, char *str, size_t str_size);
void lang_loadfile(const char *filepath);
void lang_getstring(const char *desired_key, char *str, size_t str_size);
const char *lang_get(const char *desired_key);
bool lang_haskey(const char *desired_key);
void lang_readcompatibility(const char *filename, int *ver, int *subver, int *wipver);

/* public constants */
extern const char* DEFAULT_LANGUAGE_FILEPATH;

#endif
