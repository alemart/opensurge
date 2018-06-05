/*
 * Open Surge Engine
 * lang.h - language/translation module
 * Copyright (C) 2009  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _LANG_H
#define _LANG_H

#define DEFAULT_LANGUAGE_FILEPATH           "languages/english.lng"

/* public functions */
void lang_init();
void lang_release();
void lang_readstring(const char *filepath, const char *desired_key, char *str, int str_size);
void lang_loadfile(const char *filepath);
void lang_getstring(const char *desired_key, char *str, int str_size);
const char *lang_get(const char *desired_key);
void lang_readcompatibility(const char *filename, int *ver, int *subver, int *wipver);

#endif
