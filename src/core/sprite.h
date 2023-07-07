/*
 * Open Surge Engine
 * sprite.h - code for the sprites/animations
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
#include "nanoparser.h"
#include "image.h"
#include "../util/v2d.h"
#include "../util/darray.h"

typedef struct spriteinfo_t spriteinfo_t;
typedef struct animation_t animation_t;
struct animtransition_t; /* a helper struct representing a transition from one animation to another */

/* animation */
/* this represents an animation */
struct animation_t {
    const spriteinfo_t* sprite; /* this animation belongs to this sprite */
    int id; /* id of the animation */
    bool repeat; /* repeat animation? */
    float fps; /* frames per second */
    int frame_count; /* how many frames does this animation have? */
    int* data; /* frame vector */
    v2d_t hot_spot; /* hot spot */
    v2d_t action_spot; /* action spot */
    int repeat_from; /* if repeat is true, jump back to this frame of the animation. Defaults to zero */
    const animation_t *next; /* will be NULL, unless this is a transition animation */
};

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
    image_t **frame_data; /* image_t* vector */

    int animation_count;
    animation_t **animation_data; /* vector of animation_t* */

    DARRAY(struct animtransition_t*, transition); /* transitions */
};




/*
 * Sprite system
 */

/* initializes the sprite system */
void sprite_init();

/* releases the sprite system */
void sprite_release();

/* gets the required animation - crashes if not found */
const animation_t* sprite_get_animation(const char* sprite_name, int anim_id);

/* checks if an animation exists */
bool sprite_animation_exists(const char* sprite_name, int anim_id);




/*
 * Animation system
 */

/* returns the specified frame of the given animation */
const struct image_t* animation_get_image(const animation_t* anim, int frame_id);

/* gets a transition animation - returns NULL if there is no such transition */
const animation_t* animation_get_transition(const animation_t* from, const animation_t* to);

/* is anim a transition animation? */
bool animation_is_transition(const animation_t* anim);




/*
 * Sprite metadata
 */

/* creates an anonymous spriteinfo_t by parsing the input tree */
spriteinfo_t* spriteinfo_create(const parsetree_program_t* tree);

/* releases a spriteinfo_t */
void spriteinfo_destroy(spriteinfo_t* info);

#endif
