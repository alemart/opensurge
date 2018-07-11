/*
 * Open Surge Engine
 * physics/obstaclemap.c - physics system: obstacle map
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

#include "obstaclemap.h"
#include "obstacle.h"
#include "physicsactor.h"
#include "../../core/util.h"
#include "../../core/video.h"
#include "../../core/darray.h"

/* an obstacle map is just a set of obstacles */
struct obstaclemap_t
{
    DARRAY(const obstacle_t*, obstacle);
};

/* private methods */
static const obstacle_t* pick_best_obstacle(const obstacle_t *a, const obstacle_t *b, int x1, int y1, int x2, int y2, movmode_t mm);

/* public methods */
obstaclemap_t* obstaclemap_create()
{
    obstaclemap_t *obstaclemap = mallocx(sizeof *obstaclemap);
    darray_init_ex(obstaclemap->obstacle, 32);
    return obstaclemap;
}

obstaclemap_t* obstaclemap_destroy(obstaclemap_t *obstaclemap)
{
    darray_release(obstaclemap->obstacle);
    free(obstaclemap);
    return NULL;
}

void obstaclemap_add_obstacle(obstaclemap_t *obstaclemap, const obstacle_t *obstacle)
{
    darray_push(obstaclemap->obstacle, obstacle);
}

const obstacle_t* obstaclemap_get_best_obstacle_at(const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, movmode_t mm)
{
    const obstacle_t *best = NULL;

    for(int i = 0; i < darray_length(obstaclemap->obstacle); i++) {
        /* the i-th obstacle collides with the sensor */
        if(obstacle_got_collision(obstaclemap->obstacle[i], x1, y1, x2, y2)) {
            /* the C standard mandates short-circuit evaluation */
            if((best == NULL) || pick_best_obstacle(obstaclemap->obstacle[i], best, x1, y1, x2, y2, mm) == obstaclemap->obstacle[i]) {
                /* the i-th obstacle is the best one (up until now) */
                best = obstaclemap->obstacle[i];
            }
        }
    }

    return best;
}

int obstaclemap_obstacle_exists(const obstaclemap_t* obstaclemap, int x, int y)
{
    for(int i = 0; i < darray_length(obstaclemap->obstacle); i++) {
        if(obstacle_got_collision(obstaclemap->obstacle[i], x, y, x, y))
            return TRUE;
    }

    return FALSE;
}

int obstaclemap_solid_exists(const obstaclemap_t* obstaclemap, int x, int y)
{
    for(int i = 0; i < darray_length(obstaclemap->obstacle); i++) {
        if(obstacle_got_collision(obstaclemap->obstacle[i], x, y, x, y) && obstacle_is_solid(obstaclemap->obstacle[i]))
            return TRUE;
    }

    return FALSE;
}

/* 2D raycasting */
const obstacle_t* obstaclemap_raycast(const obstaclemap_t* obstaclemap, v2d_t origin, v2d_t direction, float max_distance, v2d_t* hitpoint, float* distance)
{
    /* rays can't be larger than infty */
    const float infty = 2.0f * max(VIDEO_SCREEN_W, VIDEO_SCREEN_H);
    /*v2d_t p = origin;*/

    /* sanity checks */
    max_distance = clip(max_distance, 0.0f, infty);
    if(v2d_magnitude(direction) < EPSILON || max_distance < EPSILON)
        return NULL;

    /* TODO */

    /* 404 not found */
    return NULL;
}

/* private methods */

/* considering that a and b overlap, which one should we pick? */
const obstacle_t* pick_best_obstacle(const obstacle_t *a, const obstacle_t *b, int x1, int y1, int x2, int y2, movmode_t mm)
{
    static int (*w)(const obstacle_t*) = obstacle_get_width;
    static int (*h)(const obstacle_t*) = obstacle_get_height;
    int ha, hb, xa, xb, ya, yb, x, y;

    /* solid obstacles are better than one-way platforms */
    if(!obstacle_is_solid(a) && obstacle_is_solid(b))
        return b;

    if(!obstacle_is_solid(b) && obstacle_is_solid(a))
        return a;

    /* configuring */
    xa = (int)obstacle_get_position(a).x;
    xb = (int)obstacle_get_position(b).x;
    ya = (int)obstacle_get_position(a).y;
    yb = (int)obstacle_get_position(b).y;
    x = (x1+x2)/2; /* x1 == x2 in floor/ceiling mode */
    y = (y1+y2)/2; /* y1 == y2 in left/right wall mode */

    /* get the tallest obstacle */
    switch(mm) {
        case MM_FLOOR:
            ha = obstacle_get_height_at(a, x-xa, FROM_BOTTOM);
            hb = obstacle_get_height_at(b, x-xb, FROM_BOTTOM);
            return (ya + h(a) - ha <= yb + h(b) - hb) ? a : b;

        case MM_LEFTWALL:
            ha = obstacle_get_height_at(a, y-ya, FROM_LEFT);
            hb = obstacle_get_height_at(b, y-yb, FROM_LEFT);
            return (xa + ha >= xb + hb) ? a : b;

        case MM_CEILING:
            ha = obstacle_get_height_at(a, x-xa, FROM_TOP);
            hb = obstacle_get_height_at(b, x-xb, FROM_TOP);
            return (ya + ha >= yb + hb) ? a : b;

        case MM_RIGHTWALL:
            ha = obstacle_get_height_at(a, y-ya, FROM_RIGHT);
            hb = obstacle_get_height_at(b, y-yb, FROM_RIGHT);
            return (xa + w(a) - ha <= xb + w(b) - hb) ? a : b;
    }

    /* this shouldn't happen */
    return a;
}
