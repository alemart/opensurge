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
#include <string.h>
#include <ctype.h>
#include "util.h"
#include "stringutil.h"
#include "logfile.h"

/* utilities */
#define __H_TABLE_SIZE                97 /* prime number (must be a compile-time constant for compiler optimization) */
#define __H_HASH_FUNCTION(h, key)     ((h)->hash_function(key) % __H_TABLE_SIZE)
#define __H_CONST(KEY_TYPE)           const KEY_TYPE
#define HASHTABLE(T, var_name)        hashtable_##T* var_name = NULL; /* declares a hash table */
#define HASHTABLE_GENERATE_CODE(T, DESTRUCTOR_FN) /* use case-insensitive strings as keys */ \
    HASHTABLE_GENERATE_CODE_EX(T, DESTRUCTOR_FN, char*, __h_hash_key_##T, __h_compare_key_##T, __h_clone_key_##T, __h_delete_key_##T)

/* hashtable_<typename> class: pretty much like C++ templates */
/* DESTRUCTOR_FN is a void function that takes a T* as an argument (i.e., the object destructor) */
/* currently, the KEY_TYPE should be a pointer; HASH_FN should be a uint32_t function */
/* KEY_COMPARE_FN should return 0 if two keys are the same */
/* if KEY_COMPARE is set to NULL, then the key pointers will simply be compared for equality */
/* KEY_CLONE_FN, KEY_DELETE_FN may be NULL */
#define HASHTABLE_GENERATE_CODE_EX(T, DESTRUCTOR_FN, KEY_TYPE, KEY_HASH_FN, KEY_COMPARE_FN, KEY_CLONE_FN, KEY_DELETE_FN) \
static uint32_t __h_hash_key_##T(const char *key); \
static int __h_compare_key_##T(const char *key1, const char *key2); \
static char* __h_clone_key_##T(const char *key); \
static void __h_delete_key_##T(char *key); \
static void __h_unused_##T(); \
typedef struct hashtable_##T hashtable_##T; \
typedef struct hashtable_list_##T hashtable_list_##T; \
struct hashtable_##T { \
    hashtable_list_##T *data[__H_TABLE_SIZE]; \
    void (*destructor)(T*); \
    uint32_t (*hash_function)(__H_CONST(KEY_TYPE)); \
    int (*key_compare)(__H_CONST(KEY_TYPE),__H_CONST(KEY_TYPE)); \
    KEY_TYPE (*key_clone)(__H_CONST(KEY_TYPE)); \
    void (*key_delete)(KEY_TYPE); \
}; \
struct hashtable_list_##T { \
    KEY_TYPE key; \
    T *value;\
    int reference_count;\
    hashtable_list_##T *next; \
}; \
static hashtable_##T* hashtable_##T##_create() \
{ \
    int i; \
    hashtable_##T *h = mallocx(sizeof *h); \
    logfile_message("hashtable_" #T "_create()"); \
    h->destructor = DESTRUCTOR_FN; \
    h->hash_function = KEY_HASH_FN; \
    h->key_compare = KEY_COMPARE_FN; \
    h->key_clone = KEY_CLONE_FN; \
    h->key_delete = KEY_DELETE_FN; \
    for(i=0; i<__H_TABLE_SIZE; i++) \
        h->data[i] = NULL; \
    return h; \
} \
static hashtable_##T* hashtable_##T##_destroy(hashtable_##T *h) \
{ \
    int i; \
    hashtable_list_##T *p, *q; \
    logfile_message("hashtable_" #T "_destroy()"); \
    for(i=0; i<__H_TABLE_SIZE; i++) { \
        p = h->data[i]; \
        while(p != NULL) { \
            q = p->next; \
            if(h->destructor != NULL) \
                h->destructor(p->value); \
            if(h->key_delete != NULL) \
                h->key_delete(p->key); \
            free(p); \
            p = q; \
        } \
    } \
    free(h); \
    __h_unused_##T(); \
    return NULL; \
} \
static T* hashtable_##T##_find(const hashtable_##T *h, __H_CONST(KEY_TYPE) key) \
{ \
    int k = __H_HASH_FUNCTION(h, key); \
    hashtable_list_##T *q = h->data[k]; \
    while(q != NULL) { \
        if(h->key_compare(q->key, key) == 0) \
            return q->value; \
        else \
            q = q->next; \
    } \
    return NULL; \
} \
static void hashtable_##T##_add(hashtable_##T *h, __H_CONST(KEY_TYPE) key, T *value) \
{ \
    if(NULL == hashtable_##T##_find(h, key)) { \
        int k = __H_HASH_FUNCTION(h, key); \
        hashtable_list_##T *q; \
        q = mallocx(sizeof *q); \
        q->key = (h->key_clone != NULL) ? h->key_clone(key) : (KEY_TYPE)key; \
        q->value = value; \
        q->reference_count = 0;\
        q->next = h->data[k]; \
        h->data[k] = q; \
    } \
} \
static void hashtable_##T##_remove(hashtable_##T *h, __H_CONST(KEY_TYPE) key) \
{ \
    int k = __H_HASH_FUNCTION(h, key); \
    hashtable_list_##T *p, *q; \
    if(h->data[k] != NULL) { \
        p = h->data[k]; \
        if(h->key_compare(p->key, key) == 0) { \
            if(p->reference_count <= 0) { \
                h->data[k] = p->next; \
                if(h->destructor != NULL) \
                    h->destructor(p->value); \
                if(h->key_delete != NULL) \
                    h->key_delete(p->key); \
                free(p); \
            } \
            else \
                logfile_message("hashtable_" #T "_remove(): can't remove element with %d active references.", p->reference_count); \
            return; \
        } \
        else { \
            while(p->next != NULL) { \
                if(h->key_compare(p->next->key, key) == 0) { \
                    if(p->next->reference_count <= 0) { \
                        q = p->next; \
                        p->next = q->next; \
                        if(h->destructor != NULL) \
                            h->destructor(q->value); \
                        if(h->key_delete != NULL) \
                            h->key_delete(q->key); \
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
static int hashtable_##T##_ref(hashtable_##T *h, __H_CONST(KEY_TYPE) key) \
{ \
    int k = __H_HASH_FUNCTION(h, key); \
    hashtable_list_##T *q = h->data[k]; \
    while(q != NULL) { \
        if(h->key_compare(q->key, key) == 0) \
            return ++(q->reference_count); \
        else \
            q = q->next; \
    } \
    logfile_message("hashtable_" #T "_ref(): element does not exist."); \
    return 0; \
} \
static int hashtable_##T##_unref(hashtable_##T *h, __H_CONST(KEY_TYPE) key) \
{ \
    int k = __H_HASH_FUNCTION(h, key); \
    hashtable_list_##T *q = h->data[k]; \
    while(q != NULL) { \
        if(h->key_compare(q->key, key) == 0) { \
            q->reference_count = max(0, q->reference_count - 1); \
            return q->reference_count; \
        } \
        else \
            q = q->next; \
    } \
    logfile_message("hashtable_" #T "_unref(): element does not exist."); \
    return 0; \
} \
static int hashtable_##T##_refcount(const hashtable_##T *h, __H_CONST(KEY_TYPE) key) \
{ \
    int k = __H_HASH_FUNCTION(h, key); \
    const hashtable_list_##T *q = h->data[k]; \
    while(q != NULL) { \
        if(h->key_compare(q->key, key) == 0) \
            return q->reference_count; \
        else \
            q = q->next; \
    } \
    return 0; \
} \
static void hashtable_##T##_release_unreferenced_entries(hashtable_##T *h) \
{ \
    /* TODO: improve the performance of this routine by using a queue
             to store the unreferenced entries */ \
    int i; \
    hashtable_list_##T *q; \
    for(i=0; i<__H_TABLE_SIZE; i++) { \
        for(q=h->data[i]; q!=NULL; q=q->next) { \
            if(q->reference_count <= 0) { \
                hashtable_##T##_remove(h, q->key); \
                return; /* fast hack */ \
            } \
        } \
    } \
} \
static uint32_t __h_hash_key_##T(const char *key) \
{ \
    uint32_t hash = 0; \
    while(*key) /* case-insensitive hash */ \
        hash = tolower(*(key++)) + (hash << 6) + (hash << 16) - hash; \
    return hash; \
} \
static int __h_compare_key_##T(const char *key1, const char *key2) \
{ \
    /* case-insensitive comparison */ \
    return str_icmp(key1, key2); \
} \
static char* __h_clone_key_##T(const char *key) \
{ \
    return strcpy(mallocx((1 + strlen(key)) * sizeof(char)), key); \
} \
static void __h_delete_key_##T(char *key) \
{ \
    free(key); \
} \
static void __h_unused_##T() \
{ \
    /* omit compiler warnings */ \
    (void)hashtable_##T##_create; \
    (void)hashtable_##T##_destroy; \
    (void)hashtable_##T##_find; \
    (void)hashtable_##T##_add; \
    (void)hashtable_##T##_remove; \
    (void)hashtable_##T##_ref; \
    (void)hashtable_##T##_refcount; \
    (void)hashtable_##T##_unref; \
    (void)hashtable_##T##_release_unreferenced_entries; \
    (void)__h_hash_key_##T; \
    (void)__h_compare_key_##T; \
    (void)__h_clone_key_##T; \
    (void)__h_delete_key_##T; \
}

#endif
