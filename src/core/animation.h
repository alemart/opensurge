/*
 * Open Surge Engine
 * animation.h - animation system
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

#ifndef _ANIMATION_h
#define _ANIMATION_h

#include <stdbool.h>
#include "../util/v2d.h"

/* opaque types */
typedef struct animation_t animation_t;

/* forward declarations */
struct image_t;
struct spriteinfo_t;
struct transform_t;



/*
 * animations are defined in a stateless way
 */


/* the ID of the animation, as declared in a .spr file (typically) */
int animation_id(const animation_t* anim);

/* the FPS rate of the animation (frames per second) */
float animation_fps(const animation_t* anim);

/* the number of frames of the animation */
int animation_frame_count(const animation_t* anim);

/* the width, in pixels, of a frame of the animation */
int animation_frame_width(const animation_t* anim);

/* the height, in pixels, of a frame of the animation */
int animation_frame_height(const animation_t* anim);

/* does the animation repeat itself? (loop) */
bool animation_repeats(const animation_t* anim);

/* index of repeating frame; typically zero */
int animation_repeat_from(const animation_t* anim);

/* the hot spot of the animation, in pixels */
v2d_t animation_hot_spot(const animation_t* anim);

/* the unflipped action spot of the animation, in pixels */
v2d_t animation_action_spot(const animation_t* anim);

/* the sprite to which this animation belongs */
const struct spriteinfo_t* animation_sprite(const animation_t* anim);

/* returns the specified frame of the given animation */
const struct image_t* animation_image(const animation_t* anim, int frame_number);

/* returns an animation frame given a time in seconds (start time is zero) */
const struct image_t* animation_image_at_time(const animation_t* anim, double seconds);

/* the frame number at a given time in seconds (start time is zero) */
int animation_frame_at_time(const animation_t* anim, double seconds);

/* the time in which the given animation frame starts playing, in seconds */
double animation_start_time_of_frame(const animation_t* anim, int frame_number);

/* the index of an animation frame in the spritesheet */
int animation_frame_index(const animation_t* anim, int frame_number);

/* the duration of an animation, in seconds */
double animation_duration(const animation_t* anim);

/* checks if an animation at a given time is over */
bool animation_is_over(const animation_t* anim, double seconds);

/* is the animation a transition? */
bool animation_is_transition(const animation_t* anim);

/* gets a transition animation - returns NULL if there is no such transition */
const animation_t* animation_find_transition(const animation_t* from, const animation_t* to);

/* is this a keyframe-based animation? */
bool animation_has_keyframes(const animation_t* anim);

/* the interpolated transform of a keyframe-based animation */
struct transform_t* animation_interpolated_transform(const animation_t* anim, double seconds, struct transform_t* out_transform);

/* the interpolated translucency of a keyframe-based animation */
float animation_interpolated_translucency(const animation_t* anim, double seconds);

#endif