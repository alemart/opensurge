/*
 * Open Surge Engine
 * obstacle.h - physics system: obstacles
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

#ifndef _OBSTACLE_H
#define _OBSTACLE_H

#include <stdbool.h>
#include "collisionmask.h"

/*
 * an obstacle may be anything "physical": a non-passable brick,
 * built-in item or custom object. The Physics Engine works with
 * obstacles only.
 */
struct obstacle_t;
typedef struct obstacle_t obstacle_t;

/* obstacle flags */
enum {
    OF_SOLID = 0x0,
    OF_CLOUD = 0x1,
    OF_HFLIP = 0x2,
    OF_VFLIP = 0x4
};

/* obstacle layer */
typedef enum obstaclelayer_t obstaclelayer_t;
enum obstaclelayer_t {
    OL_DEFAULT,
    OL_GREEN,
    OL_YELLOW
};

/* create and destroy */
obstacle_t* obstacle_create(const collisionmask_t *mask, int xpos, int ypos, obstaclelayer_t layer, int flags);
obstacle_t* obstacle_create_ex(const collisionmask_t* mask, int xpos, int ypos, obstaclelayer_t layer, int flags, void (*dtor)(void*), void *dtor_userdata);
obstacle_t* obstacle_destroy(obstacle_t *obstacle);

/* public methods */
void obstacle_get_position(const obstacle_t *obstacle, int *xpos, int *ypos); /* get position (in world coordinates) */
void obstacle_set_position(obstacle_t* obstacle, int xpos, int ypos); /* set position (in world coordinates) */
bool obstacle_is_solid(const obstacle_t *obstacle); /* is it solid or oneway? */
int obstacle_get_width(const obstacle_t *obstacle); /* width of the bounding box */
int obstacle_get_height(const obstacle_t *obstacle); /* height of the bounding box */
bool obstacle_got_collision(const obstacle_t *obstacle, int x1, int y1, int x2, int y2); /* check for collision with sensor (x1,y1,x2,y2); x1<=x2, y1<=y2 */
int obstacle_ground_position(const obstacle_t* obstacle, int x, int y, grounddir_t ground_direction); /* get the (absolute) ground position for world coordinates (x,y) */
obstaclelayer_t obstacle_get_layer(const obstacle_t *obstacle); /* get layer */

#endif
