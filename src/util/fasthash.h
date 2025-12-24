/*
 * Open Surge Engine
 * fasthash.h - a fast hash table with integer keys and linear probing
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
#ifndef _FASTHASH_H
#define _FASTHASH_H

#include <stdint.h>
#include <stdbool.h>

/* Inline usage:

#define FASTHASH_INLINE
#include "fasthash.h"

No need to compile fasthash.c separately */
#if defined(FASTHASH_INLINE)
#define FASTHASH_API static inline
#else
#define FASTHASH_API
#endif

typedef struct fasthash_t fasthash_t;
FASTHASH_API fasthash_t* fasthash_create(void (*element_destructor)(void*), int lg2_cap);
FASTHASH_API fasthash_t* fasthash_destroy(fasthash_t* hashtable);
FASTHASH_API void* fasthash_get(fasthash_t* hashtable, uint64_t key);
FASTHASH_API void fasthash_put(fasthash_t* hashtable, uint64_t key, void* value);
FASTHASH_API bool fasthash_delete(fasthash_t* hashtable, uint64_t key);
FASTHASH_API void* fasthash_find(fasthash_t* hashtable, bool (*predicate)(const void*,void*), void* data);

#if defined(FASTHASH_INLINE)
#include "fasthash.c"
#endif

#endif