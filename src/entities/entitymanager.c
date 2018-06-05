/*
 * Open Surge Engine
 * entitymanager.c - efficient data structure for retrieving bricks,
 *                   built-in items and custom objects in the level.
 * Copyright (C) 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "entitymanager.h"
#include "brick.h"
#include "item.h"
#include "enemy.h"
#include "actor.h"
#include "../core/spatialhash.h"
#include "../core/util.h"

/* defining the spatial hashes */
SPATIALHASH_GENERATE_CODE(brick_t)
SPATIALHASH_GENERATE_CODE(item_t)
SPATIALHASH_GENERATE_CODE(enemy_t)

/* private stuff */
static spatialhash_brick_t *bricks;
static spatialhash_item_t *items;
static spatialhash_enemy_t *objects;

static brick_list_t *dead_bricks;
static item_list_t *dead_items;
static enemy_list_t *dead_objects;

static int active_rectangle_xpos;
static int active_rectangle_ypos;
static int active_rectangle_width;
static int active_rectangle_height;

static int brick_count;
static int item_count;
static int object_count;

static void add_to_dead_bricks_list(brick_t *brick);
static void add_to_dead_items_list(item_t *item);
static void add_to_dead_objects_list(enemy_t *object);

static int retrieve_bricks(brick_t *brick, void *ref_to_brick_list);
static int retrieve_items(item_t *item, void *ref_to_item_list);
static int retrieve_objects(enemy_t *object, void *ref_to_object_list);

static int get_brick_xpos(const brick_t *brick);
static int get_brick_ypos(const brick_t *brick);
static int get_brick_width(const brick_t *brick);
static int get_brick_height(const brick_t *brick);
static int get_item_xpos(const item_t *item);
static int get_item_ypos(const item_t *item);
static int get_item_width(const item_t *item);
static int get_item_height(const item_t *item);
static int get_object_xpos(const enemy_t *object);
static int get_object_ypos(const enemy_t *object);
static int get_object_width(const enemy_t *object);
static int get_object_height(const enemy_t *object);

/* public methods */
void entitymanager_init()
{
    logfile_message("Initializing the Entity Manager...");

    dead_bricks = NULL;
    dead_items = NULL;
    dead_objects = NULL;

    active_rectangle_xpos = 0;
    active_rectangle_ypos = 0;
    active_rectangle_width = 0;
    active_rectangle_height = 0;

    brick_count = 0;
    item_count = 0;
    object_count = 0;

    bricks = spatialhash_brick_t_create(brick_destroy, get_brick_xpos, get_brick_ypos, get_brick_width, get_brick_height);
    items = spatialhash_item_t_create(item_destroy, get_item_xpos, get_item_ypos, get_item_width, get_item_height);
    objects = spatialhash_enemy_t_create(enemy_destroy, get_object_xpos, get_object_ypos, get_object_width, get_object_height);
}

void entitymanager_release()
{
    logfile_message("Releasing the Entity Manager...");

    logfile_message("releasing bricks...");
    bricks = spatialhash_brick_t_destroy(bricks);

    logfile_message("releasing built-in items...");
    items = spatialhash_item_t_destroy(items);

    logfile_message("releasing custom objects...");
    objects = spatialhash_enemy_t_destroy(objects);
}

void entitymanager_store_brick(brick_t *brick)
{
    (brick->brick_ref->behavior == BRB_CIRCULAR ? spatialhash_brick_t_add_persistent : spatialhash_brick_t_add)(bricks, brick);
    brick_count++;
}

void entitymanager_store_item(item_t *item)
{
    (item->always_active ? spatialhash_item_t_add_persistent : spatialhash_item_t_add)(items, item);
    item_count++;
}

void entitymanager_store_object(enemy_t *object)
{
    (object->always_active ? spatialhash_enemy_t_add_persistent : spatialhash_enemy_t_add)(objects, object);
    object_count++;
}

void entitymanager_set_active_region(int rectangle_xpos, int rectangle_ypos, int rectangle_width, int rectangle_height)
{
    active_rectangle_xpos = rectangle_xpos;
    active_rectangle_ypos = rectangle_ypos;
    active_rectangle_width = rectangle_width;
    active_rectangle_height = rectangle_height;
}

brick_list_t* entitymanager_retrieve_active_bricks()
{
    brick_list_t *list = NULL;
    spatialhash_brick_t_foreach(bricks, active_rectangle_xpos, active_rectangle_ypos, active_rectangle_width, active_rectangle_height, (void*)(&list), retrieve_bricks);
    return list;
}

item_list_t* entitymanager_retrieve_active_items()
{
    item_list_t *list = NULL;
    spatialhash_item_t_foreach(items, active_rectangle_xpos, active_rectangle_ypos, active_rectangle_width, active_rectangle_height, (void*)(&list), retrieve_items);
    return list;
}

enemy_list_t* entitymanager_retrieve_active_objects()
{
    enemy_list_t *list = NULL;
    spatialhash_enemy_t_foreach(objects, active_rectangle_xpos, active_rectangle_ypos, active_rectangle_width, active_rectangle_height, (void*)(&list), retrieve_objects);
    return list;
}

brick_list_t* entitymanager_retrieve_all_bricks()
{
    brick_list_t *list = NULL;
    spatialhash_brick_t_forall(bricks, (void*)(&list), retrieve_bricks);
    return list;
}

item_list_t* entitymanager_retrieve_all_items()
{
    item_list_t *list = NULL;
    spatialhash_item_t_forall(items, (void*)(&list), retrieve_items);
    return list;
}

