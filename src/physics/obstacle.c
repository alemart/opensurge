/*
 * Open Surge Engine
 * obstacle.c - physics system: obstacles
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

#include "obstacle.h"
#include "collisionmask.h"
#include "../core/util.h"

/* obstacle struct */
struct obstacle_t
{
    v2d_t position;
    int width;
    int height;
    uint8 is_solid;
    const collisionmask_t* mask;
};

/* public methods */
obstacle_t* obstacle_create_solid(const collisionmask_t* mask, v2d_t position)
{
    obstacle_t *o = mallocx(sizeof *o);

    o->position = position;
    o->width = collisionmask_width(mask);
    o->height = collisionmask_height(mask);
    o->is_solid = TRUE;
    o->mask = mask;

    return o;
}

obstacle_t* obstacle_create_oneway(const collisionmask_t* mask, v2d_t position)
{
    obstacle_t *o = mallocx(sizeof *o);

    o->position = position;
    o->width = collisionmask_width(mask);
    o->height = collisionmask_height(mask);
    o->is_solid = FALSE;
    o->mask = mask;

    return o;
}

obstacle_t* obstacle_destroy(obstacle_t *obstacle)
{
    free(obstacle);
    return NULL;
}

v2d_t obstacle_get_position(const obstacle_t *obstacle)
{
    return obstacle->position;
}

void obstacle_set_position(obstacle_t* obstacle, v2d_t position)
{
    obstacle->position = position;
}

int obstacle_is_solid(const obstacle_t *obstacle)
{
    return obstacle->is_solid;
}

int obstacle_get_width(const obstacle_t *obstacle)
{
    return obstacle->width;
}

int obstacle_get_height(const obstacle_t *obstacle)
{
    return obstacle->height;
}

/* find the ground position, given (x,y) in world coordinates */
/* if the ground direction is up or down, this returns the absolute y position of the ground */
/* if the ground direction is left or right, this returns the absolute x position of the ground */
int obstacle_ground_position(const obstacle_t* obstacle, int x, int y, grounddir_t ground_direction)
{
    /* no need to perform any clipping */
    int ox = (int)obstacle->position.x;
    int oy = (int)obstacle->position.y;

    /* get the absolute ground position */
    switch(ground_direction) {
        case GD_DOWN:
        case GD_UP:
            return oy + collisionmask_locate_ground(obstacle->mask, x - ox, y - oy, ground_direction);

        case GD_LEFT:
        case GD_RIGHT:
            return ox + collisionmask_locate_ground(obstacle->mask, x - ox, y - oy, ground_direction);
    }

    /* this shouldn't happen */
    return oy + obstacle->height - 1;
}

/* detects a pixel-perfect collision between an obstacle and a sensor
 * (x1, y1, x2, y2) are given in world-coordinates; also, x1 <= x2 and y1 <= y2 */
int obstacle_got_collision(const obstacle_t *obstacle, int x1, int y1, int x2, int y2)
{
    const collisionmask_t* mask = obstacle->mask;
    int o_x1 = (int)obstacle->position.x;
    int o_y1 = (int)obstacle->position.y;
    int o_x2 = o_x1 + obstacle->width;
    int o_y2 = o_y1 + obstacle->height;

    /* bounding box collision check */
    if(x1 < o_x2 && x2 >= o_x1 && y1 < o_y2 && y2 >= o_y1) {
        /* pixel perfect collision check */
        int pitch = collisionmask_pitch(mask);
        if((x1 != x2) || (y1 != y2)) {
            /* since y1 == y2 XOR x1 == x2, it's really a linear loop */
            int px, py, x, y;
            for(y = y2; y >= y1; y--) {
                for(x = x2; x >= x1; x--) {
                    px = x - o_x1;
                    py = y - o_y1;
                    if(px >= 0 && px < obstacle->width && py >= 0 && py < obstacle->height) {
                        if(collisionmask_at(mask, px, py, pitch))
                            return TRUE;
                    }
                }
            }
        }
        else {
            /* single-pixel collision check */
            return collisionmask_at(mask, x1 - o_x1, y1 - o_y1, pitch);
        }
    }

    /* no collision */
    return FALSE;
}