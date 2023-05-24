/*
 * Open Surge Engine
 * darray.h - Dynamic (expandable) Arrays
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

#ifndef _DARRAY_H
#define _DARRAY_H

#include <stdlib.h>
#include "util.h"

/*
 * DARRAY()
 * declares a dynamic array of a given type
 */
#define DARRAY(type, arr)                    type* arr; size_t arr##_len, arr##_cap;
#define STATIC_DARRAY(type, arr)             static type* arr; static size_t arr##_len, arr##_cap;

/*
 * darray_init()
 * initializes the array
 */
#define darray_init(arr)                     darray_init_ex(arr, 0)

/*
 * darray_release()
 * releases the array
 */
#define darray_release(arr)                  \
    do { arr##_len = arr##_cap = 0; free(arr); arr = NULL; } while(0)

/*
 * darray_init_ex()
 * initializes the array with a given capacity
 */
#define darray_init_ex(arr, cap)             \
    (arr##_len = 0, arr##_cap = ((cap) > 0 ? (cap) : 4), arr = mallocx(arr##_cap * sizeof(*(arr))))

/*
 * darray_push()
 * pushes element 'x' into the array, returning the new length of the array
 */
#define darray_push(arr, x)                  \
    (*(((arr##_len >= arr##_cap) ? (arr = reallocx(arr, (arr##_cap *= 2) * sizeof(*(arr)))) : arr) + (arr##_len)) = (x), ++arr##_len)

/*
 * darray_pop()
 * pops the last element from the array, writing its contents to variable dst
 */
#define darray_pop(arr, dst)                 \
    do { if(arr##_len > 0) dst = arr[--arr##_len]; } while(0)
    
/*
 * darray_remove()
 * removes the index-th element from the array
 */
 #define darray_remove(arr, index)           \
    do { if((index) < arr##_len && (index) >= 0) { memmove((arr) + (index), (arr) + ((index) + 1), (arr##_len - ((index) + 1)) * sizeof(*(arr))); arr##_len--; } } while(0)

/*
 * darray_length()
 * returns the length of the array
 */
#define darray_length(arr)                   (+arr##_len)

/*
 * darray_clear()
 * sets the length of the array to zero, without freeing any of its contents
 */
#define darray_clear(arr)                    (arr##_len = 0)

/*
 * darray_iterator()
 * returns a new iterator linked to the array; do not modify the array while iterating
 */
#define darray_iterator(arr)                (iterator_create_from_array((arr), darray_length(arr), sizeof(*(arr))))

#endif
