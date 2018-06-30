/*
 * Open Surge Engine
 * physics/obstacle.c - physics system: obstacles
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
#include "../collisionmask.h"
#include "../../core/util.h"
#include "../../core/video.h"
#include "../../core/image.h"

/* obstacle class */
struct obstacle_t
{
    v2d_t position;
    int width;
    int height;
    int angle;
    int is_solid;
    const collisionmask_t* mask;
};

/* public methods */
obstacle_t* obstacle_create_solid(const collisionmask_t* mask, int angle, v2d_t position)
{
    obstacle_t *o = mallocx(sizeof *o);

    o->position = position;
    o->width = collisionmask_width(mask);
    o->height = collisionmask_height(mask);
    o->angle = angle;
    o->is_solid = TRUE;
    o->mask = mask;

    return o;
}

obstacle_t* obstacle_create_oneway(const collisionmask_t* mask, int angle, v2d_t position)
{
    obstacle_t *o = mallocx(sizeof *o);

    o->position = position;
    o->width = collisionmask_width(mask);
    o->height = collisionmask_height(mask);
    o->angle = angle;
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

int obstacle_get_angle(const obstacle_t *obstacle)
{
    return obstacle->angle;
}

int obstacle_get_height_at(const obstacle_t *obstacle, int position_on_base_axis, obstaclebaselevel_t base_level)
{
    const collisionmask_t* mask = obstacle->mask;
    int w = obstacle->width;
    int h = obstacle->height;
    int x, y;

    switch(base_level) {

        /* will return the height counting from the left side to the right side of the obstacle */
        /* +---------------+ */
        /* |               / */
        /* | ----->        \ */
        /* |               / */
        /* +--------------+  */
        case FROM_LEFT:
            if(position_on_base_axis < 0 || position_on_base_axis >= h)
                return 0;

            y = position_on_base_axis;
            for(x=w-1; x>=0; x--) {
                if(collisionmask_check(mask, x, y))
                    break;
            }

            return x;

        /* will return the height counting from the top side to the bottom side of the obstacle */
        /*  +-----------------+  */
        /*  |         |       |  */
        /*  |        \|/      |  */
        /*  |                 |  */
        /*  |   __      ____  |  */
        /*  \__/  \_/\_/    \_/  */
        case FROM_TOP:
            if(position_on_base_axis < 0 || position_on_base_axis >= w)
                return 0;

            x = position_on_base_axis;
            for(y=h-1; y>=0; y--) {
                if(collisionmask_check(mask, x, y))
                    break;
            }

            return y;

        /* will return the height counting from the right side to the left side of the obstacle */
        /* +---------------+ */
        /* \               | */
        /* /        <----- | */
        /* \               | */
        /* +---------------+ */
        case FROM_RIGHT:
            if(position_on_base_axis < 0 || position_on_base_axis >= h)
                return 0;

            y = position_on_base_axis;
            for(x=0; x<w; x++) {
                if(collisionmask_check(mask, x, y))
                    break;
            }

            return w-x;

        /* will return the height counting from the bottom side to the top side of the obstacle */
        /*   __    __     _  _   */
        /*  /  \__/  \___/ \/ \  */
        /*  |                 |  */
        /*  |                 |  */
        /*  |      /|\        |  */
        /*  |       |         |  */
        /*  +-----------------+  */
        case FROM_BOTTOM:
            if(position_on_base_axis < 0 || position_on_base_axis >= w)
                return 0;

            x = position_on_base_axis;
            for(y=0; y<h; y++) {
                if(collisionmask_check(mask, x, y))
                    break;
            }

            return h-y;
    }

    /* this shouldn't happen */
    return 0;
}

/* detects a pixel-perfect collision between an obstacle and a sensor
 * (x1, y1, x2, y2) are given in world-coordinates; also, x1 <= x2 and y1 <= y2 */
int obstacle_got_collision(const obstacle_t *obstacle, int x1, int y1, int x2, int y2)
{
    /* bounding box collision */
    const collisionmask_t* mask = obstacle->mask;
    int o_x1 = (int)obstacle->position.x;
    int o_y1 = (int)obstacle->position.y;
    int o_x2 = o_x1 + obstacle->width;
    int o_y2 = o_y1 + obstacle->height;

    if(x1 < o_x2 && x2 >= o_x1 && y1 < o_y2 && y2 >= o_y1) {
        /* pixel perfect collision */
        int x, y, px, py;

        /* since y1 == y2 XOR x1 == x2, it's really a linear loop */
        for(y=y1; y<=y2; y++) {
            for(x=x1; x<=x2; x++) {
                px = x - o_x1;
                py = y - o_y1;
                if(px >= 0 && px < obstacle->width && py >= 0 && py < obstacle->height) {
                    if(collisionmask_check(mask, px, py))
                        return TRUE;
                }
            }
        }
    }

    return FALSE;
}