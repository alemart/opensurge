/*
 * Open Surge Engine
 * renderqueue.h - render queue
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

#ifndef _RENDERQUEUE_H
#define _RENDERQUEUE_H

#include <stdbool.h>
#include "../core/v2d.h"

/* forward declarations */
struct brick_t;
struct item_t;
struct enemy_t;
struct player_t;
struct bgtheme_t;
struct surgescript_object_t;

/* initialization & deinitialization */
void renderqueue_init(bool want_depth_buffer);
void renderqueue_release();

/* rendering */
void renderqueue_begin(v2d_t camera_position);
void renderqueue_end();

/* enqueues entities */
void renderqueue_enqueue_brick(struct brick_t* brick);
void renderqueue_enqueue_brick_mask(struct brick_t* brick);
void renderqueue_enqueue_brick_debug(struct brick_t* brick);
void renderqueue_enqueue_brick_path(struct brick_t* brick);
void renderqueue_enqueue_item(struct item_t* item);
void renderqueue_enqueue_object(struct enemy_t* object);
void renderqueue_enqueue_player(struct player_t* player);
void renderqueue_enqueue_ssobject(struct surgescript_object_t* object);
void renderqueue_enqueue_ssobject_gizmo(struct surgescript_object_t* object);
void renderqueue_enqueue_ssobject_debug(struct surgescript_object_t* object);
void renderqueue_enqueue_background(struct bgtheme_t* background);
void renderqueue_enqueue_foreground(struct bgtheme_t* foreground);
void renderqueue_enqueue_water();

/* misc */
bool renderqueue_toggle_stats_report();

#endif
