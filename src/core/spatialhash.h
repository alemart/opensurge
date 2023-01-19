/*
 * Open Surge Engine
 * spatialhash.h - generic-type spatial hash table (bidimensional)
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

#ifndef _SPATIALHASH_H
#define _SPATIALHASH_H

#include <stdbool.h>
#include "util.h"
#include "logfile.h"

/* utilities */
#define SPATIALHASH_GRID_WIDTH      64
#define SPATIALHASH_GRID_HEIGHT     32
#define DEFAULT_WORLD_WIDTH         50048 /* max. estimates; multiples of the grid size */
#define DEFAULT_WORLD_HEIGHT        15008

/* spatialhash_<typename> class: pretty much like C++ templates */
#define SPATIALHASH_GENERATE_CODE(T) \
typedef struct spatialhash_##T spatialhash_##T; \
typedef struct spatialhash_list_##T spatialhash_list_##T; \
struct spatialhash_##T { \
    spatialhash_list_##T *bucket[SPATIALHASH_GRID_HEIGHT][SPATIALHASH_GRID_WIDTH]; /* regular elements */ \
    spatialhash_list_##T *persistent_elements; /* persistent elements */  \
    int cell_width, cell_height; /* a cell is an element of a SPATIALHASH_GRID_WIDTH x SPATIALHASH_GRID_HEIGHT grid, also known as the bucket */ \
    int largest_element_width, largest_element_height; \
    int (*xpos)(const T*); \
    int (*ypos)(const T*); \
    int (*width)(const T*); \
    int (*height)(const T*); \
    T* (*destroy_element)(T*); \
}; \
struct spatialhash_list_##T { \
    T *data; \
    spatialhash_list_##T *next; \
}; \
spatialhash_##T* spatialhash_##T##_create_ex(T* (*destroy_element_strategy)(T*), int (*get_element_xpos)(const T*), int (*get_element_ypos)(const T*), int (*get_element_width)(const T*), int (*get_element_height)(const T*), int estimated_world_width, int estimated_world_height) /* destroy_element_strategy may be NULL */ \
{ \
    int i, j; \
    spatialhash_##T *sh = mallocx(sizeof *sh); \
    logfile_message("spatialhash_" #T "_create_ex(%d, %d)", estimated_world_width, estimated_world_height); \
    sh->cell_width = max(1, estimated_world_width / SPATIALHASH_GRID_WIDTH); \
    sh->cell_height = max(1, estimated_world_height / SPATIALHASH_GRID_HEIGHT); \
    sh->largest_element_width = 0; \
    sh->largest_element_height = 0; \
    sh->xpos = get_element_xpos; \
    sh->ypos = get_element_ypos; \
    sh->width = get_element_width; \
    sh->height = get_element_height; \
    sh->destroy_element = destroy_element_strategy; \
    for(i=0; i<SPATIALHASH_GRID_HEIGHT; i++) { \
        for(j=0; j<SPATIALHASH_GRID_WIDTH; j++) \
            sh->bucket[i][j] = NULL; \
    } \
    sh->persistent_elements = NULL; \
    return sh; \
} \
/* creates a new spatial hash */ \
spatialhash_##T* spatialhash_##T##_create(T* (*destroy_element_strategy)(T*), int (*get_element_xpos)(const T*), int (*get_element_ypos)(const T*), int (*get_element_width)(const T*), int (*get_element_height)(const T*)) /* destroy_element_strategy may be NULL */ \
{ \
    return spatialhash_##T##_create_ex(destroy_element_strategy, get_element_xpos, get_element_ypos, get_element_width, get_element_height, DEFAULT_WORLD_WIDTH, DEFAULT_WORLD_HEIGHT); \
} \
/* destroys an existing spatial hash */ \
spatialhash_##T* spatialhash_##T##_destroy(spatialhash_##T *sh) \
{ \
    int i, j; \
    spatialhash_list_##T *p, *q; \
    logfile_message("spatialhash_" #T "_destroy()"); \
    for(i=0; i<SPATIALHASH_GRID_HEIGHT; i++) { \
        for(j=0; j<SPATIALHASH_GRID_WIDTH; j++) { \
            p = sh->bucket[i][j]; \
            while(p != NULL) { \
                q = p->next; \
                if(sh->destroy_element != NULL) \
                    p->data = sh->destroy_element(p->data); \
                free(p); \
                p = q; \
            } \
        } \
    } \
    p = sh->persistent_elements; \
    while(p != NULL) { \
        q = p->next; \
        if(sh->destroy_element != NULL) \
            p->data = sh->destroy_element(p->data); \
        free(p); \
        p = q; \
    } \
    free(sh); \
    logfile_message("spatialhash_" #T "_destroy() - success!"); \
    return NULL; \
} \
/* adds an element to the spatial hash */ \
void spatialhash_##T##_add(spatialhash_##T *sh, T *element) \
{ \
    int row, col; \
    spatialhash_list_##T *p; \
    \
    col = sh->xpos(element) / sh->cell_width; \
    row = sh->ypos(element) / sh->cell_height; \
    col = clip(col, 0, SPATIALHASH_GRID_WIDTH-1); \
    row = clip(row, 0, SPATIALHASH_GRID_HEIGHT-1); \
    \
    for(p = sh->bucket[row][col]; p != NULL; p = p->next) { \
        if(p->data == element) { \
            logfile_message("spatialhash_" #T "_add(): element '%p' already exists! It won't be added.", element); \
            return; \
        } \
    } \
    \
    /*logfile_message("spatialhash_" #T "_add(): adding '%p'...", element);*/ \
    \
    p = mallocx(sizeof *p); \
    p->data = element; \
    p->next = sh->bucket[row][col]; \
    sh->bucket[row][col] = p; \
    \
    sh->largest_element_width = max(sh->largest_element_width, sh->width(element)); \
    sh->largest_element_height = max(sh->largest_element_height, sh->height(element)); \
} \
/* adds a persistent element to the spatial hash */ \
void spatialhash_##T##_add_persistent(spatialhash_##T *sh, T *element) \
{ \
    spatialhash_list_##T *p; \
    \
    for(p = sh->persistent_elements; p != NULL; p = p->next) { \
        if(p->data == element) { \
            logfile_message("spatialhash_" #T "_add_persistent(): element '%p' already exists! It won't be added.", element); \
            return; \
        } \
    } \
    \
    /*logfile_message("spatialhash_" #T "_add_persistent(): adding '%p'...", element);*/ \
    \
    p = mallocx(sizeof *p); \
    p->data = element; \
    p->next = sh->persistent_elements; \
    sh->persistent_elements = p; \
} \
/* checks if an element of the spatial hash is persistent */ \
bool spatialhash_##T##_is_persistent(spatialhash_##T *sh, T *element) \
{ \
    spatialhash_list_##T *p; \
    \
    for(p = sh->persistent_elements; p != NULL; p = p->next) { \
        if(p->data == element) \
            return true; \
    } \
    \
    return false; \
} \
/* removes an element from the spatial hash */ \
void spatialhash_##T##_remove(spatialhash_##T *sh, T *element) \
{ \
    int row, col; \
    spatialhash_list_##T *p, *prev; \
    \
    col = sh->xpos(element) / sh->cell_width; \
    row = sh->ypos(element) / sh->cell_height; \
    col = clip(col, 0, SPATIALHASH_GRID_WIDTH-1); \
    row = clip(row, 0, SPATIALHASH_GRID_HEIGHT-1); \
    \
    /* is it a regular element? */ \
    for(prev = NULL, p = sh->bucket[row][col]; p != NULL; prev = p, p = p->next) { \
        if(p->data == element) { \
            if(prev != NULL) \
                prev->next = p->next; \
            else \
                sh->bucket[row][col] = p->next; \
            \
            if(sh->destroy_element != NULL) \
                p->data = sh->destroy_element(p->data); \
            \
            free(p); \
            return; \
        } \
    } \
    \
    /* is it a persistent element? */ \
    for(prev = NULL, p = sh->persistent_elements; p != NULL; prev = p, p = p->next) { \
        if(p->data == element) { \
            if(prev != NULL) \
                prev->next = p->next; \
            else \
                sh->persistent_elements = p->next; \
            \
            if(sh->destroy_element != NULL) \
                p->data = sh->destroy_element(p->data); \
            \
            free(p); \
            return; \
        } \
    } \
    \
    /* FIXME looking in the entire table to see if we find the element (this is BAD) */ \
    for(row = 0; row < SPATIALHASH_GRID_HEIGHT; row++) { \
        for(col = 0; col < SPATIALHASH_GRID_WIDTH; col++) { \
            for(prev = NULL, p = sh->bucket[row][col]; p != NULL; prev = p, p = p->next) { \
                if(p->data == element) { \
                    logfile_message("spatialhash_" #T "_remove(): trouble on removing '%p'... I had to look for it in the entire table", element); \
                    \
                    if(prev != NULL) \
                        prev->next = p->next; \
                    else \
                        sh->bucket[row][col] = p->next; \
                    \
                    if(sh->destroy_element != NULL) \
                        p->data = sh->destroy_element(p->data); \
                    \
                    free(p); \
                    return; \
                } \
            } \
        } \
    } \
    \
    /* aargh! it's 3:00 AM and we found nothing! */ \
    logfile_message("spatialhash_" #T "_remove(): element '%p' was not found.", element); \
} \
/* for each element X in the given rectangle, calls callback_function(X,some_user_data), */ \
/* where some_user_data, a void pointer, may be anything you need. */ \
/* callback_function must return zero to let the enumeration proceed, or any non-zero value */ \
/* to stop it. */ \
/* ATTENTION! persistent elements ("always_active") are considered even if they're not */ \
/* inside the given rectangle */ \
void spatialhash_##T##_foreach(spatialhash_##T *sh, int rectangle_xpos, int rectangle_ypos, int rectangle_width, int rectangle_height, void *some_user_data, int (*callback_function)(T*,void*)) \
{ \
    int r_x1, r_y1, r_x2, r_y2, e_x1, e_y1, e_x2, e_y2; \
    int row, col, first_row, first_col, last_row, last_col; \
    int stop_iteration = FALSE; \
    spatialhash_list_##T *p, *prev; \
    \
    r_x1 = rectangle_xpos - sh->largest_element_width; \
    r_y1 = rectangle_ypos - sh->largest_element_height; \
    r_x2 = rectangle_xpos + sh->largest_element_width + rectangle_width; \
    r_y2 = rectangle_ypos + sh->largest_element_height + rectangle_height; \
    \
    first_col = r_x1 / sh->cell_width; \
    first_row = r_y1 / sh->cell_height; \
    last_col = r_x2 / sh->cell_width; \
    last_row = r_y2 / sh->cell_height; \
    \
    first_col = clip(first_col, 0, SPATIALHASH_GRID_WIDTH-1); \
    first_row = clip(first_row, 0, SPATIALHASH_GRID_HEIGHT-1); \
    last_col = clip(last_col, 0, SPATIALHASH_GRID_WIDTH-1); \
    last_row = clip(last_row, 0, SPATIALHASH_GRID_HEIGHT-1); \
    \
    /* scanning persistent elements */ \
    for(p = sh->persistent_elements; p != NULL && !stop_iteration; p = p->next) { \
        if(0 != callback_function(p->data, some_user_data)) \
            stop_iteration = TRUE; \
    } \
    \
    /* scanning regular elements */ \
    stop_iteration = !((rectangle_width > 0) && (rectangle_height > 0)); \
    for(row=first_row; row<=last_row; row++) { \
        for(col=first_col; col<=last_col; col++) { \
            prev = NULL; \
            p = sh->bucket[row][col]; \
            \
            while(p != NULL && !stop_iteration) { \
                int cx, cy; \
                e_x1 = sh->xpos(p->data); \
                e_y1 = sh->ypos(p->data); \
                e_x2 = e_x1 + sh->width(p->data); \
                e_y2 = e_y1 + sh->height(p->data); \
                sh->largest_element_width = max(sh->largest_element_width, e_x2 - e_x1); \
                sh->largest_element_height = max(sh->largest_element_height, e_y2 - e_y1); \
                \
                cx = e_x1 / sh->cell_width; \
                cy = e_y1 / sh->cell_height; \
                cx = clip(cx, 0, SPATIALHASH_GRID_WIDTH-1); \
                cy = clip(cy, 0, SPATIALHASH_GRID_HEIGHT-1); \
                \
                if(cx >= first_col && cx <= last_col && cy >= first_row && cy <= last_row) { \
                    /* is p->data inside the given rectangle? (bounding box check) */ \
                    if((e_x1 <= r_x2 && e_x2 >= r_x1) && (e_y1 <= r_y2 && e_y2 >= r_y1)) { \
                        if((!stop_iteration) && (0 != callback_function(p->data, some_user_data))) \
                            stop_iteration = TRUE; \
                    } \
                } \
                \
                if(!(cx == col && cy == row)) { \
                    /* do we need to move p->data to some other bucket? */ \
                    T *e = p->data; \
                    if(prev != NULL) { \
                        prev->next = p->next; \
                        free(p); \
                        p = prev; \
                        spatialhash_##T##_add(sh, e); \
                    } \
                    else { \
                        sh->bucket[row][col] = p->next; \
                        free(p); \
                        p = sh->bucket[row][col]; \
                        spatialhash_##T##_add(sh, e); \
                        continue; \
                    } \
                } \
                \
                prev = p; \
                p = p->next; \
            } \
        } \
    } \
} \
/* similar to spatialhash_##T##_foreach, but this one retrieves all the elements stored in the spatial hash */ \
void spatialhash_##T##_forall(spatialhash_##T *sh, void *some_user_data, int (*callback_function)(T*,void*)) \
{ \
    spatialhash_##T##_foreach(sh, -LARGE_INT/2, -LARGE_INT/2, LARGE_INT, LARGE_INT, some_user_data, callback_function); \
}

#endif
