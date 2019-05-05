/*
 * Open Surge Engine
 * hashtable.h - generic-type hash table
 * Copyright (C) 2010, 2018, 2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "util.h"
#include "stringutil.h"
#include "logfile.h"

/* utilities */
#define TABLE_SIZE            97 /* prime number (must be a compile-time constant for compiler optimization) */
#define HASH_FUNCTION(h, key) ((h)->hash_function(key) % TABLE_SIZE)
#define CONST_PTR(KEY_TYPE)   const KEY_TYPE

/* hashtable_<typename> class: pretty much like C++ templates */
/* currently, the KEY_TYPE should be a pointer; HASH_FN should be a uint32_t function */
#define HASHTABLE_GENERATE_CODE(T) HASHTABLE_GENERATE_CODE_EX(T, char*, str_icmp, str_ihash) /* use strings as keys */
#define HASHTABLE_GENERATE_CODE_EX(T, KEY_TYPE, KEY_EQUALS_FN, HASH_FN) \
typedef struct hashtable_##T hashtable_##T; \
typedef struct hashtable_list_##T hashtable_list_##T; \
struct hashtable_##T { \
    hashtable_list_##T *data[TABLE_SIZE]; \
    void (*destroy_element)(T*); \
    int (*keycmp)(CONST_PTR(KEY_TYPE),CONST_PTR(KEY_TYPE)); \
    uint32_t (*hash_function)(CONST_PTR(KEY_TYPE)); \
}; \
struct hashtable_list_##T { \
    KEY_TYPE key; \
    T *value;\
    int reference_count;\
    hashtable_list_##T *next; \
}; \
hashtable_##T* hashtable_##T##_create(void (*destroy_element_strategy)(T*)) /* destroy_element_strategy may be NULL */ \
{ \
    int i; \
    hashtable_##T *h = mallocx(sizeof *h); \
    logfile_message("hashtable_" #T "_create()"); \
    h->destroy_element = destroy_element_strategy; \
    h->keycmp = KEY_EQUALS_FN; \
    h->hash_function = HASH_FN; \
    for(i=0; i<TABLE_SIZE; i++) \
        h->data[i] = NULL; \
    return h; \
} \
hashtable_##T* hashtable_##T##_destroy(hashtable_##T *h) \
{ \
    int i; \
    hashtable_list_##T *p, *q; \
    logfile_message("hashtable_" #T "_destroy()"); \
    for(i=0; i<TABLE_SIZE; i++) { \
        p = h->data[i]; \
        while(p != NULL) { \
            q = p->next; \
            if(h->destroy_element != NULL) \
                h->destroy_element(p->value); \
            free(p->key); \
            free(p); \
            p = q; \
        } \
    } \
    free(h); \
    return NULL; \
} \
T* hashtable_##T##_find(const hashtable_##T *h, CONST_PTR(KEY_TYPE) key) \
{ \
    int k = HASH_FUNCTION(h, key); \
    hashtable_list_##T *q = h->data[k]; \
    while(q != NULL) { \
        if(h->keycmp(q->key, key) == 0) \
            return q->value; \
        else \
            q = q->next; \
    } \
    return NULL; \
} \
void hashtable_##T##_add(hashtable_##T *h, CONST_PTR(KEY_TYPE) key, T *value) \
{ \
    if(NULL == hashtable_##T##_find(h, key)) { \
        int k = HASH_FUNCTION(h, key); \
        hashtable_list_##T *q; \
        q = mallocx(sizeof *q); \
        q->key = str_dup(key); \
        q->value = value; \
        q->reference_count = 0;\
        q->next = h->data[k]; \
        h->data[k] = q; \
    } \
} \
void hashtable_##T##_remove(hashtable_##T *h, CONST_PTR(KEY_TYPE) key) \
{ \
    int k = HASH_FUNCTION(h, key); \
    hashtable_list_##T *p, *q; \
    if(h->data[k] != NULL) { \
        p = h->data[k]; \
        if(h->keycmp(p->key, key) == 0) { \
            if(p->reference_count <= 0) { \
                h->data[k] = p->next; \
                if(h->destroy_element != NULL) \
                    h->destroy_element(p->value); \
                free(p->key); \
                free(p); \
            } \
            else \
                logfile_message("hashtable_" #T "_remove(): can't remove element with %d active references.", p->reference_count); \
            return; \
        } \
        else { \
            while(p->next != NULL) { \
                if(h->keycmp(p->next->key, key) == 0) { \
                    if(p->next->reference_count <= 0) { \
                        q = p->next; \
                        p->next = q->next; \
                        if(h->destroy_element != NULL) \
                            h->destroy_element(q->value); \
                        free(q->key); \
                        free(q); \
                    } \
                    else \
                        logfile_message("hashtable_" #T "_remove(): can't remove element with %d active references.", p->next->reference_count); \
                    return; \
                } \
                p = p->next; \
            } \
        } \
    } \
} \
int hashtable_##T##_ref(hashtable_##T *h, CONST_PTR(KEY_TYPE) key) \
{ \
    int k = HASH_FUNCTION(h, key); \
    hashtable_list_##T *q = h->data[k]; \
    while(q != NULL) { \
        if(h->keycmp(q->key, key) == 0) \
            return ++(q->reference_count); \
        else \
            q = q->next; \
    } \
    logfile_message("hashtable_" #T "_ref(): element does not exist."); \
    return 0; \
} \
int hashtable_##T##_unref(hashtable_##T *h, CONST_PTR(KEY_TYPE) key) \
{ \
    int k = HASH_FUNCTION(h, key); \
    hashtable_list_##T *q = h->data[k]; \
    while(q != NULL) { \
        if(h->keycmp(q->key, key) == 0) { \
            q->reference_count = max(0, q->reference_count - 1); \
            return q->reference_count; \
        } \
        else \
            q = q->next; \
    } \
    logfile_message("hashtable_" #T "_unref(): element does not exist."); \
    return 0; \
} \
int hashtable_##T##_refcount(const hashtable_##T *h, CONST_PTR(KEY_TYPE) key) \
{ \
    int k = HASH_FUNCTION(h, key); \
    const hashtable_list_##T *q = h->data[k]; \
    while(q != NULL) { \
        if(h->keycmp(q->key, key) == 0) \
            return q->reference_count; \
        else \
            q = q->next; \
    } \
    return 0; \
} \
void hashtable_##T##_release_unreferenced_entries(hashtable_##T *h) \
{ \
    /* TODO: improve the performance of this routine by using a queue
             to store the unreferenced entries */ \
    int i; \
    hashtable_list_##T *q; \
    for(i=0; i<TABLE_SIZE; i++) { \
        for(q=h->data[i]; q!=NULL; q=q->next) { \
            if(q->reference_count <= 0) { \
                hashtable_##T##_remove(h, q->key); \
                return; \
            } \
        } \
    } \
}

#endif
