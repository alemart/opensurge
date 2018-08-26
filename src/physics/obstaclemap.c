/*
 * Open Surge Engine
 * obstaclemap.c - physics system: obstacle map
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
#include "../core/util.h"
#include "../core/video.h"
#include "../core/darray.h"

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
        if(obstacle_got_collision(obstaclemap->obstacle[i], x1, y1, x2, y2))
            best = pick_best_obstacle(obstaclemap->obstacle[i], best, x1, y1, x2, y2, mm);
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
/* we know that x1 <= x2 and y1 <= y2; these values already come rotated according to the movmode */
const obstacle_t* pick_best_obstacle(const obstacle_t *a, const obstacle_t *b, int x1, int y1, int x2, int y2, movmode_t mm)
{
    int x, y, ha, hb;

    /* NULL pointers should be handled */
    if(a == NULL)
        return b;
    if(b == NULL)
        return a;

    /* solid obstacles are better than one-way platforms */
    if(!obstacle_is_solid(a) && obstacle_is_solid(b))
        return b;
    if(!obstacle_is_solid(b) && obstacle_is_solid(a))
        return a;

    /* one-way platforms only: get the shortest obstacle */
    if(!obstacle_is_solid(a) && !obstacle_is_solid(b)) {
        switch(mm) {
            case MM_FLOOR:
                ha = obstacle_ground_position(a, x2, y2, GD_DOWN);
                hb = obstacle_ground_position(b, x2, y2, GD_DOWN);
                return ha >= hb ? a : b;

            case MM_RIGHTWALL:
                ha = obstacle_ground_position(a, x2, y2, GD_RIGHT);
                hb = obstacle_ground_position(b, x2, y2, GD_RIGHT);
                return ha >= hb ? a : b;

            case MM_CEILING:
                ha = obstacle_ground_position(a, x2, y1, GD_UP);
                hb = obstacle_ground_position(b, x2, y1, GD_UP);
                return ha < hb ? a : b;
                
            case MM_LEFTWALL:
                ha = obstacle_ground_position(a, x1, y2, GD_LEFT);
                hb = obstacle_ground_position(b, x1, y2, GD_LEFT);
                return ha < hb ? a : b;
        }
    }

    /* solid obstacles: get the tallest one */
    switch(mm) {
        case MM_FLOOR:
            x = x2; /* x1 == x2 */
            y = y2; /* y2 == max(y1, y2) */
            ha = obstacle_ground_position(a, x, y, GD_DOWN);
            hb = obstacle_ground_position(b, x, y, GD_DOWN);
            return ha < hb ? a : b;

        case MM_LEFTWALL:
            x = x1; /* x1 == min(x1, x2) */
            y = y2; /* y1 == y2 */
            ha = obstacle_ground_position(a, x, y, GD_LEFT);
            hb = obstacle_ground_position(b, x, y, GD_LEFT);
            return ha >= hb ? a : b;

        case MM_CEILING:
            x = x2; /* x1 == x2 */
            y = y1; /* y1 == min(y1, y2) */
            ha = obstacle_ground_position(a, x, y, GD_UP);
            hb = obstacle_ground_position(b, x, y, GD_UP);
            return ha >= hb ? a : b;

        case MM_RIGHTWALL:
            x = x2; /* x2 == max(x1, x2) */
            y = y2; /* y1 == y2 */
            ha = obstacle_ground_position(a, x, y, GD_RIGHT);
            hb = obstacle_ground_position(b, x, y, GD_RIGHT);
            return ha < hb ? a : b;
    }

    /* this shouldn't happen */
    return a;
}
