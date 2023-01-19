/*
 * Open Surge Engine
 * obstaclemap.h - physics system: obstacle map
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

#ifndef _OBSTACLEMAP_H
#define _OBSTACLEMAP_H

#include <stdbool.h>
#include "../core/v2d.h"

/*
 * an obstacle map is a set of obstacles
 */
typedef struct obstaclemap_t obstaclemap_t;

/* forward declarations */
struct obstacle_t;
enum movmode_t;

/* create and destroy */
obstaclemap_t* obstaclemap_create();
obstaclemap_t* obstaclemap_destroy(obstaclemap_t *obstaclemap);

/* public methods */
void obstaclemap_add_obstacle(obstaclemap_t *obstaclemap, const struct obstacle_t *obstacle); /* adds an obstacle to the map (you have to release it) */
bool obstaclemap_obstacle_exists(const obstaclemap_t* obstaclemap, int x, int y); /* checks if an obstacle exists at (x,y) */
bool obstaclemap_solid_exists(const obstaclemap_t* obstaclemap, int x, int y); /* checks if a solid obstacle exists at (x,y) */
const struct obstacle_t* obstaclemap_get_best_obstacle_at(const obstaclemap_t *obstaclemap, int x1, int y1, int x2, int y2, enum movmode_t mm); /* x2 > x1 && y2 > y1; NULL may be returned */
void obstaclemap_clear(obstaclemap_t* obstaclemap); /* removes all obstacles from the obstacle map */

#endif
