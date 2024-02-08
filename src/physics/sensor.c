/*
 * Open Surge Engine
 * sensor.c - physics system: sensors
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

#include "sensor.h"
#include "sensorstate.h"
#include "obstaclemap.h"
#include "obstacle.h"
#include "physicsactor.h"
#include "../util/util.h"

/* sensor struct */
struct sensor_t
{
    /* a sensor is an oriented segment [head ---> tail] such that
       head.x == tail.x or head.y == tail.y */
    point2d_t local_head, local_tail; /* coordinates relative to the physics actor */
    color_t color; /* color of the sensor (used for rendering) */
    bool enabled; /* is the sensor enabled? it is by default */

    /* states */
    sensorstate_t *floormode;
    sensorstate_t *rightwallmode;
    sensorstate_t *ceilingmode;
    sensorstate_t *leftwallmode;
};

/* private stuff ;-) */
static sensorstate_t* select_state(const sensor_t *sensor, movmode_t mm);
static color_t make_translucent_color(color_t color, float alpha);


/*
 * public
 */

/*
 * sensor_create_horizontal()
 * Create a horizontal sensor
 */
sensor_t* sensor_create_horizontal(int y, int head_x, int tail_x, color_t color)
{
    sensor_t *s = mallocx(sizeof *s);

    s->local_head = point2d_new(head_x, y);
    s->local_tail = point2d_new(tail_x, y);
    s->color = color;
    s->enabled = true;

    s->floormode = sensorstate_create_floormode();
    s->rightwallmode = sensorstate_create_rightwallmode();
    s->ceilingmode = sensorstate_create_ceilingmode();
    s->leftwallmode = sensorstate_create_leftwallmode();

    return s;
}

/*
 * sensor_create_vertical()
 * Create a vertical sensor
 */
sensor_t* sensor_create_vertical(int x, int head_y, int tail_y, color_t color)
{
    sensor_t *s = mallocx(sizeof *s);

    s->local_head = point2d_new(x, head_y);
    s->local_tail = point2d_new(x, tail_y);
    s->color = color;
    s->enabled = true;

    s->floormode = sensorstate_create_floormode();
    s->rightwallmode = sensorstate_create_rightwallmode();
    s->ceilingmode = sensorstate_create_ceilingmode();
    s->leftwallmode = sensorstate_create_leftwallmode();

    return s;
}

/*
 * sensor_destroy()
 * Destroy a sensor
 */
sensor_t* sensor_destroy(sensor_t *sensor)
{
    sensorstate_destroy(sensor->floormode);
    sensorstate_destroy(sensor->rightwallmode);
    sensorstate_destroy(sensor->ceilingmode);
    sensorstate_destroy(sensor->leftwallmode);

    free(sensor);
    return NULL;
}

/*
 * sensor_get_x1()
 * Read local coordinate x1
 */
int sensor_get_x1(const sensor_t *sensor)
{
    return sensor->local_head.x;
}

/*
 * sensor_get_y1()
 * Read local coordinate y1
 */
int sensor_get_y1(const sensor_t *sensor)
{
    return sensor->local_head.y;
}

/*
 * sensor_get_x2()
 * Read local coordinate x2
 */
int sensor_get_x2(const sensor_t *sensor)
{
    return sensor->local_tail.x;
}

/*
 * sensor_get_y2()
 * Read local coordinate y2
 */
int sensor_get_y2(const sensor_t *sensor)
{
    return sensor->local_tail.y;
}

/*
 * sensor_local_head()
 * The position of the head of the sensor relative to the physics actor;
 * not rotated
 */
point2d_t sensor_local_head(const sensor_t* sensor)
{
    return sensor->local_head;
}

/*
 * sensor_local_tail()
 * The position of the tail of the sensor relative to the physics actor;
 * not rotated
 */
point2d_t sensor_local_tail(const sensor_t* sensor)
{
    return sensor->local_tail;
}

/*
 * sensor_color()
 * The color of the sensor
 */
color_t sensor_color(const sensor_t *sensor)
{
    return sensor->color;
}

/*
 * sensor_is_enabled()
 * Will the sensor detect collisions?
 */
bool sensor_is_enabled(const sensor_t* sensor)
{
    return sensor->enabled;
}

/*
 * sensor_set_enable()
 * Enable or disable the sensor
 */
void sensor_set_enabled(sensor_t* sensor, bool enabled)
{
    sensor->enabled = enabled;
}

/*
 * sensor_check()
 * Find an obstacle that collides with the sensor
 * Returns NULL if there is no such obstacle
 */
const obstacle_t* sensor_check(const sensor_t *sensor, v2d_t actor_position, movmode_t mm, obstaclelayer_t layer_filter, const obstaclemap_t *obstaclemap)
{
    if(!sensor->enabled)
        return NULL;

    int x1 = sensor->local_head.x;
    int y1 = sensor->local_head.y;
    int x2 = sensor->local_tail.x;
    int y2 = sensor->local_tail.y;

    sensorstate_t *s = select_state(sensor, mm);
    return sensorstate_check(s, actor_position, obstaclemap, x1, y1, x2, y2, layer_filter);
}

