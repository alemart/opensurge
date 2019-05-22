/*
 * Open Surge Engine
 * sprite.h - code for the sprites/animations
 * Copyright (C) 2008-2009  Alexandre Martins <alemartf@gmail.com>
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

#include <stdio.h>
#include "v2d.h"
#include "image.h"
#include "nanoparser/nanoparser.h"

typedef struct animation_t animation_t;
typedef struct spriteinfo_t spriteinfo_t;

/* animation */
/* this represents an animation */
struct animation_t {
    int id; /* id of the animation */
    int repeat; /* repeat animation? */
    float fps; /* frames per second */
    int frame_count; /* how many frames does this animation have? */
    int *data; /* frame vector */
    v2d_t hot_spot; /* hot spot */
    image_t **frame_data; /* reference to spriteinfo->frame_data */
    int repeat_from; /* n. if repeat == TRUE, jump back to the n-th frame of the animation. Defaults to zero */
};

/* sprite info */
/* spriteinfo_t represents only ONE sprite (meta data), with several animations */
struct spriteinfo_t {
    char *source_file;
    int rect_x, rect_y, rect_w, rect_h;
    int frame_w, frame_h;
    v2d_t hot_spot;

    int frame_count; /* every frame related to this sprite */
    image_t **frame_data; /* image_t* vector */

    int animation_count;
    animation_t **animation_data; /* animation_t* vector */
};



/* === sprite management: public methods === */

/* initializes the sprite module */
void sprite_init();

/* releases the sprite module */
void sprite_release();

/* returns the required animation */
animation_t *sprite_get_animation(const char *sprite_name, int anim_id);

/* returns the specified frame of the given animation */
struct image_t *sprite_get_image(const animation_t *anim, int frame_id);

/* checks if an animation exists */
int sprite_animation_exists(const char* sprite_name, int anim_id);







/* === spriteinfo_t class: public methods === */

/* creates an anonymous spriteinfo_t object by parsing the passed tree */
spriteinfo_t *spriteinfo_create(const parsetree_program_t *tree);

/* if you have called spriteinfo_create(), call this too when you're done with the sprite */
void spriteinfo_destroy(spriteinfo_t *info);

#endif
