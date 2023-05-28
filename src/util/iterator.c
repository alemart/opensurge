/*
 * Open Surge Engine
 * iterator.c - general-purpose iterator
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

#include <string.h>
#include <stdint.h>
#include "iterator.h"
#include "util.h"


/* iterator struct */
struct iterator_t
{
    iterator_state_t* state; /* first element */
    void (*state_dtor)(iterator_state_t*);

    void* (*next)(iterator_state_t*);
    bool (*has_next)(iterator_state_t*);
};



/*
 * iterator_create()
 * Creates a new general-purpose iterator
 */
iterator_t* iterator_create(void* ctor_data, iterator_state_t* (*state_ctor)(void*), void (*state_dtor)(iterator_state_t*), void* (*next_fn)(iterator_state_t*), bool (*has_next_fn)(iterator_state_t*))
{
    iterator_t* it = mallocx(sizeof *it);

    it->state = state_ctor(ctor_data);
    it->state_dtor = state_dtor;

    it->next = next_fn;
    it->has_next = has_next_fn;

    return it;
}

/*
 * iterator_destroy()
 * Destroys an iterator
 */
iterator_t* iterator_destroy(iterator_t* it)
{
    it->state_dtor(it->state);
    free(it);

    return NULL;
}

/*
 * iterator_has_next()
 * Returns true if the iteration isn't over
 */
bool iterator_has_next(iterator_t* it)
{
    return it->has_next(it->state);
}

/*
 * iterator_next()
 * Returns a pointer to the next element of the collection and advances the iteration pointer
 * Returns NULL if there is no next element
 */
void* iterator_next(iterator_t* it)
{
    return it->next(it->state);
}

/*
 * iterator_foreach()
 * For each element of the collection, invoke a callback
 * If the callback returns false, the iteration stops
 */
bool iterator_foreach(iterator_t* it, void* data, bool (*callback)(void* element, void* data))
{
    void* element = NULL;

    while(it->has_next(it->state)) {
        element = it->next(it->state);
        if(!callback(element, data))
            return false; /* stop prematurely */
    }

    /* we have iterated over the entire collection */
    return true;
}





/*
 * ArrayIterator
 */

typedef struct arrayiterator_state_t arrayiterator_state_t;
struct arrayiterator_state_t
{
    void* array;
    size_t length;
    size_t element_size_in_bytes;
    unsigned int current_index;
};

static iterator_state_t* arrayiterator_copy_ctor(void* ctor_data);
static void arrayiterator_dtor(iterator_state_t* state);
static void* arrayiterator_next(iterator_state_t* state);
static bool arrayiterator_has_next(iterator_state_t* state);


/*
 * iterator_create_from_array()
 * Creates a new iterator suitable for iterating over a fixed-size array
 */
iterator_t* iterator_create_from_array(void* array, size_t length, size_t element_size_in_bytes)
{
    arrayiterator_state_t state = {
        .array = array,
        .length = length,
        .element_size_in_bytes = element_size_in_bytes,
        .current_index = 0
    };

    return iterator_create(&state, arrayiterator_copy_ctor, arrayiterator_dtor, arrayiterator_next, arrayiterator_has_next);
}




/* private */

#if defined(__ANDROID__)
#define WANT_ALIGNMENT_CHECK 1
#else
#define WANT_ALIGNMENT_CHECK 0
#endif

/* copy constructor */
iterator_state_t* arrayiterator_copy_ctor(void* ctor_data)
{
    const arrayiterator_state_t* s = (const arrayiterator_state_t*)ctor_data;
    const size_t size = sizeof *s;

    #if WANT_ALIGNMENT_CHECK
    #define ALIGNMENT_SIZE 4 /* in bytes */
    #define is_aligned(ptr, byte_count) (((uintptr_t)(const void *)(ptr)) % (byte_count) == 0)

    /* alignment check for ARM */
    if(!is_aligned(s->array, ALIGNMENT_SIZE))
        fatal_error("%s: unaligned pointer %p", __func__, s->array);
    /*else if(s->element_size_in_bytes % ALIGNMENT_SIZE != 0) // iterate over char, short...?
        fatal_error("%s: element size %lu may trigger unaligned accesses in %p", __func__, s->element_size_in_bytes, s->array);*/

    /* reminder: use memcpy() for unaligned accesses? */

    #undef is_aligned
    #endif /* WANT_ALIGNMENT_CHECK */

    return memcpy(mallocx(size), s, size);
}

/* destructor */
void arrayiterator_dtor(iterator_state_t* state)
{
    arrayiterator_state_t* s = (arrayiterator_state_t*)state;

    free(s);
}

/* returns the next element of the collection and advances the iteration pointer */
void* arrayiterator_next(iterator_state_t* state)
{
    arrayiterator_state_t* s = (arrayiterator_state_t*)state;

    if(s->current_index < s->length)
        return (uint8_t*)(s->array) + s->element_size_in_bytes * (s->current_index++);
    else
        return NULL;
}

/* should the iteration continue? */
bool arrayiterator_has_next(iterator_state_t* state)
{
    arrayiterator_state_t* s = (arrayiterator_state_t*)state;

    return s->current_index < s->length;
}