/*
 * sensor_render()
 * Render the sensor
 */
void sensor_render(const sensor_t *sensor, v2d_t actor_position, movmode_t mm, v2d_t camera_position)
{
    int x1 = sensor->local_head.x;
    int y1 = sensor->local_head.y;
    int x2 = sensor->local_tail.x;
    int y2 = sensor->local_tail.y;

    color_t color = sensor->color;
    if(!sensor->enabled)
        color = make_translucent_color(color, 0.25f);

    sensorstate_t *s = select_state(sensor, mm);
    sensorstate_render(s, actor_position, camera_position, x1, y1, x2, y2, color);
}

/*
 * sensor_worldpos()
 * Read the position of the sensor in world space, performing the appropriate
 * rotations according to the movmode. Output coordinates are NOT guaranteed to
 * be such that x1 <= x2 and y1 <= y2. (x1,y1) is the head; (x2,y2), the tail
 */
void sensor_worldpos(const sensor_t* sensor, v2d_t actor_position, movmode_t mm, int *x1, int *y1, int *x2, int *y2)
{
    int xa = sensor->local_head.x;
    int ya = sensor->local_head.y;
    int xb = sensor->local_tail.x;
    int yb = sensor->local_tail.y;

    sensorstate_t *s = select_state(sensor, mm);
    sensorstate_worldpos(s, actor_position, &xa, &ya, &xb, &yb);

    if(x1) *x1 = xa;
    if(y1) *y1 = ya;
    if(x2) *x2 = xb;
    if(y2) *y2 = yb;
}

/*
 * sensor_overlaps_obstacle()
 * Check if the sensor is overlapping an obstacle
 */
bool sensor_overlaps_obstacle(const sensor_t* sensor, v2d_t actor_position, movmode_t mm, obstaclelayer_t layer_filter, const obstacle_t* obstacle)
{
    int x1, y1, x2, y2;
    obstaclelayer_t layer;

    sensor_worldpos(sensor, actor_position, mm, &x1, &y1, &x2, &y2);
    layer = obstacle_get_layer(obstacle);

    return (
        layer == OL_DEFAULT ||
        layer_filter == OL_DEFAULT ||
        layer_filter == layer
     ) && obstacle_got_collision(obstacle, min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2));
}

/*
 * sensor_head()
 * Read the position of the head of the sensor in world space, performing the
 * appropriate rotations according to the movmode
 */
point2d_t sensor_head(const sensor_t* sensor, v2d_t actor_position, movmode_t mm)
{
    point2d_t p;
    sensor_worldpos(sensor, actor_position, mm, &p.x, &p.y, NULL, NULL);
    return p;
}

/*
 * sensor_tail()
 * Read the position of the head of the sensor in world space, performing the
 * appropriate rotations according to the movmode
 */
point2d_t sensor_tail(const sensor_t* sensor, v2d_t actor_position, movmode_t mm)
{
    point2d_t p;
    sensor_worldpos(sensor, actor_position, mm, NULL, NULL, &p.x, &p.y);
    return p;
}

/*
 * sensor_extend()
 * Analogous to sensor_worldpos(), except that the returned segment will:
 *
 * a) have its head be the tail of the sensor
 * b) grow from the tail of the sensor away from its head
 * c) have length extended_length (given as input)
 *
 * If extended_length is negative, the returned segment will grow from the tail
 * of the sensor towards its head and will have length -extended_length
 */
void sensor_extend(const sensor_t* sensor, v2d_t actor_position, movmode_t mm, int extended_length, point2d_t* extended_head, point2d_t* extended_tail)
{
    /* read the head and the tail in world space */
    point2d_t head, tail;
    sensor_worldpos(sensor, actor_position, mm, &head.x, &head.y, &tail.x, &tail.y);

    /* compute the normalized direction of the sensor:
       (0,-1), (1,0), (0,1), (-1,0)

       if head == tail, that's a single point that will not be extended */
    point2d_t dir = point2d_subtract(tail, head); /* either vertical or horizontal */
    dir.x = (dir.x > 0) - (dir.x < 0);
    dir.y = (dir.y > 0) - (dir.y < 0);
    assertx(dir.x * dir.x + dir.y * dir.y <= 1); /* unit vector or zero */

    /* compute lambda */
    int lambda;
    if(extended_length > 0)
        lambda = extended_length - 1; /* non-negative */
    else if(extended_length < 0)
        lambda = extended_length + 1; /* non-positive */
    else
        lambda = 0;

    /* extend the sensor */
    *extended_head = tail;
    extended_tail->x = extended_head->x + dir.x * lambda;
    extended_tail->y = extended_head->y + dir.y * lambda;
}



/*
 * private
 */

sensorstate_t* select_state(const sensor_t *sensor, movmode_t mm)
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

color_t make_translucent_color(color_t color, float alpha)
{
    uint8_t r, g, b, a;
    color_unmap(color, &r, &g, &b, &a);
    return color_premul_rgba(r, g, b, (uint8_t)(255.0f * alpha));
}