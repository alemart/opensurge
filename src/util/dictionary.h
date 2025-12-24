/*
 * Open Surge Engine
 * dictionary.h - a simple dictionary with string keys
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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

#ifndef _DICTIONARY_H
#define _DICTIONARY_H

#include <stdbool.h>

/* opaque type */
typedef struct dictionary_t dictionary_t;

/* forward declarations */
struct iterator_t;

/* API */
dictionary_t* dictionary_create(bool want_case_sensitive_keys, void (*element_dtor)(void*,void*), void* dtor_context);
dictionary_t* dictionary_destroy(dictionary_t* dict);
void* dictionary_get(const dictionary_t* dict, const char* key);
void dictionary_put(dictionary_t* dict, const char* key, void* element);
struct iterator_t* dictionary_keys(const dictionary_t* dict);
struct iterator_t* dictionary_values(const dictionary_t* dict);

#endif