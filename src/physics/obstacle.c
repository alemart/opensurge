/*
 * Open Surge Engine
 * obstacle.c - physics system: obstacles
 * Copyright (C) 2011, 2018  Alexandre Martins <alemartf@gmail.com>
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

#include <stdint.h>
#include "obstacle.h"
#include "collisionmask.h"
#include "../core/util.h"

/* obstacle flags */
const int OF_SOLID = 0x0;
const int OF_CLOUD = 0x1;
const int OF_HFLIP = 0x2;
const int OF_VFLIP = 0x4;

/* obstacle struct */
struct obstacle_t
{
    int xpos;
    int ypos;
    uint16_t width;
    uint16_t height;
    uint8_t flags;
    const collisionmask_t* mask;
};

/* private utilities */
static inline void flip(const obstacle_t* obstacle, int *local_x, int *local_y, grounddir_t *ground_direction);
static inline grounddir_t flip_grounddir(grounddir_t ground_direction);

/* public methods */
obstacle_t* obstacle_create(const collisionmask_t* mask, int xpos, int ypos, int flags)
{
    obstacle_t *o = mallocx(sizeof *o);

    o->xpos = xpos;
    o->ypos = ypos;
    o->width = collisionmask_width(mask);
    o->height = collisionmask_height(mask);
    o->flags = flags;
    o->mask = mask;

    return o;
}

obstacle_t* obstacle_destroy(obstacle_t *obstacle)
{
    free(obstacle);
    return NULL;
}

void obstacle_get_position(const obstacle_t *obstacle, int *xpos, int *ypos)
{
    if(xpos != NULL)
        *xpos = obstacle->xpos;

    if(ypos != NULL)
        *ypos = obstacle->ypos;
}

void obstacle_set_position(obstacle_t* obstacle, int xpos, int ypos)
{
    obstacle->xpos = xpos;
    obstacle->ypos = ypos;
}

int obstacle_is_solid(const obstacle_t *obstacle)
{
    return !(obstacle->flags & OF_CLOUD);
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
    x -= obstacle->xpos;
    y -= obstacle->ypos;
    flip(obstacle, &x, &y, &ground_direction);

    /* get the absolute ground position */
    switch(ground_direction) {
        case GD_DOWN:
        case GD_UP:
            y = collisionmask_locate_ground(obstacle->mask, x, y, ground_direction);
            flip(obstacle, &x, &y, NULL);
            return obstacle->ypos + y;

        case GD_LEFT:
        case GD_RIGHT:
            x = collisionmask_locate_ground(obstacle->mask, x, y, ground_direction);
            flip(obstacle, &x, &y, NULL);
            return obstacle->xpos + x;
    }

    /* this shouldn't happen */
    return obstacle->ypos + obstacle->height - 1;
}

/* detects a pixel-perfect collision between an obstacle and a sensor
 * (x1, y1, x2, y2) are given in world-coordinates; also, x1 <= x2 and y1 <= y2 */
int obstacle_got_collision(const obstacle_t *obstacle, int x1, int y1, int x2, int y2)
{
    const collisionmask_t* mask = obstacle->mask;
    int o_x1 = obstacle->xpos;
    int o_y1 = obstacle->ypos;
    int o_x2 = o_x1 + obstacle->width;
    int o_y2 = o_y1 + obstacle->height;

    /* assert: x1 == x2 or y1 == y2 */

    /* bounding box collision check */
    if(x1 < o_x2 && x2 >= o_x1 && y1 < o_y2 && y2 >= o_y1) {
        int px, py;
        int pitch = collisionmask_pitch(mask);

        /* pixel perfect collision check */
        if(x1 != x2) {
            /* horizontal sensor */
            if(y1 >= o_y1 && y1 < o_y2) {
                int _x1 = max(x1, o_x1);
                int _x2 = min(x2, o_x2 - 1);
                for(int x = _x2; x >= _x1; x--) {
                    px = x - o_x1;
                    py = y1 - o_y1;
                    flip(obstacle, &px, &py, NULL);
                    if(collisionmask_at(mask, px, py, pitch))
                        return TRUE;
                }
            }
        }
        else if(y1 != y2) {
            /* vertical sensor */
            if(x1 >= o_x1 && x1 < o_x2) {
                int _y1 = max(y1, o_y1);
                int _y2 = min(y2, o_y2 - 1);
                for(int y = _y2; y >= _y1; y--) {
                    px = x1 - o_x1;
                    py = y - o_y1;
                    flip(obstacle, &px, &py, NULL);
                    if(collisionmask_at(mask, px, py, pitch))
                        return TRUE;
                }
            }
        }
        else {
            /* single-pixel collision check */
            px = x1 - o_x1;
            py = y1 - o_y1;
            flip(obstacle, &px, &py, NULL);
            return collisionmask_at(mask, px, py, pitch);
        }
    }

    /* no collision */
    return FALSE;
}


/* will flip the given output parameters according to the flip flags of the obstacle */
/* ground_direction may be NULL */
void flip(const obstacle_t* obstacle, int *local_x, int *local_y, grounddir_t *ground_direction)
{
    if(obstacle->flags != OF_SOLID) {
        if(obstacle->flags & OF_HFLIP)
            *local_x = obstacle->width - (*local_x) - 1;
        if(obstacle->flags & OF_VFLIP)
            *local_y = obstacle->height - (*local_y) - 1;
        if(ground_direction != NULL && (((obstacle->flags & OF_HFLIP) && (*ground_direction == GD_RIGHT || *ground_direction == GD_LEFT)) || ((obstacle->flags & OF_VFLIP) && (*ground_direction == GD_UP || *ground_direction == GD_DOWN))))
            *ground_direction = flip_grounddir(*ground_direction);
    }
}

/* the given ground_direction, flipped */
grounddir_t flip_grounddir(grounddir_t ground_direction)
{
    switch(ground_direction) {
        case GD_UP: return GD_DOWN;
        case GD_RIGHT: return GD_LEFT;
        case GD_DOWN: return GD_UP;
        case GD_LEFT: return GD_RIGHT;
        default: return GD_UP;
    }
}