/*
 * Open Surge Engine
 * renderqueue.h - render queue
 * Copyright (C) 2010, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _RENDERQUEUE_H
#define _RENDERQUEUE_H

#include "../core/v2d.h"

/*
 * a render queue is used to render entities in the correct order (according to their z-indexes).
 * Just add the entities to the queue and it will do all the hard work for you
 *
 * ps: if two entities have the same z-index, the entity that was enqueued first will be
 * rendered first
 */

/* forward declarations */
struct brick_t;
struct item_t;
struct enemy_t;
struct player_t;
struct bgtheme_t;
struct surgescript_object_t;

/* starts a new rendering process */
void renderqueue_begin(v2d_t camera_position);

/* finishes an existing rendering process, rendering everything */
void renderqueue_end();

/* enqueues entities */
void renderqueue_enqueue_brick(struct brick_t *brick);
void renderqueue_enqueue_item(struct item_t *item);
void renderqueue_enqueue_object(struct enemy_t *object);
void renderqueue_enqueue_player(struct player_t *player);
void renderqueue_enqueue_particles(); /* enqueues the whole particle system defined in particle.h */
void renderqueue_enqueue_ssobject(struct surgescript_object_t* object);
void renderqueue_enqueue_ssobject_debug(struct surgescript_object_t* object);
void renderqueue_enqueue_background(struct bgtheme_t* background);
void renderqueue_enqueue_foreground(struct bgtheme_t* foreground);
void renderqueue_enqueue_water();

#endif
