/*
 * Open Surge Engine
 * dictionary.c - a simple dictionary with string keys
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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
#include "dictionary.h"
#include "stringutil.h"
#include "util.h"
#include "darray.h"
#include "iterator.h"

/* dictionary entry */
typedef struct dictentry_t dictentry_t;
struct dictentry_t
{
    char* key;
    void* value;
};

/* dictionary */
struct dictionary_t
{
    int (*keycmp)(const char*,const char*);
    void (*dtor)(void*,void*);
    void *dtor_context;

    /* entries sorted by key */
    DARRAY(dictentry_t, entry);
};

/* helpers */
static int binary_search(const dictionary_t* dict, const char* key);
static void release_entry(dictionary_t* dict, int entry_index);
static void null_dtor(void* element, void* dtor_context);

/* DictionaryIterator */
typedef struct dictiterator_state_t dictiterator_state_t;
struct dictiterator_state_t
{
    const dictionary_t* dict;
    int current_index;
};

static iterator_state_t* dictiterator_copy_ctor(void* ctor_data);
static void dictiterator_dtor(iterator_state_t* state);
static void* dictiterator_next_key(iterator_state_t* state);
static void* dictiterator_next_value(iterator_state_t* state);
static bool dictiterator_has_next(iterator_state_t* state);



/*
 *
 * public
 *
 */


/*
 * dictionary_create()
 * Creates a dictionary with an optional element destructor
 */
dictionary_t* dictionary_create(bool want_case_sensitive_keys, void (*element_dtor)(void*,void*), void* dtor_context)
{
    dictionary_t* dict = mallocx(sizeof *dict);

    dict->keycmp = want_case_sensitive_keys ? strcmp : str_icmp;
    dict->dtor = element_dtor != NULL ? element_dtor : null_dtor;
    dict->dtor_context = dtor_context;

    darray_init(dict->entry);

    return dict;
}


/*
 * dictionary_destroy()
 * Destroys a dictionary
 */
dictionary_t* dictionary_destroy(dictionary_t* dict)
{
    for(int i = 0; i < darray_length(dict->entry); i++)
        release_entry(dict, i);

    darray_release(dict->entry);
    free(dict);
    return NULL;
}


/*
 * dictionary_get()
 * Gets an element from a dictionary. Returns NULL if not found
 */
void* dictionary_get(const dictionary_t* dict, const char* key)
{
    int index_of_key = binary_search(dict, key);

    if(index_of_key >= 0)
        return dict->entry[index_of_key].value;

    return NULL;
}


/*
 * dictionary_put()
 * Puts an element into a dictionary. If the provided key is already in the
 * dictionary, the previously stored element will be removed.
 */
void dictionary_put(dictionary_t* dict, const char* key, void* element)
{
    assertx(element != NULL);

    /* insert new element */
    dictentry_t new_entry = { str_dup(key), element };
    darray_push(dict->entry, new_entry);

    /* insertion sort */
    int last = darray_length(dict->entry) - 1;
    for(int i = last; i > 0; i--) {
        int cmp = dict->keycmp(dict->entry[i].key, dict->entry[i-1].key);
        if(cmp < 0) {
            /* swap entries i and i-1 */
            dictentry_t tmp = dict->entry[i];
            dict->entry[i] = dict->entry[i-1];
            dict->entry[i-1] = tmp;
        }
        else if(cmp == 0) {
            /* duplicate key; erase old entry */
            release_entry(dict, i-1);
            darray_remove(dict->entry, i-1);
            break;
        }
        else {
            /* done */
            break;
        }
    }

    /* this simple implementation works, but it won't be very efficient for
       a dictionary with hundreds or thousands of elements UNLESS it is
       fed in sorted order. */
}

/*
 * dictionary_keys()
 * Returns an iterator to the keys of the dictionary
 */
iterator_t* dictionary_keys(const dictionary_t* dict)
{
    dictiterator_state_t state = {
        .dict = dict,
        .current_index = 0
    };

    return iterator_create(&state, dictiterator_copy_ctor, dictiterator_dtor, dictiterator_next_key, dictiterator_has_next);
}

/*
 * dictionary_values()
 * Returns an iterator to the values of the dictionary
 */
iterator_t* dictionary_values(const dictionary_t* dict)
{
    dictiterator_state_t state = {
        .dict = dict,
        .current_index = 0
    };

    return iterator_create(&state, dictiterator_copy_ctor, dictiterator_dtor, dictiterator_next_value, dictiterator_has_next);
}




/*
 *
 * private
 *
 */

/* finds j >= 0 such that dict->entry[k].key == key;
   returns -1 if not found */
int binary_search(const dictionary_t* dict, const char* key)
{
    int l = 0, r = darray_length(dict->entry) - 1;

    while(l <= r) {
        int m = (l + r) / 2;
        int cmp = dict->keycmp(key, dict->entry[m].key);

        if(cmp < 0)
            r = m - 1;
        else if(cmp > 0)
            l = m + 1;
        else
            return m;
    }

    return -1;
}

/* releases dict->entry[entry_index] */
void release_entry(dictionary_t* dict, int entry_index)
{
    free(dict->entry[entry_index].key);
    dict->dtor(dict->entry[entry_index].value, dict->dtor_context);
    dict->entry[entry_index] = (dictentry_t){ NULL, NULL };
}

/* an element destructor that does nothing */
void null_dtor(void* element, void* dtor_context)
{
    /* do nothing */
}

/* DictionaryIterator: copy constructor */
iterator_state_t* dictiterator_copy_ctor(void* ctor_data)
{
    const dictiterator_state_t* s = (const dictiterator_state_t*)ctor_data;
    const size_t size = sizeof *s;

    return memcpy(mallocx(size), s, size);
}

/* DictionaryIterator: destructor */
void dictiterator_dtor(iterator_state_t* state)
{
    dictiterator_state_t* s = (dictiterator_state_t*)state;
    free(s);
}

/* DictionaryIterator: return the next element and advance the iteration pointer */
void* dictiterator_next_key(iterator_state_t* state)
{
    dictiterator_state_t* s = (dictiterator_state_t*)state;
    const dictionary_t* dict = s->dict;

    if(s->current_index < darray_length(dict->entry)) {
        const dictentry_t* entry = &dict->entry[s->current_index++];
        return entry->key;
    }

    return NULL;
}

/* DictionaryIterator: return the next element and advance the iteration pointer */
void* dictiterator_next_value(iterator_state_t* state)
{
    dictiterator_state_t* s = (dictiterator_state_t*)state;
    const dictionary_t* dict = s->dict;

    if(s->current_index < darray_length(dict->entry)) {
        const dictentry_t* entry = &dict->entry[s->current_index++];
        return entry->value;
    }

    return NULL;
}

/* DictionaryIterator: can we still advance the iteration pointer? */
bool dictiterator_has_next(iterator_state_t* state)
{
    const dictiterator_state_t* s = (dictiterator_state_t*)state;
    const dictionary_t* dict = s->dict;

    return s->current_index < darray_length(dict->entry);
}