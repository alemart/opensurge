/*
 * Open Surge Engine
 * sensor.c - physics system: sensors
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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

#include "sensor.h"
#include "sensorstate.h"
#include "obstaclemap.h"
#include "obstacle.h"
#include "physicsactor.h"
#include "../core/util.h"

/* sensor class */
struct sensor_t
{
    /* a sensor is a segment (x1,y1)~(x2,y2) such that x1=x2 or y1=y2 */
    int x1, y1, x2, y2; /* coordinates relative to the physics actor */
    color_t color; /* color of the sensor (used for rendering) */

    sensorstate_t *floormode;
    sensorstate_t *rightwallmode;
    sensorstate_t *ceilingmode;
    sensorstate_t *leftwallmode;
};

/* private stuff ;-) */
static sensorstate_t* get_active_state(const sensor_t *sensor, movmode_t mm);

/* public methods */
sensor_t* sensor_create_horizontal(int y, int x1, int x2, color_t color)
{
    sensor_t *s = mallocx(sizeof *s);

    s->x1 = min(x1,x2);
    s->y1 = y;
    s->x2 = max(x1,x2);
    s->y2 = y;
    s->color = color;

    s->floormode = sensorstate_create_floormode();
    s->rightwallmode = sensorstate_create_rightwallmode();
    s->ceilingmode = sensorstate_create_ceilingmode();
    s->leftwallmode = sensorstate_create_leftwallmode();

    return s;
}

sensor_t* sensor_create_vertical(int x, int y1, int y2, color_t color)
{
    sensor_t *s = mallocx(sizeof *s);

    s->x1 = x;
    s->y1 = min(y1,y2);
    s->x2 = x;
    s->y2 = max(y1,y2);
    s->color = color;

    s->floormode = sensorstate_create_floormode();
    s->rightwallmode = sensorstate_create_rightwallmode();
    s->ceilingmode = sensorstate_create_ceilingmode();
    s->leftwallmode = sensorstate_create_leftwallmode();

    return s;
}

sensor_t* sensor_destroy(sensor_t *sensor)
{
    sensorstate_destroy(sensor->floormode);
    sensorstate_destroy(sensor->rightwallmode);
    sensorstate_destroy(sensor->ceilingmode);
    sensorstate_destroy(sensor->leftwallmode);

    free(sensor);
    return NULL;
}


const obstacle_t* sensor_check(const sensor_t *sensor, v2d_t actor_position, movmode_t mm, const obstaclemap_t *obstaclemap)
{
    sensorstate_t *s = get_active_state(sensor, mm);
    return sensorstate_check(s, actor_position, obstaclemap, sensor->x1, sensor->y1, sensor->x2, sensor->y2);
}

void sensor_render(const sensor_t *sensor, v2d_t actor_position, movmode_t mm, v2d_t camera_position)
{
    sensorstate_t *s = get_active_state(sensor, mm);
    sensorstate_render(s, actor_position, camera_position, sensor->x1, sensor->y1, sensor->x2, sensor->y2, sensor->color);
}

void sensor_worldpos(const sensor_t* sensor, v2d_t actor_position, movmode_t mm, int *x1, int *y1, int *x2, int *y2)
{
    sensorstate_t *s = get_active_state(sensor, mm);
    int xa = sensor->x1, ya = sensor->y1;
    int xb = sensor->x2, yb = sensor->y2;

    sensorstate_worldpos(s, actor_position, &xa, &ya, &xb, &yb);

    if(x1) *x1 = xa;
    if(y1) *y1 = ya;
    if(x2) *x2 = xb;
    if(y2) *y2 = yb;
}

int sensor_get_x1(const sensor_t *sensor)
{
    return sensor->x1;
}

int sensor_get_y1(const sensor_t *sensor)
{
    return sensor->y1;
}

int sensor_get_x2(const sensor_t *sensor)
{
    return sensor->x2;
}

int sensor_get_y2(const sensor_t *sensor)
{
    return sensor->y2;
}

color_t sensor_get_color(const sensor_t *sensor)
{
    return sensor->color;
}

/* private methods */
sensorstate_t* get_active_state(const sensor_t *sensor, movmode_t mm)
{
    switch(mm) {
        case MM_FLOOR:
            return sensor->floormode;

        case MM_RIGHTWALL:
            return sensor->rightwallmode;

        case MM_CEILING:
            return sensor->ceilingmode;

        case MM_LEFTWALL:
            return sensor->leftwallmode;
    }

    return sensor->floormode;
}
