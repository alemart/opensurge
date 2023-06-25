/*
 * Open Surge Engine
 * entitymanager.h - efficient data structure for retrieving bricks,
 *                   built-in items and custom objects in the level.
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

#ifndef _ENTITYMANAGER_H
#define _ENTITYMANAGER_H

#include "../../util/rect.h"

/* forward declarations */
struct brick_t;
struct item_t;
struct enemy_t;
struct brick_list_t;
struct item_list_t;
struct enemy_list_t;

/* public methods */
void entitymanager_init(); /* initializes the entity manager */
void entitymanager_release(); /* releases the entity manager and all the stored entities */

/* storing entities */
void entitymanager_store_brick(struct brick_t *brick);
void entitymanager_store_item(struct item_t *item);
void entitymanager_store_object(struct enemy_t *object);

/* retrieving active entities efficiently */
void entitymanager_set_active_region(rect_t roi);
struct brick_list_t* entitymanager_retrieve_active_bricks();
struct brick_list_t* entitymanager_retrieve_active_unmoving_bricks();
struct item_list_t* entitymanager_retrieve_active_items();
struct enemy_list_t* entitymanager_retrieve_active_objects();

/* retrieving all entities */
struct brick_list_t* entitymanager_retrieve_all_bricks();
struct item_list_t* entitymanager_retrieve_all_items();
struct enemy_list_t* entitymanager_retrieve_all_objects();

/* after you retrieve a list of entities, you must release that list */
struct brick_list_t* entitymanager_release_retrieved_brick_list(struct brick_list_t *list);
struct item_list_t* entitymanager_release_retrieved_item_list(struct item_list_t *list);
struct enemy_list_t* entitymanager_release_retrieved_object_list(struct enemy_list_t *list);

/* removing dead entities */
void entitymanager_remove_dead_bricks();
void entitymanager_remove_dead_items();
void entitymanager_remove_dead_objects();

/* other utilities */
int entitymanager_get_number_of_bricks();
int entitymanager_get_number_of_items();
int entitymanager_get_number_of_objects();

#endif
