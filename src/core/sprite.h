/*
 * Open Surge Engine
 * sprite.h - sprite manager
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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
#include "../util/rect.h"

/* forward declarations */
typedef struct spriteinfo_t spriteinfo_t;
struct parsetree_program_t;
struct collisionmask_t;
struct animation_t;
struct proganim_t;
struct image_t;



/*
 * Sprite system
 */

/* initializes the sprite system */
void sprite_init();

/* releases the sprite system */
void sprite_release();

/* checks if an animation exists */
bool sprite_animation_exists(const char* sprite_name, int anim_id);

/* gets the required animation - crashes if not found */
const struct animation_t* sprite_get_animation(const char* sprite_name, int anim_id);




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

/* get a programmatic animation, or NULL if none is defined with the given name */
const struct proganim_t* spriteinfo_get_proganim(const spriteinfo_t* info, const char* name);

/* gets a NULL-terminated array with the element(s) of a user-defined custom property, or NULL if no property with the given name exists */
const char* const* spriteinfo_user_property(const spriteinfo_t* info, const char* name);

/* the source file (image) of the sprite */
const char* spriteinfo_source_file(const spriteinfo_t* info);

/* the source rect of the sprite */
rect_t spriteinfo_source_rect(const spriteinfo_t* info);

/* the width of a frame of the sprite */
int spriteinfo_frame_width(const spriteinfo_t* info);

/* the height of a frame of the sprite */
int spriteinfo_frame_height(const spriteinfo_t* info);

/* create a collision mask from a frame of the spritesheet */
struct collisionmask_t* spriteinfo_to_collisionmask(const spriteinfo_t* info, int frame_index);

#endif