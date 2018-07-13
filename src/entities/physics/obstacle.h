/*
 * Open Surge Engine
 * physics/obstacle.h - physics system: obstacles
 * Copyright (C) 2011, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "../../core/v2d.h"
#include "collisionmask.h"

/*
 * an obstacle may be anything "physical": a non-passable brick,
 * built-in item or custom object. The Physics Engine works with
 * obstacles only.
 */
struct obstacle_t;
typedef struct obstacle_t obstacle_t;

/* auxiliary enumeration for obstacle_get_height_at() */
typedef enum obstaclebaselevel_t obstaclebaselevel_t;
enum obstaclebaselevel_t {
    FROM_BOTTOM,
    FROM_LEFT,
    FROM_TOP,
    FROM_RIGHT
};

/* create and destroy */
obstacle_t* obstacle_create_solid(const collisionmask_t *mask, v2d_t position);
obstacle_t* obstacle_create_oneway(const collisionmask_t *mask, v2d_t position);
obstacle_t* obstacle_destroy(obstacle_t *obstacle);

/* public methods */
v2d_t obstacle_get_position(const obstacle_t *obstacle); /* position (in world coordinates) */
int obstacle_is_solid(const obstacle_t *obstacle); /* is it solid or oneway? */
int obstacle_get_width(const obstacle_t *obstacle); /* width of the bounding box */
int obstacle_get_height(const obstacle_t *obstacle); /* height of the bounding box */
int obstacle_get_height_at(const obstacle_t *obstacle, int position_on_base_axis, obstaclebaselevel_t base_level); /* height map */
int obstacle_got_collision(const obstacle_t *obstacle, int x1, int y1, int x2, int y2); /* check for collision with sensor (x1,y1,x2,y2); x1<=x2, y1<=y2 */
int obstacle_ground_position(const obstacle_t* obstacle, int x, int y, grounddir_t ground_direction); /* get the (absolute) ground position for world coordinates (x,y) */

#endif
