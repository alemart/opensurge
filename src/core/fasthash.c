/*
 * Open Surge Engine
 * fasthash.c - a fast tiny hash table with integer keys and linear probing
 * Copyright (C) 2019  Alexandre Martins <alemartf@gmail.com>
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
    uint32_t key;
    fasthash_entry_state_t state;
    void* value;
};

struct fasthash_t
{
    int length;
    int capacity;
    fasthash_entry_t* data;
    void (*destructor)(void*); /* element destructor */
};

/* static data */
static int INITIAL_CAPACITY = 4111;
static const int GROWTH_FACTOR = 2;
static const int SPARSITY = 4; /* 1 / load_factor */
static fasthash_entry_t BLANK_ENTRY = { 0, BLANK, NULL };
static inline uint32_t hash(uint32_t x);
static inline void grow(fasthash_t* hashtable);
static void empty_destructor(void* data);



/* ----- public API ----- */

/*
 * fasthash_create()
 * Create a new hash table
 * Function element_destructor deallocates individual elements
 * (it may be set to NULL if no destructor is desired)
 */
fasthash_t* fasthash_create(void (*element_destructor)(void*))
{
    fasthash_t* hashtable = mallocx(sizeof(fasthash_t));
    int i;

    hashtable->length = 0;
    hashtable->capacity = INITIAL_CAPACITY;
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
    return NULL;
}

/*
 * fasthash_get()
 * Gets an element from the hash table
 * Returns NULL if the element doesn't exist
 */
void* fasthash_get(fasthash_t* hashtable, uint32_t key)
{
    int k = hash(key) % hashtable->capacity;
    int marker = -1;

    while(hashtable->data[k].state != BLANK) {
        if(hashtable->data[k].state == ACTIVE) {
            if(hashtable->data[k].key == key) {
                /* swap marker */
                if(marker >= 0) {
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
        else if(marker < 0)
            marker = k; /* save first deleted entry */

        /* probe */
        ++(k); (k) %= hashtable->capacity;
    }

    return NULL;
}

/*
 * fasthash_put()
 * Puts an element into the hash table
 */
void fasthash_put(fasthash_t* hashtable, uint32_t key, void* value)
{
    if(hashtable->length < hashtable->capacity / SPARSITY) { /* make it sparse */
        int k = hash(key) % hashtable->capacity;

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
            ++(k); (k) %= hashtable->capacity;
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
bool fasthash_delete(fasthash_t* hashtable, uint32_t key)
{
    int k = hash(key) % hashtable->capacity;

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
        ++(k); (k) %= hashtable->capacity;
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
    int i, old_cap = hashtable->capacity;

    hashtable->capacity *= GROWTH_FACTOR;
    hashtable->data = reallocx(hashtable->data, hashtable->capacity * sizeof(fasthash_entry_t));
    for(i = old_cap; i < hashtable->capacity; i++)
        hashtable->data[i] = BLANK_ENTRY;
}

uint32_t hash(uint32_t x)
{
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    return ((x >> 16) ^ x);
}

void empty_destructor(void* data)
{
    ; /* do nothing */
}