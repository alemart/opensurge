/*
 * Open Surge Engine
 * fasthash.c - a fast hash table with integer keys and linear probing
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
#include <stdlib.h>
#include <stdio.h>
#include "fasthash.h"
#include "util.h"

/* types */
typedef enum fasthash_entry_state_t fasthash_entry_state_t;
typedef struct fasthash_entry_t fasthash_entry_t;

enum fasthash_entry_state_t {
    BLANK,
    ACTIVE,
    DELETED
};

struct fasthash_entry_t
{
    uint64_t key;
    fasthash_entry_state_t state;
    void* value;
};

struct fasthash_t
{
    size_t length;
    size_t capacity;
    uint64_t cap_mask; /* capacity - 1 */
    fasthash_entry_t* data;
    void (*destructor)(void*); /* element destructor */
};

/* static data */
static const unsigned SPARSITY = 4; /* 1 / load_factor */
static fasthash_entry_t BLANK_ENTRY = { 0, BLANK, NULL };
static inline uint64_t hash(uint64_t x, uint64_t m);
static inline void grow(fasthash_t* hashtable);
static void empty_destructor(void* data);



/* ----- public API ----- */

/*
 * fasthash_create()
 * Create a new hash table
 * Function element_destructor deallocates individual elements
 * (it may be set to NULL if no destructor is desired)
 * The initial capacity of the hash table will be set to 2^lg2_cap
 * (it grows as needed)
 */
fasthash_t* fasthash_create(void (*element_destructor)(void*), size_t lg2_cap)
{
    fasthash_t* hashtable = mallocx(sizeof(fasthash_t));
    int i;

    hashtable->length = 0;
    hashtable->capacity = 1 << min(16, lg2_cap); /* no more than 64K */
    hashtable->cap_mask = hashtable->capacity - 1;
    hashtable->destructor = element_destructor ? element_destructor : empty_destructor;
    hashtable->data = mallocx(hashtable->capacity * sizeof(fasthash_entry_t));
    for(i = 0; i < hashtable->capacity; i++)
        hashtable->data[i] = BLANK_ENTRY;

    return hashtable;
}

/*
 * fasthash_destroy()
 * Destroys an existing hash table
 */
fasthash_t* fasthash_destroy(fasthash_t* hashtable)
{
    /* destroy the remaining elements */
    for(int i = 0; i < hashtable->capacity; i++) {
        if(hashtable->data[i].state == ACTIVE)
            hashtable->destructor(hashtable->data[i].value);
    }
    
    /* release the hash table */
    free(hashtable->data);
    free(hashtable);

    /* omit warnings */
    (void)fasthash_get;
    (void)fasthash_put;
    (void)fasthash_delete;
    (void)fasthash_find;

    /* done */
    return NULL;
}

/*
 * fasthash_get()
 * Gets an element from the hash table
 * Returns NULL if the element doesn't exist
 */
void* fasthash_get(fasthash_t* hashtable, uint64_t key)
{
    uint32_t k = hash(key, hashtable->cap_mask);
    uint32_t marker = hashtable->capacity;

    while(hashtable->data[k].state != BLANK) {
        if(hashtable->data[k].state == ACTIVE) {
            if(hashtable->data[k].key == key) {
                /* swap marker */
                if(marker < hashtable->capacity) {
                    /* remove deleted entry */
                    hashtable->data[marker] = hashtable->data[k];
                    hashtable->data[k] = BLANK_ENTRY;
                    hashtable->length--;
                    return hashtable->data[marker].value;
                }

                /* return element */
                return hashtable->data[k].value;
            }
        }
        else if(marker == hashtable->capacity)
            marker = k; /* save first deleted entry */

        /* probe */
        ++k; k &= hashtable->cap_mask;
    }

    return NULL;
}

/*
 * fasthash_put()
 * Puts an element into the hash table
 */
void fasthash_put(fasthash_t* hashtable, uint64_t key, void* value)
{
    if(hashtable->length < hashtable->capacity / SPARSITY) { /* make it sparse */
        uint32_t k = hash(key, hashtable->cap_mask);

        /* won't accept NULL values */
        if(value == NULL)
            return;

        while(hashtable->data[k].state != BLANK) {
            if(hashtable->data[k].state == DELETED) {
                /* replace deleted element */
                hashtable->data[k].key = key;
                hashtable->data[k].value = value;
                hashtable->data[k].state = ACTIVE;
                return;
            }
            else if(hashtable->data[k].key == key) {
                /* replace active element */
                if(value != hashtable->data[k].value) {
                    hashtable->destructor(hashtable->data[k].value); /* TODO: save until later? */
                    hashtable->data[k].value = value;
                }
                return;
            }

            /* probe */
            ++k; k &= hashtable->cap_mask;
        }

        /* insert new element */
        hashtable->data[k].key = key;
        hashtable->data[k].value = value;
        hashtable->data[k].state = ACTIVE;
        hashtable->length++;
    }
    else {
        grow(hashtable);
        fasthash_put(hashtable, key, value);
    }
}

/*
 * fasthash_delete()
 * Deletes an element from the hash table
 * Returns true on success
 */
bool fasthash_delete(fasthash_t* hashtable, uint64_t key)
{
    uint32_t k = hash(key, hashtable->cap_mask);

    while(hashtable->data[k].state != BLANK) {
        if(hashtable->data[k].key == key) {
            if(hashtable->data[k].state == ACTIVE) {
                /* lazy removal of the entry */
                hashtable->data[k].state = DELETED;
                hashtable->destructor(hashtable->data[k].value);
                return true;
            }
            else
                return false; /* duplicate removal */
        }

        /* probe */
        ++k; k &= hashtable->cap_mask;
    }

    /* key not found */
    return false;
}

/*
 * fasthash_find()
 * Finds an element value such that test(value, data) is true
 * data is a generic pointer given as a parameter
 * If no element satisfies the given test function, NULL is returned
 */
void* fasthash_find(fasthash_t* hashtable, bool (*test)(const void*,void*), void* data)
{
    /* search the entire table */
    for(int i = 0; i < hashtable->capacity; i++) {
        if(hashtable->data[i].state == ACTIVE) {
            if(test(hashtable->data[i].value, data))
                return hashtable->data[i].value;
        }
    }

    /* no element passes the test */
    return NULL;
}


/* ----- private ----- */

void grow(fasthash_t* hashtable)
{
    size_t old_cap = hashtable->capacity;
    int i;

    hashtable->capacity *= 2;
    hashtable->cap_mask = (hashtable->cap_mask << 1) | 1;
    hashtable->data = reallocx(hashtable->data, hashtable->capacity * sizeof(fasthash_entry_t));
    for(i = old_cap; i < hashtable->capacity; i++)
        hashtable->data[i] = BLANK_ENTRY;
}

uint64_t hash(uint64_t x, uint64_t m)
{
    /* splitmix64 */
    x += UINT64_C(0x9e3779b97f4a7c15);
	x = (x ^ (x >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
	x = (x ^ (x >> 27)) * UINT64_C(0x94d049bb133111eb);
	return (x ^ (x >> 31)) & m; /* m = 2^k - 1 */
}

void empty_destructor(void* data)
{
    ; /* do nothing */
}