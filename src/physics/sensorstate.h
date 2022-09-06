/*
 * Open Surge Engine
 * sensorstate.h - physics system: sensor state
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _SENSORSTATE_H
#define _SENSORSTATE_H

#include "../core/v2d.h"
#include "../core/color.h"

/*
 * there are four modes of movement:
 * - floor mode
 * - right wall mode
 * - ceiling mode
 * - left wall mode
 */
typedef struct sensorstate_t sensorstate_t;

/* forward declarations */
struct obstacle_t;
struct obstaclemap_t;

/* create and destroy */
sensorstate_t* sensorstate_create_floormode();
sensorstate_t* sensorstate_create_rightwallmode();
sensorstate_t* sensorstate_create_ceilingmode();
sensorstate_t* sensorstate_create_leftwallmode();
sensorstate_t* sensorstate_destroy(sensorstate_t *sensorstate);

/* public methods */
const struct obstacle_t* sensorstate_check(const sensorstate_t *sensorstate, v2d_t actor_position, const struct obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2); /* x2 > x1 && y2 > y1 */
void sensorstate_render(const sensorstate_t *sensorstate, v2d_t actor_position, v2d_t camera_position, int x1, int y1, int x2, int y2, color_t color);
void sensorstate_worldpos(const sensorstate_t *sensorstate, v2d_t actor_position, int *x1, int *y1, int *x2, int *y2);

#endif
