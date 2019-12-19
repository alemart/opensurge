/*
 * Open Surge Engine
 * lang.h - language/translation module
 * Copyright (C) 2009, 2019  Alexandre Martins <alemartf@gmail.com>
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
void lang_loadfile(const char* filepath);
char* lang_getstring(const char* desired_key, char* dest, size_t dest_size);
const char* lang_get(const char* desired_key);
const char* lang_getid();
bool lang_haskey(const char* desired_key);
void lang_compatibility(const char* filename, int* supver, int* subver, int* wipver);
void lang_metadata(const char* filepath, const char* desired_key, char* dest, size_t dest_size);

/* public constants */
extern const char* DEFAULT_LANGUAGE_FILEPATH;

#endif
