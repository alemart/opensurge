/*
 * Open Surge Engine
 * sprite.h - sprite manager
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

#ifndef _SPRITE_H
#define _SPRITE_H

#include <stdbool.h>
#include "animation.h"
#include "image.h"
#include "../util/v2d.h"
#include "../util/darray.h"

typedef struct spriteinfo_t spriteinfo_t;
struct animation_t;
struct parsetree_program_t;

/* sprite info */
/* spriteinfo_t represents only ONE sprite (meta data), with several animations */
struct spriteinfo_t {
    char* source_file; /* path to image file */
    int rect_x; /* source rectangle: xpos */
    int rect_y; /* source rectangle: ypos */
    int rect_w; /* source rectangle: width */
    int rect_h; /* source rectangle: height */
    int frame_w; /* frame width */
    int frame_h; /* frame height */
    v2d_t default_hot_spot; /* default hot spot for all animations */
    v2d_t default_action_spot; /* default unflipped action spot for all animations */

    int frame_count; /* every frame related to this sprite */
    struct image_t** frame_data; /* image_t* vector */

    int animation_count;
    struct animation_t** animation_data; /* vector of animation_t* */

    DARRAY(struct animtransition_t*, transition); /* transitions */
    DARRAY(struct animtransition_t*, preprocessed_transition); /* transitions without "any" sorted by from_id and then by to_id */
    int* transition_from; /* transition_from[i] is the first index of preprocessed_transition having from_id == i, if it exists */
    int transition_from_length; /* length of transition_from[] */
};



/*
 * Sprite system
 */

/* initializes the sprite system */
void sprite_init();

/* releases the sprite system */
void sprite_release();

/* gets the required animation - crashes if not found */
const struct animation_t* sprite_get_animation(const char* sprite_name, int anim_id);

/* checks if an animation exists */
bool sprite_animation_exists(const char* sprite_name, int anim_id);



/*
 * Sprite metadata
 */

/* creates a spriteinfo_t given a parse tree */
spriteinfo_t* spriteinfo_create(const struct parsetree_program_t* tree);

/* releases a spriteinfo_t */
void spriteinfo_destroy(spriteinfo_t* info);

/* get an animation frame of the sprite */
const struct image_t* spriteinfo_get_animation_frame(const spriteinfo_t* info, int frame_index);

/* get an animation of the sprite */
const struct animation_t* spriteinfo_get_animation(const spriteinfo_t* info, int anim_id);

/* finds a transition animation in a sprite - returns NULL if there isn't any */
const struct animation_t* spriteinfo_find_transition_animation(const spriteinfo_t* info, int from_id, int to_id);

#endif
