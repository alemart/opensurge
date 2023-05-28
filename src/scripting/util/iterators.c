/*
 * Open Surge Engine
 * iterators.c - iterators for SurgeScript Collections
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
#include <stdbool.h>
#include "iterators.h"
#include "../../util/util.h"

/* SurgeScriptArrayIterator */
typedef struct ssarrayiterator_state_t ssarrayiterator_state_t;
struct ssarrayiterator_state_t
{
    /* SurgeScript Array */
    surgescript_object_t* array;
    bool is_disposable_array;

    /* a copy of the data */
    surgescript_var_t** c_array;
    size_t c_array_length;

    /* delegation */
    iterator_t* c_array_iterator;
};

static iterator_state_t* surgescript_arrayiterator_ctor(void* ctor_data);
static iterator_state_t* surgescript_arrayiterator_ctor_disposable(void* ctor_data);
static void surgescript_arrayiterator_dtor(iterator_state_t* state);
static void* surgescript_arrayiterator_next(iterator_state_t* state);
static bool surgescript_arrayiterator_has_next(iterator_state_t* state);

/* helpers */
static size_t get_surgescript_array_length(surgescript_object_t* array);
static void get_surgescript_array_element(surgescript_object_t* array, int index, surgescript_var_t* return_value);



/*
 * iterator_create_from_surgescript_array()
 * Create an iterator from a SurgeScript Array.
 * Each entry will be a surgescript_var_t**.
 */
ssarrayiterator_t* iterator_create_from_surgescript_array(surgescript_object_t* array)
{
    /* validate */
    if(0 != strcmp("Array", surgescript_object_name(array))) {
        fatal_error("%s: input isn't a SurgeScript Array", __func__);
        return iterator_create_from_array(NULL, 0, 0);
    }

    /* create the iterator */
    return iterator_create(
        array,
        surgescript_arrayiterator_ctor,
        surgescript_arrayiterator_dtor,
        surgescript_arrayiterator_next,
        surgescript_arrayiterator_has_next
    );
}

/*
 * iterator_create_from_disposable_surgescript_array()
 * Iterate over a SurgeScript Array that will be removed
 * as soon as the returned iterator is destroyed.
 */
ssarrayiterator_t* iterator_create_from_disposable_surgescript_array(surgescript_object_t* array)
{
    /* validate */
    if(0 != strcmp("Array", surgescript_object_name(array))) {
        fatal_error("%s: input isn't a SurgeScript Array", __func__);
        return iterator_create_from_array(NULL, 0, 0);
    }

    /* create the iterator */
    return iterator_create(
        array,
        surgescript_arrayiterator_ctor_disposable,
        surgescript_arrayiterator_dtor,
        surgescript_arrayiterator_next,
        surgescript_arrayiterator_has_next
    );
}


/* private */

/* constructor */
iterator_state_t* surgescript_arrayiterator_ctor(void* ctor_data)
{
    /*

    SurgeScript Arrays may change through time or may even be Garbage Collected.
    We'll copy its values to a temporary storage and iterate over these.
    Changing them will not affect the SurgeScript Array in any way, and vice-versa.

    Issue: okay, but what if the SurgeScript Array is large?
    Solution: iterate over any iterable SurgeScript Collection without copying any data.
              ^^^ TODO

    */
    ssarrayiterator_state_t* state = mallocx(sizeof *state);

    state->array = (surgescript_object_t*)ctor_data;
    state->is_disposable_array = false;

    state->c_array_length = get_surgescript_array_length(state->array);
    state->c_array = mallocx(state->c_array_length * sizeof(*(state->c_array)));

    for(int i = 0; i < state->c_array_length; i++) {
        state->c_array[i] = surgescript_var_create();
        get_surgescript_array_element(state->array, i, state->c_array[i]); /* read the data eagerly, now */
    }

    state->c_array_iterator = iterator_create_from_array(
        state->c_array,
        state->c_array_length,
        sizeof(*(state->c_array))
    );

    return state;
}

/* constructor of the disposable variant of the surgescript array iterator */
iterator_state_t* surgescript_arrayiterator_ctor_disposable(void* ctor_data)
{
    ssarrayiterator_state_t* state = surgescript_arrayiterator_ctor(ctor_data);
    state->is_disposable_array = true;
    return state;
}

/* destructor */
void surgescript_arrayiterator_dtor(iterator_state_t* s)
{
    ssarrayiterator_state_t* state = (ssarrayiterator_state_t*)s;

    /* release the iterator of the C array */
    iterator_destroy(state->c_array_iterator);

    /* release the C array */
    for(int i = state->c_array_length - 1; i >= 0; i--)
        surgescript_var_destroy(state->c_array[i]);
    free(state->c_array);

    /* release the SurgeScript Array if it's disposable */
    if(state->is_disposable_array)
        surgescript_object_kill(state->array);

    /* release the state */
    free(state);
}

/* next SurgeScript variable */
void* surgescript_arrayiterator_next(iterator_state_t* s)
{
    ssarrayiterator_state_t* state = (ssarrayiterator_state_t*)s;
    return iterator_next(state->c_array_iterator);
}

/* iteration not over? */
bool surgescript_arrayiterator_has_next(iterator_state_t* s)
{
    ssarrayiterator_state_t* state = (ssarrayiterator_state_t*)s;
    return iterator_has_next(state->c_array_iterator);
}

/* return the length of the array */
size_t get_surgescript_array_length(surgescript_object_t* array)
{
    surgescript_var_t* ret = surgescript_var_create();
    size_t size;

    surgescript_object_call_function(array, "get_length", NULL, 0, ret);
    size = (size_t)surgescript_var_get_number(ret);

    surgescript_var_destroy(ret);
    return size;
}

/* return (a copy of) an element of the array */
void get_surgescript_array_element(surgescript_object_t* array, int index, surgescript_var_t* return_value)
{
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };

    surgescript_var_set_number(arg, index);
    surgescript_object_call_function(array, "get", args, 1, return_value);

    surgescript_var_destroy(arg);
}