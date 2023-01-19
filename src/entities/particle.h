/*
 * Open Surge Engine
 * particle.h - particle effect
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

#ifndef _PARTICLE_H
#define _PARTICLE_H

#include <stdbool.h>
#include "../core/v2d.h"

struct brick_list_t;
struct image_t;

/* initializes the particle system */
void particle_init();

/* releases the particle system */
void particle_release();

/* adds a new particle to the system */
void particle_add(const struct image_t* source_image, int source_x, int source_y, int width, int height, v2d_t position, v2d_t speed);

/* updates all the particles */
void particle_update(const struct brick_list_t* brick_list);

/* renders the particles */
void particle_render(v2d_t camera_position);

#endif
