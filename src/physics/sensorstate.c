/*
 * Open Surge Engine
 * sensorstate.c - physics system: sensor state
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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

#include "sensorstate.h"
#include "physicsactor.h"
#include "obstacle.h"
#include "obstaclemap.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../util/util.h"

/* auxiliary structure: stores world coordinates */
typedef struct swpos_t
{
    int x1, y1, x2, y2;
} swpos_t;

/* sensorstate */
struct sensorstate_t
{
    const obstacle_t* (*check)(v2d_t,const obstaclemap_t*,int,int,int,int,obstaclelayer_t);
    void (*render)(v2d_t,v2d_t,int,int,int,int,color_t);
    swpos_t (*worldpos)(v2d_t,int,int,int,int);
};

/* private stuff ;-) */
static const obstacle_t* check(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, movmode_t mm, obstaclelayer_t layer_filter);
static void render(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color);
static swpos_t worldpos(v2d_t actor_position, int x1, int y1, int x2, int y2);
static color_t make_inverse_color(color_t color);

/* rotation methods */
static const obstacle_t* check_floormode(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter);
static void render_floormode(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color);
static swpos_t worldpos_floormode(v2d_t actor_position, int x1, int y1, int x2, int y2);
static const obstacle_t* check_rightwallmode(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter);
static void render_rightwallmode(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color);
static swpos_t worldpos_rightwallmode(v2d_t actor_position, int x1, int y1, int x2, int y2);
static const obstacle_t* check_ceilingmode(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter);
static void render_ceilingmode(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color);
static swpos_t worldpos_ceilingmode(v2d_t actor_position, int x1, int y1, int x2, int y2);
static const obstacle_t* check_leftwallmode(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter);
static void render_leftwallmode(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color);
static swpos_t worldpos_leftwallmode(v2d_t actor_position, int x1, int y1, int x2, int y2);

/* stateless states :P */
static sensorstate_t floormode = {
    check_floormode,
    render_floormode,
    worldpos_floormode
};

static sensorstate_t rightwallmode = {
    check_rightwallmode,
    render_rightwallmode,
    worldpos_rightwallmode
};

static sensorstate_t ceilingmode = {
    check_ceilingmode,
    render_ceilingmode,
    worldpos_ceilingmode
};

static sensorstate_t leftwallmode = {
    check_leftwallmode,
    render_leftwallmode,
    worldpos_leftwallmode
};



/* public methods */
sensorstate_t* sensorstate_create_floormode()
{
    return &floormode;
}

sensorstate_t* sensorstate_create_rightwallmode()
{
    return &rightwallmode;
}

sensorstate_t* sensorstate_create_ceilingmode()
{
    return &ceilingmode;
}

sensorstate_t* sensorstate_create_leftwallmode()
{
    return &leftwallmode;
}

sensorstate_t* sensorstate_destroy(sensorstate_t *sensorstate)
{
    return NULL;
}

const obstacle_t* sensorstate_check(const sensorstate_t *sensorstate, v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter)
{
    return sensorstate->check(actor_position, obstaclemap, x1, y1, x2, y2, layer_filter);
}

void sensorstate_render(const sensorstate_t *sensorstate, v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color)
{
    sensorstate->render(actor_position, camera_position, x1, y1, x2, y2, color);
}

void sensorstate_worldpos(const sensorstate_t *sensorstate, v2d_t actor_position, int *x1, int *y1, int *x2, int *y2)
{
    swpos_t position = sensorstate->worldpos(actor_position, *x1, *y1, *x2, *y2);

    *x1 = position.x1;
    *y1 = position.y1;
    *x2 = position.x2;
    *y2 = position.y2;
}


/* private stuff */
const obstacle_t* check(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, movmode_t mm, obstaclelayer_t layer_filter)
{
    x1 += (int)actor_position.x;
    y1 += (int)actor_position.y;
    x2 += (int)actor_position.x;
    y2 += (int)actor_position.y;

    return obstaclemap_get_best_obstacle_at(obstaclemap, min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2), mm, layer_filter);
}

void render(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color)
{
    v2d_t half_screen = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t topleft = v2d_subtract(camera_position, half_screen);

    x1 += (int)actor_position.x;
    y1 += (int)actor_position.y;
    x2 += (int)actor_position.x;
    y2 += (int)actor_position.y;

    x1 -= (int)topleft.x;
    y1 -= (int)topleft.y;
    x2 -= (int)topleft.x;
    y2 -= (int)topleft.y;

    image_rectfill(min(x1,x2), min(y1,y2), max(x1,x2), max(y1,y2), color);
    if(x1 != x2 || y1 != y2)
        image_rectfill(x2, y2, x2, y2, make_inverse_color(color)); /* render the tail (x2,y2) differently */
}

swpos_t worldpos(v2d_t actor_position, int x1, int y1, int x2, int y2)
{
    return (swpos_t){
        .x1 = x1 + (int)actor_position.x,
        .y1 = y1 + (int)actor_position.y,
        .x2 = x2 + (int)actor_position.x,
        .y2 = y2 + (int)actor_position.y
    };
}

color_t make_inverse_color(color_t color)
{
    uint8_t r, g, b, a;
    color_unmap(color, &r, &g, &b, &a);
    return color_rgba(255 - r, 255 - g, 255 - b, a);
}


/* rotation methods */
const obstacle_t* check_floormode(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter)
{
    /* clockwise rotation of 0 degrees. pivot = (0,0) */
    return check(actor_position, obstaclemap, x1, y1, x2, y2, MM_FLOOR, layer_filter);
}

void render_floormode(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color)
{
    render(actor_position, camera_position, x1, y1, x2, y2, color);
}

swpos_t worldpos_floormode(v2d_t actor_position, int x1, int y1, int x2, int y2)
{
    return worldpos(actor_position, x1, y1, x2, y2);
}

const obstacle_t* check_rightwallmode(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter)
{
    /* clockwise rotation of 90 degrees. pivot = (0,0) */
    return check(actor_position, obstaclemap, y1, -x1, y2, -x2, MM_RIGHTWALL, layer_filter);
}

void render_rightwallmode(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color)
{
    render(actor_position, camera_position, y1, -x1, y2, -x2, color);
}

swpos_t worldpos_rightwallmode(v2d_t actor_position, int x1, int y1, int x2, int y2)
{
    return worldpos(actor_position, y1, -x1, y2, -x2);
}

const obstacle_t* check_ceilingmode(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter)
{
    /* clockwise rotation of 180 degrees. pivot = (0,0) */
    return check(actor_position, obstaclemap, -x1, -y1, -x2, -y2, MM_CEILING, layer_filter);
}

void render_ceilingmode(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color)
{
    render(actor_position, camera_position, -x1, -y1, -x2, -y2, color);
}

swpos_t worldpos_ceilingmode(v2d_t actor_position, int x1, int y1, int x2, int y2)
{
    return worldpos(actor_position, -x1, -y1, -x2, -y2);
}

const obstacle_t* check_leftwallmode(v2d_t actor_position, const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, obstaclelayer_t layer_filter)
{
    /* clockwise rotation of 270 degrees. pivot = (0,0) */
    return check(actor_position, obstaclemap, -y1, x1, -y2, x2, MM_LEFTWALL, layer_filter);
}

void render_leftwallmode(v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color)
{
    render(actor_position, camera_position, -y1, x1, -y2, x2, color);
}

swpos_t worldpos_leftwallmode(v2d_t actor_position, int x1, int y1, int x2, int y2)
{
    return worldpos(actor_position, -y1, x1, -y2, x2);
}