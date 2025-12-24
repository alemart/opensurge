/*
 * Open Surge Engine
 * iterators.h - iterators for SurgeScript Collections
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

#ifndef _SCRIPTING_ITERATORS_H
#define _SCRIPTING_ITERATORS_H

#include <surgescript.h>
#include "../../util/iterator.h"

/* iterate over SurgeScript Arrays */
typedef iterator_t ssarrayiterator_t;

ssarrayiterator_t* iterator_create_from_surgescript_array(surgescript_object_t* array); /* iterate over a SurgeScript Array */
ssarrayiterator_t* iterator_create_from_disposable_surgescript_array(surgescript_object_t* array); /* iterate over a SurgeScript Array that will be removed when the iterator is destroyed */

/* TODO: create a C iterator that iterates over any SurgeScript Collection that implements iterable. How cool is that!!! :D */
/*iterator_t* iterator_create_from_surgescript_collection(surgescript_object_t* collection);*/

#endif