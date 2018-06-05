/*
 * Open Surge Engine
 * physics/obstacle.c - physics system: obstacles
 * Copyright (C) 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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
    int (*is_solid)(const obstacle_t*);
    const image_t *image;
};

/* private methods */
static int solidobstacle_is_solid(const obstacle_t *obstacle);
static int onewayobstacle_is_solid(const obstacle_t *obstacle);

/* public methods */
obstacle_t* obstacle_create_solid(const image_t *image, int angle, v2d_t position)
{
    obstacle_t *o = mallocx(sizeof *o);

    o->position = position;
    o->width = image_width(image);
    o->height = image_height(image);
    o->angle = angle & 0xFF;
    o->is_solid = solidobstacle_is_solid;
    o->image = image;

    return o;
}

obstacle_t* obstacle_create_oneway(const image_t *image, int angle, v2d_t position)
{
    obstacle_t *o = mallocx(sizeof *o);

    o->position = position;
    o->width = image_width(image);
    o->height = image_height(image);
    o->angle = angle & 0xFF;
    o->is_solid = onewayobstacle_is_solid;
    o->image = image;

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
    return obstacle->is_solid(obstacle);
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
    uint32 mask = video_get_maskcolor();
    int w = obstacle_get_width(obstacle);
    int h = obstacle_get_height(obstacle);
    const image_t *image = obstacle_get_image(obstacle);
    int x, y;

    if(NULL == image)
        return 0;

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
                if(image_getpixel(image, x, y) != mask)
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
                if(image_getpixel(image, x, y) != mask)
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
                if(image_getpixel(image, x, y) != mask)
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
                if(image_getpixel(image, x, y) != mask)
                    break;
            }

            return h-y;
    }

    /* this shouldn't happen */
    return 0;
}

const image_t *obstacle_get_image(const obstacle_t *obstacle)
{
    return obstacle->image;
}

/* private methods */
int solidobstacle_is_solid(const obstacle_t *obstacle)
{
    return TRUE;
}

int onewayobstacle_is_solid(const obstacle_t *obstacle)
{
    return FALSE;
}
