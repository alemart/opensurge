/*
 * Open Surge Engine
 * brickmanager.h - brick manager
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

#ifndef _BRICKMANAGER_H
#define _BRICKMANAGER_H

#include "../util/rect.h"

/* forward declarations */
typedef struct brickmanager_t brickmanager_t;
struct brickmanager_t;
struct brick_list_t;
struct brick_t;
struct iterator_t;

/* public API */
brickmanager_t* brickmanager_create();
brickmanager_t* brickmanager_destroy(brickmanager_t* manager);
void brickmanager_update(brickmanager_t* manager);

/* storage */
void brickmanager_add_brick(brickmanager_t* manager, struct brick_t* brick);
void brickmanager_remove_all_bricks(brickmanager_t* manager);
int brickmanager_number_of_bricks(const brickmanager_t* manager);

/* retrieval */
void brickmanager_set_roi(brickmanager_t* manager, rect_t roi); /* set region of interest (ROI) */
struct iterator_t* brickmanager_retrieve_active_bricks(const brickmanager_t* manager); /* efficient retrieval based on a ROI */
struct iterator_t* brickmanager_retrieve_active_moving_bricks(const brickmanager_t* manager); /* retrieve moving bricks within the ROI */
struct iterator_t* brickmanager_retrieve_all_bricks(const brickmanager_t* manager);

/* world size */
void brickmanager_world_size(const brickmanager_t* manager, int* world_width, int* world_height);
int brickmanager_world_height_at_interval(const brickmanager_t* manager, int left_xpos, int right_xpos); /* coordinates are inclusive */
void brickmanager_recalculate_world_size(brickmanager_t* manager);

/* legacy brick list for backwards compatibility */
struct brick_list_t* brickmanager_retrieve_all_bricks_as_list(const brickmanager_t* manager);
struct brick_list_t* brickmanager_retrieve_active_bricks_as_list(const brickmanager_t* manager);
struct brick_list_t* brickmanager_release_list(struct brick_list_t* list);

#endif