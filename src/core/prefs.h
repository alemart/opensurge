/*
 * Open Surge Engine
 * prefs.h - load/save user preferences
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

#ifndef _PREFS_H
#define _PREFS_H

#include <stdbool.h>
typedef struct prefs_t prefs_t;

/* create & destroy */
prefs_t* prefs_create(const char* prefsid); /* prefsid is a non-empty string composed by lowercase characters and/or digits */
prefs_t* prefs_destroy(prefs_t* prefs);

/* load & store */
void prefs_set_null(prefs_t* prefs, const char* key);
const char* prefs_get_string(prefs_t* prefs, const char* key);
void prefs_set_string(prefs_t* prefs, const char* key, const char* value);
int prefs_get_int(prefs_t* prefs, const char* key);
void prefs_set_int(prefs_t* prefs, const char* key, int value);
double prefs_get_double(prefs_t* prefs, const char* key);
void prefs_set_double(prefs_t* prefs, const char* key, double value);
bool prefs_get_bool(prefs_t* prefs, const char* key);
void prefs_set_bool(prefs_t* prefs, const char* key, bool value);

/* utilities */
const char* prefs_id(const prefs_t* prefs);
void prefs_save(const prefs_t* prefs); /* persist the data */
char prefs_item_type(prefs_t* prefs, const char* key); /* '\0', 's', 'i', 'f', 'b', '?' (unknown), '-' (not found) */
bool prefs_has_item(prefs_t* prefs, const char* key);
bool prefs_delete_item(prefs_t* prefs, const char* key);
void prefs_clear(prefs_t* prefs); /* clears all data */

#endif
