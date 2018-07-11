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
#include "collisionmask.h"
#include "../../core/util.h"

/* obstacle struct */
struct obstacle_t
{
    v2d_t position;
    int width;
    int height;
    uint8 is_solid;
    uint16* height_map[4];
    const collisionmask_t* mask;
};

/* private */
static uint16* create_height_map(const collisionmask_t* mask, obstaclebaselevel_t base_level);
static uint16* destroy_height_map(uint16* height_map);

/* public methods */
obstacle_t* obstacle_create_solid(const collisionmask_t* mask, v2d_t position)
{
    obstacle_t *o = mallocx(sizeof *o);

    o->position = position;
    o->width = collisionmask_width(mask);
    o->height = collisionmask_height(mask);
    o->is_solid = TRUE;
    o->height_map[0] = create_height_map(mask, FROM_BOTTOM);
    o->height_map[1] = create_height_map(mask, FROM_LEFT);
    o->height_map[2] = create_height_map(mask, FROM_TOP);
    o->height_map[3] = create_height_map(mask, FROM_RIGHT);
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
    o->height_map[0] = create_height_map(mask, FROM_BOTTOM);
    o->height_map[1] = create_height_map(mask, FROM_LEFT);
    o->height_map[2] = create_height_map(mask, FROM_TOP);
    o->height_map[3] = create_height_map(mask, FROM_RIGHT);
    o->mask = mask;

    return o;
}

obstacle_t* obstacle_destroy(obstacle_t *obstacle)
{
    destroy_height_map(obstacle->height_map[3]);
    destroy_height_map(obstacle->height_map[2]);
    destroy_height_map(obstacle->height_map[1]);
    destroy_height_map(obstacle->height_map[0]);
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

int obstacle_get_height_at(const obstacle_t *obstacle, int position_on_base_axis, obstaclebaselevel_t base_level)
{
    /* get the cached height map */
    /* assume the base extends to infinity */
    switch(base_level) {
        case FROM_BOTTOM:
            if(position_on_base_axis >= 0 && position_on_base_axis < obstacle->width)
                return obstacle->height_map[0][position_on_base_axis]; 
            else if(position_on_base_axis >= obstacle->width)
                return obstacle->height_map[0][obstacle->width - 1];
            else
                return obstacle->height_map[0][0];

        case FROM_LEFT:
            if(position_on_base_axis >= 0 && position_on_base_axis < obstacle->height)
                return obstacle->height_map[1][position_on_base_axis]; 
            else if(position_on_base_axis >= obstacle->height)
                return obstacle->height_map[1][obstacle->height - 1];
            else
                return obstacle->height_map[1][0];

        case FROM_TOP:
            if(position_on_base_axis >= 0 && position_on_base_axis < obstacle->width)
                return obstacle->height_map[2][position_on_base_axis]; 
            else if(position_on_base_axis >= obstacle->width)
                return obstacle->height_map[2][obstacle->width - 1];
            else
                return obstacle->height_map[2][0];

        case FROM_RIGHT:
            if(position_on_base_axis >= 0 && position_on_base_axis < obstacle->height)
                return obstacle->height_map[3][position_on_base_axis]; 
            else if(position_on_base_axis >= obstacle->height)
                return obstacle->height_map[3][obstacle->height - 1];
            else
                return obstacle->height_map[3][0];

        default:
            return 0;
    }
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
        if((x1 != x2) || (y1 != y2)) {
            /* since y1 == y2 XOR x1 == x2, it's really a linear loop */
            int px, py;
            for(int y = y1; y <= y2; y++) {
                for(int x = x1; x <= x2; x++) {
                    px = x - o_x1;
                    py = y - o_y1;
                    if(px >= 0 && px < obstacle->width && py >= 0 && py < obstacle->height) {
                        if(collisionmask_check(mask, px, py, obstacle->width))
                            return TRUE;
                    }
                }
            }
        }
        else {
            /* single-pixel collision check */
            return collisionmask_check(mask, x1 - o_x1, y1 - o_y1, obstacle->width);
        }
    }

    /* no collision */
    return FALSE;
}


/* --- private --- */
uint16* create_height_map(const collisionmask_t* mask, obstaclebaselevel_t base_level)
{
    int w = collisionmask_width(mask);
    int h = collisionmask_height(mask);

    switch(base_level) {
        /* compute the height counting from the left side to the right side of the obstacle */
        /* +---------------+ */
        /* |               / */
        /* | ----->        \ */
        /* |               / */
        /* +--------------+  */
        case FROM_LEFT: {
            uint16* height = mallocx(h * sizeof(*height));
            for(int y = 0; y < h; y++) {
                height[y] = 0;
                for(int x = w-1; x >= 0; x--) {
                    if(collisionmask_check(mask, x, y, w)) {
                        height[y] = x;
                        break;
                    }
                }
            }
            return height;
        }

        /* compute the height counting from the top side to the bottom side of the obstacle */
        /*  +-----------------+  */
        /*  |         |       |  */
        /*  |        \|/      |  */
        /*  |                 |  */
        /*  |   __      ____  |  */
        /*  \__/  \_/\_/    \_/  */
        case FROM_TOP: {
            uint16* height = mallocx(w * sizeof(*height));
            for(int x = 0; x < w; x++) {
                height[x] = 0;
                for(int y = h-1; y >= 0; y--) {
                    if(collisionmask_check(mask, x, y, w)) {
                        height[x] = y;
                        break;
                    }
                }
            }
            return height;
        }

        /* compute the height counting from the right side to the left side of the obstacle */
        /* +---------------+ */
        /* \               | */
        /* /        <----- | */
        /* \               | */
        /* +---------------+ */
        case FROM_RIGHT: {
            uint16* height = mallocx(h * sizeof(*height));
            for(int y = 0; y < h; y++) {
                height[y] = 0;
                for(int x = 0; x < w; x++) {
                    if(collisionmask_check(mask, x, y, w)) {
                        height[y] = w-x;
                        break;
                    }
                }
            }
            return height;
        }

        /* compute the height counting from the bottom side to the top side of the obstacle */
        /*   __    __     _  _   */
        /*  /  \__/  \___/ \/ \  */
        /*  |                 |  */
        /*  |                 |  */
        /*  |      /|\        |  */
        /*  |       |         |  */
        /*  +-----------------+  */
        case FROM_BOTTOM: {
            uint16* height = mallocx(w * sizeof(*height));
            for(int x = 0; x < w; x++) {
                height[x] = 0;
                for(int y = 0; y < h; y++) {
                    if(collisionmask_check(mask, x, y, w)) {
                        height[x] = h-y;
                        break;
                    }
                }
            }
            return height;
        }

        /* this shouldn't happen */
        default:
            return NULL;
    }
}

uint16* destroy_height_map(uint16* height_map)
{
    if(height_map)
        free(height_map);
    return NULL;
}