/*
 * Open Surge Engine
 * particle.h - particle effect
 * Copyright (C) 2008-2010  Alexandre Martins <alemartf@gmail.com>
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

#include "../core/v2d.h"

struct brick_list_t;
struct image_t;

/* initializes the particle system */
void particle_init();

/* releases the particle system */
void particle_release();

/* adds a new particle to the system. Warning: image will be free'd internally. */
void particle_add(struct image_t *image, v2d_t position, v2d_t speed, int destroy_on_brick);

/* updates all the particles */
void particle_update_all(const struct brick_list_t* brick_list);

/* renders the particles */
void particle_render_all(v2d_t camera_position);

#endif
