/*
 * Open Surge Engine
 * sensor.h - physics system: sensors
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

#ifndef _SENSOR_H
#define _SENSOR_H

#include <stdbool.h>
#include "../util/v2d.h"
#include "../util/point2d.h"
#include "../core/color.h"

/*
 * a sensor detects collisions between the
 * obstacle map and itself
 */
typedef struct sensor_t sensor_t;

/* forward declarations */
struct obstacle_t;
struct obstaclemap_t;
enum obstaclelayer_t;
enum movmode_t;

/* create and destroy */
sensor_t* sensor_create_horizontal(int y, int head_x, int tail_x, color_t color); /* factory method: new horizontal sensor */
sensor_t* sensor_create_vertical(int x, int head_y, int tail_y, color_t color); /* factory method: new vertical sensor */
sensor_t* sensor_destroy(sensor_t *sensor);

/* non-rotation-based methods */
int sensor_get_x1(const sensor_t *sensor);
int sensor_get_y1(const sensor_t *sensor);
int sensor_get_x2(const sensor_t *sensor);
int sensor_get_y2(const sensor_t *sensor);
point2d_t sensor_local_head(const sensor_t* sensor);
point2d_t sensor_local_tail(const sensor_t* sensor);
color_t sensor_color(const sensor_t *sensor);
bool sensor_is_enabled(const sensor_t* sensor);
void sensor_set_enabled(sensor_t* sensor, bool enabled);

/* rotation-based methods */
const struct obstacle_t* sensor_check(const sensor_t *sensor, v2d_t actor_position, enum movmode_t mm, enum obstaclelayer_t layer_filter, const struct obstaclemap_t *obstaclemap); /* returns NULL if no obstacle was found */
void sensor_render(const sensor_t *sensor, v2d_t actor_position, enum movmode_t mm, v2d_t camera_position);
void sensor_worldpos(const sensor_t* sensor, v2d_t actor_position, enum movmode_t mm, int *x1, int *y1, int *x2, int *y2);
point2d_t sensor_head(const sensor_t* sensor, v2d_t actor_position, enum movmode_t mm);
point2d_t sensor_tail(const sensor_t* sensor, v2d_t actor_position, enum movmode_t mm);
void sensor_extend(const sensor_t* sensor, v2d_t actor_position, enum movmode_t mm, int extended_length, point2d_t* extended_head, point2d_t* extended_tail);

#endif