enemy_list_t* entitymanager_retrieve_all_objects()
{
    enemy_list_t *list = NULL;
    spatialhash_enemy_t_forall(objects, (void*)(&list), retrieve_objects);
    return list;
}

brick_list_t* entitymanager_release_retrieved_brick_list(brick_list_t *list)
{
    brick_list_t *next;

    while(list != NULL) {
        next = list->next;
        free(list);
        list = next;
    }

    return NULL;
}

item_list_t* entitymanager_release_retrieved_item_list(item_list_t *list)
{
    item_list_t *next;

    while(list != NULL) {
        next = list->next;
        free(list);
        list = next;
    }

    return NULL;
}

enemy_list_t* entitymanager_release_retrieved_object_list(enemy_list_t *list)
{
    enemy_list_t *next;

    while(list != NULL) {
        next = list->next;
        free(list);
        list = next;
    }

    return NULL;
}

int entitymanager_get_number_of_bricks()
{
    return brick_count;
}

int entitymanager_get_number_of_items()
{
    return item_count;
}

int entitymanager_get_number_of_objects()
{
    return object_count;
}

void entitymanager_remove_dead_bricks()
{
    brick_list_t *it, *next;

    for(it = dead_bricks; it != NULL; it = next) {
        next = it->next;
        spatialhash_brick_t_remove(bricks, it->data);
        brick_count--;
        free(it);
    }

    dead_bricks = NULL;
}

void entitymanager_remove_dead_items()
{
    item_list_t *it, *next;

    for(it = dead_items; it != NULL; it = next) {
        next = it->next;
        spatialhash_item_t_remove(items, it->data);
        item_count--;
        free(it);
    }

    dead_items = NULL;
}

void entitymanager_remove_dead_objects()
{
    enemy_list_t *it, *next;

    for(it = dead_objects; it != NULL; it = next) {
        next = it->next;
        spatialhash_enemy_t_remove(objects, it->data);
        object_count--;
        free(it);
    }

    dead_objects = NULL;
}

/* private methods */
int get_brick_xpos(const brick_t *brick)
{
    return brick->x;
}

int get_brick_ypos(const brick_t *brick)
{
    return brick->y;
}

int get_brick_width(const brick_t *brick)
{
    return image_width(brick->brick_ref->image);
}

int get_brick_height(const brick_t *brick)
{
    return image_height(brick->brick_ref->image);
}

int get_item_xpos(const item_t *item)
{
    return (int)item->actor->position.x;
}

int get_item_ypos(const item_t *item)
{
    return (int)item->actor->position.y;
}

int get_item_width(const item_t *item)
{
    return image_width(actor_image(item->actor));
}

int get_item_height(const item_t *item)
{
    return image_height(actor_image(item->actor));
}

int get_object_xpos(const enemy_t *object)
{
    return (int)object->actor->position.x;
}

int get_object_ypos(const enemy_t *object)
{
    return (int)object->actor->position.y;
}

int get_object_width(const enemy_t *object)
{
    return image_width(actor_image(object->actor));
}

int get_object_height(const enemy_t *object)
{
    return image_height(actor_image(object->actor));
}

int retrieve_bricks(brick_t *brick, void *ref_to_brick_list)
{
    brick_list_t **list = (brick_list_t**)ref_to_brick_list;

    if(brick->state != BRS_DEAD) {
        brick_list_t *p = mallocx(sizeof *p);
        p->data = brick;
        p->next = *list;
        *list = p;
    }
    else
        add_to_dead_bricks_list(brick);

    return 0;
}

int retrieve_items(item_t *item, void *ref_to_item_list)
{
    item_list_t **list = (item_list_t**)ref_to_item_list;

    if(item->state != IS_DEAD) {
        item_list_t *p = mallocx(sizeof *p);
        p->data = item;
        p->next = *list;
        *list = p;
    }
    else
        add_to_dead_items_list(item);

    return 0;
}

int retrieve_objects(enemy_t *object, void *ref_to_object_list)
{
    enemy_list_t **list = (enemy_list_t**)ref_to_object_list;

    if(object->state != ES_DEAD) {
        enemy_list_t *p = mallocx(sizeof *p);
        p->data = object;
        p->next = *list;
        *list = p;
    }
    else
        add_to_dead_objects_list(object);

    return 0;
}

void add_to_dead_bricks_list(brick_t *brick)
{
    brick_list_t *it, *prev, *node;

    for(prev = NULL, it = dead_bricks; it != NULL; prev = it, it = it->next) {
        if(it->data == brick)
            return;
    }

    node = mallocx(sizeof *node);
    node->data = brick;
    node->next = NULL;
    if(prev == NULL)
        dead_bricks = node;
    else
        prev->next = node;
}

void add_to_dead_items_list(item_t *item)
{
    item_list_t *it, *prev, *node;

    for(prev = NULL, it = dead_items; it != NULL; prev = it, it = it->next) {
        if(it->data == item)
            return;
    }

    node = mallocx(sizeof *node);
    node->data = item;
    node->next = NULL;
    if(prev == NULL)
        dead_items = node;
    else
        prev->next = node;
}

void add_to_dead_objects_list(enemy_t *object)
{
    enemy_list_t *it, *prev, *node;

    for(prev = NULL, it = dead_objects; it != NULL; prev = it, it = it->next) {
        if(it->data == object)
            return;
    }

    node = mallocx(sizeof *node);
    node->data = object;
    node->next = NULL;
    if(prev == NULL)
        dead_objects = node;
    else
        prev->next = node;
}
