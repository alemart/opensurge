/*
 * Open Surge Engine
 * collisionmask.h - collision mask
 * Copyright (C) 2012, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

struct image_t;
struct collisionmask_t;
typedef struct collisionmask_t collisionmask_t;

/* create and destroy a collision mask */
collisionmask_t *collisionmask_create(const struct image_t *image, int x, int y, int width, int height);
collisionmask_t *collisionmask_destroy(collisionmask_t *cm);

/* retrieve dimensions */
int collisionmask_width(const collisionmask_t* cm);
int collisionmask_height(const collisionmask_t* cm);

/* collision checking
   WARNING: no boundary checking and no (cm == NULL) checking!! (for faster access) */
#define collisionmask_check(cm, x, y, w) *(*((char**)(cm)) + (y) * (w) + (x))

#endif
