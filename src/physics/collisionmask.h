/*
 * Open Surge Engine
 * collisionmask.h - collision mask
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

#ifndef _COLLISIONMASK_H
#define _COLLISIONMASK_H

#include <stdint.h>
#include <stdbool.h>
#include "../core/color.h"

struct image_t;
struct collisionmask_t;
typedef struct collisionmask_t collisionmask_t;

/* create and destroy a collision mask */
collisionmask_t* collisionmask_create(const struct image_t* image, int x, int y, int width, int height);
collisionmask_t* collisionmask_create_box(int width, int height);
collisionmask_t* collisionmask_destroy(collisionmask_t *mask);
collisionmask_t* collisionmask_clone(const collisionmask_t* mask);

/* retrieve dimensions */
int collisionmask_width(const collisionmask_t* mask);
int collisionmask_height(const collisionmask_t* mask);
int collisionmask_pitch(const collisionmask_t* mask);

/* collision checking */
#define collisionmask_at(mask, x, y, pitch) (*(*((const uint8_t**)(mask)) + (y) * (pitch) + (x))) /* fast pixel test with no boundary checking and no (mask == NULL) checking!! */
bool collisionmask_pixel_test(const collisionmask_t* mask, int x, int y); /* slower pixel test with boundary checking */
bool collisionmask_area_test(const collisionmask_t* mask, int left, int top, int right, int bottom); /* fast area test */

/* locating the ground */
typedef enum grounddir_t { GD_DOWN, GD_RIGHT, GD_UP, GD_LEFT } grounddir_t; /* analogous to a gravity vector (the default is "down") */
#define FLIPPED_GROUNDDIR(dir) (grounddir_t)(((dir) + 2) & 3) /* flip a grounddir_t */
int collisionmask_locate_ground(const collisionmask_t* mask, int x, int y, grounddir_t ground_direction);

/* misc */
struct image_t* collisionmask_to_image(const collisionmask_t* mask, color_t color);

#endif
