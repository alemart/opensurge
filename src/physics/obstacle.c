/*
 * Open Surge Engine
 * obstacle.c - physics system: obstacles
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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
#include "../util/util.h"

/* obstacle struct */
struct obstacle_t
{
    point2d_t position; /* position in world space */

    uint16_t width;
    uint16_t height;

    obstaclelayer_t layer;
    uint8_t flags;

    const collisionmask_t* mask;

    void (*dtor)(void*); /* optional destructor */
    void *dtor_userdata;
};

/* FLIP macro: x, y are input/output parameters */
#define FLIP(obstacle, x, y) do { \
    if((obstacle)->flags & OF_HFLIP) \
        x = (obstacle)->width - x - 1; \
    if((obstacle)->flags & OF_VFLIP) \
        y = (obstacle)->height - y - 1; \
} while(0)



/* public methods */
obstacle_t* obstacle_create(const collisionmask_t* mask, point2d_t position, obstaclelayer_t layer, int flags)
{
    return obstacle_create_ex(mask, position, layer, flags, NULL, NULL);
}

obstacle_t* obstacle_create_ex(const collisionmask_t* mask, point2d_t position, obstaclelayer_t layer, int flags, void (*dtor)(void*), void *dtor_userdata)
{
    obstacle_t *o = mallocx(sizeof *o);

    o->position = position;

    o->layer = layer;
    o->flags = flags;

    o->mask = mask;
    o->width = collisionmask_width(mask);
    o->height = collisionmask_height(mask);

    o->dtor = dtor;
    o->dtor_userdata = dtor_userdata;

    if(o->mask == NULL || o->width == 0 || o->height == 0)
        fatal_error("Obstacle with no mask / zero area"); /* this must never happen */

    return o;
}

obstacle_t* obstacle_destroy(obstacle_t *obstacle)
{
    if(obstacle->dtor != NULL)
        obstacle->dtor(obstacle->dtor_userdata);

    free(obstacle);
    return NULL;
}

point2d_t obstacle_get_position(const obstacle_t *obstacle)
{
    return obstacle->position;
}

void obstacle_set_position(obstacle_t* obstacle, point2d_t position)
{
    obstacle->position = position;
}

bool obstacle_is_solid(const obstacle_t *obstacle)
{
    return !(obstacle->flags & OF_CLOUD);
}

bool obstacle_is_static(const obstacle_t *obstacle)
{
    return !(obstacle->flags & OF_NONSTATIC);
}

int obstacle_get_width(const obstacle_t *obstacle)
{
    return obstacle->width;
}

int obstacle_get_height(const obstacle_t *obstacle)
{
    return obstacle->height;
}

obstaclelayer_t obstacle_get_layer(const obstacle_t *obstacle)
{
    return obstacle->layer;
}

/* find the ground position, given (x,y) in world coordinates */
/* if the ground direction is up or down, this returns the absolute y position of the ground */
/* if the ground direction is left or right, this returns the absolute x position of the ground */
int obstacle_ground_position(const obstacle_t* obstacle, int x, int y, grounddir_t ground_direction)
{
    /* no need to perform any clipping */
    x -= obstacle->position.x;
    y -= obstacle->position.y;
    FLIP(obstacle, x, y);

    /* flip the ground direction */
    bool is_vertical_ground_direction = ((ground_direction == GD_UP) || (ground_direction == GD_DOWN));
    bool is_horizontal_ground_direction = !is_vertical_ground_direction;
    if(
        ((obstacle->flags & OF_HFLIP) != 0 && is_horizontal_ground_direction) ||
        ((obstacle->flags & OF_VFLIP) != 0 && is_vertical_ground_direction)
    )
        ground_direction = FLIPPED_GROUNDDIR(ground_direction);

    /* get the absolute ground position */
    switch(ground_direction) {
        case GD_DOWN:
        case GD_UP:
            y = collisionmask_locate_ground(obstacle->mask, x, y, ground_direction);
            FLIP(obstacle, x, y);
            return obstacle->position.y + y;

        case GD_LEFT:
        case GD_RIGHT:
            x = collisionmask_locate_ground(obstacle->mask, x, y, ground_direction);
            FLIP(obstacle, x, y);
            return obstacle->position.x + x;
    }

    /* this shouldn't happen */
    return obstacle->position.y + obstacle->height - 1;
}

/* detects a pixel-perfect collision between an obstacle and a sensor
 * (x1, y1, x2, y2) are given in world-coordinates; also, x1 <= x2 and y1 <= y2 */
bool obstacle_got_collision(const obstacle_t *obstacle, int x1, int y1, int x2, int y2)
{
    /* this function needs to be highly performant! */
    int o_x1 = obstacle->position.x;
    int o_y1 = obstacle->position.y;
    int o_x2 = o_x1 + obstacle->width;
    int o_y2 = o_y1 + obstacle->height;

    /* assert: x1 == x2 or y1 == y2 */

    /* bounding box collision check */
    if(x1 < o_x2 && x2 >= o_x1 && y1 < o_y2 && y2 >= o_y1) {
        const collisionmask_t* mask = obstacle->mask;

        /* pixel perfect collision check */
        if(y1 != y2) {
            /* vertical sensor */
            if(x1 >= o_x1 && x1 < o_x2) {
                /* change of coordinates */
                int _y1 = max(y1, o_y1) - o_y1;
                int _y2 = min(y2, o_y2 - 1) - o_y1;
                int _x = x1 - o_x1;

                if((obstacle->flags & OF_VHFLIP) == 0) {
                    /* fast collision detection */
                    return collisionmask_area_test(mask, _x, _y1, _x, _y2);
                }
                else {
                    /* flip the sensor */
                    FLIP(obstacle, _x, _y1);
                    FLIP(obstacle, x1, _y2); /* x1 will be unused */
                    return collisionmask_area_test(mask, _x, min(_y1,_y2), _x, max(_y1,_y2));
                }
            }
        }
        else if(x1 != x2) {
            /* horizontal sensor */
            if(y1 >= o_y1 && y1 < o_y2) {
                /* change of coordinates */
                int _x1 = max(x1, o_x1) - o_x1;
                int _x2 = min(x2, o_x2 - 1) - o_x1;
                int _y = y1 - o_y1;

                if((obstacle->flags & OF_VHFLIP) == 0) {
                    /* fast collision detection */
                    return collisionmask_area_test(mask, _x1, _y, _x2, _y);
                }
                else {
                    /* flip the sensor */
                    FLIP(obstacle, _x1, _y);
                    FLIP(obstacle, _x2, y1); /* y1 will be unused */
                    return collisionmask_area_test(mask, min(_x1, _x2), _y, max(_x1, _x2), _y);
                }
            }
        }
        else {
            /* fast single-pixel collision check */
            int pitch = collisionmask_pitch(mask);
            int _x = x1 - o_x1;
            int _y = y1 - o_y1;

            FLIP(obstacle, _x, _y);
            return collisionmask_at(mask, _x, _y, pitch) != 0;
        }
    }

    /* no collision */
    return false;
}

/* check for collision with a point in world space */
bool obstacle_point_collision(const obstacle_t *obstacle, point2d_t point)
{
    return obstacle_got_collision(obstacle, point.x, point.y, point.x, point.y);
}