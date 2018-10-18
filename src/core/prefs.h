/*
 * Open Surge Engine
 * prefs.h - load/save user preferences
 * Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
void prefs_setnull(prefs_t* prefs, const char* key);
const char* prefs_getstring(prefs_t* prefs, const char* key);
void prefs_setstring(prefs_t* prefs, const char* key, const char* value);
int prefs_getint(prefs_t* prefs, const char* key);
void prefs_setint(prefs_t* prefs, const char* key, int value);
double prefs_getdouble(prefs_t* prefs, const char* key);
void prefs_setdouble(prefs_t* prefs, const char* key, double value);
bool prefs_getbool(prefs_t* prefs, const char* key);
void prefs_setbool(prefs_t* prefs, const char* key, bool value);

/* utilities */
const char* prefs_id(const prefs_t* prefs);
void prefs_save(const prefs_t* prefs); /* persist the data */
char prefs_itemtype(prefs_t* prefs, const char* key); /* '\0', 's', 'i', 'f', 'b', '?' (unknown), '-' (not found) */
int prefs_hasitem(prefs_t* prefs, const char* key);
int prefs_deleteitem(prefs_t* prefs, const char* key);
void prefs_clear(prefs_t* prefs); /* clears all data */

#endif
