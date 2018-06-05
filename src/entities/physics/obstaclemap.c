/*
 * Open Surge Engine
 * physics/obstaclemap.c - physics system: obstacle map
 * Copyright (C) 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "obstaclemap.h"
#include "obstacle.h"
#include "physicsactor.h"
#include "../../core/util.h"
#include "../../core/image.h"
#include "../../core/video.h"

/* linked list of obstacles */
typedef struct obstacle_list_t obstacle_list_t;
struct obstacle_list_t
{
    obstacle_t *data;
    obstacle_list_t *next;
};

/* obstaclemap class */
struct obstaclemap_t
{
    obstacle_list_t *list;
};

/* private methods */
static int obstacle_sensor_collision(const obstacle_t *obstacle, int x1, int y1, int x2, int y2);
static const obstacle_t* pick_best_obstacle(const obstacle_t *a, const obstacle_t *b, int x1, int y1, int x2, int y2, movmode_t mm);

/* public methods */
obstaclemap_t* obstaclemap_create()
{
    obstaclemap_t *o = mallocx(sizeof *o);
    o->list = NULL;
    return o;
}

obstaclemap_t* obstaclemap_destroy(obstaclemap_t *obstaclemap)
{
    obstacle_list_t *l, *next;

    for(l = obstaclemap->list; l != NULL; l = next) {
        next = l->next;
        obstacle_destroy(l->data);
        free(l);
    }

    free(obstaclemap);
    return NULL;
}

void obstaclemap_add_obstacle(obstaclemap_t *obstaclemap, obstacle_t *obstacle)
{
    obstacle_list_t *l = mallocx(sizeof *l);
    l->data = obstacle;
    l->next = obstaclemap->list;
    obstaclemap->list = l;
}

const obstacle_t* obstaclemap_get_best_obstacle_at(const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, movmode_t mm)
{
    obstacle_t *o = NULL;
    obstacle_list_t *l;

    for(l = obstaclemap->list; l != NULL; l = l->next) {
        /* l->data is colliding with the sensor */
        if(obstacle_sensor_collision(l->data, x1, y1, x2, y2)) {
            /* l->data is better than o */
            if((!o) || (o && pick_best_obstacle(l->data, o, x1, y1, x2, y2, mm) == l->data))
                o = l->data;
        }
    }

    return o;
}


/* private methods */

/* detects a pixel-perfect collision between an obstacle and a sensor */
int obstacle_sensor_collision(const obstacle_t *obstacle, int x1, int y1, int x2, int y2)
{
    /* bounding box collision */
    int o_x1 = (int)obstacle_get_position(obstacle).x;
    int o_y1 = (int)obstacle_get_position(obstacle).y;
    int o_x2 = o_x1 + obstacle_get_width(obstacle);
    int o_y2 = o_y1 + obstacle_get_height(obstacle);
    const image_t *img = obstacle_get_image(obstacle);

    if(x1 < o_x2 && x2 >= o_x1 && y1 < o_y2 && y2 >= o_y1 && img != NULL) {
        /* pixel perfect collision */
        int x, y, px, py;
        uint32 mask = video_get_maskcolor();

        /* since y1 == y2 XOR x1 == x2, it's really a linear loop */
        for(y=y1; y<=y2; y++) {
            for(x=x1; x<=x2; x++) {
                px = x - o_x1;
                py = y - o_y1;
                if(px >= 0 && px < image_width(img) && py >= 0 && py < image_height(img)) {
                    if(image_getpixel(img, px, py) != mask)
                        return TRUE;
                }
            }
        }
    }

    return FALSE;
}

/* considering that a and b overlap, which one should we pick? */
const obstacle_t* pick_best_obstacle(const obstacle_t *a, const obstacle_t *b, int x1, int y1, int x2, int y2, movmode_t mm)
{
    int ha, hb, xa, xb, ya, yb, x, y;
    int (*w)(const obstacle_t*) = obstacle_get_width;
    int (*h)(const obstacle_t*) = obstacle_get_height;

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
            return (ya + (h(a)-1) - ha < yb + (h(b)-1) - hb) ? a : b;

        case MM_LEFTWALL:
            ha = obstacle_get_height_at(a, y-ya, FROM_LEFT);
            hb = obstacle_get_height_at(b, y-yb, FROM_LEFT);
            return (xa + ha > xb + hb) ? a : b;

        case MM_CEILING:
            ha = obstacle_get_height_at(a, x-xa, FROM_TOP);
            hb = obstacle_get_height_at(b, x-xb, FROM_TOP);
            return (ya + ha > yb + hb) ? a : b;

        case MM_RIGHTWALL:
            ha = obstacle_get_height_at(a, y-ya, FROM_RIGHT);
            hb = obstacle_get_height_at(b, y-yb, FROM_RIGHT);
            return (xa + (w(a)-1) - ha < xb + (w(b)-1) - hb) ? a : b;
    }

    /* this shouldn't happen */
    return a;
}
