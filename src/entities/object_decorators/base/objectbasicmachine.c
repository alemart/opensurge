/*
 * Open Surge Engine
 * objectbasicmachine.c - Blank implementation of an Object Machine
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "objectbasicmachine.h"
#include "../../../core/util.h"
#include "../../../scenes/level.h"

/* object basic machine class */
typedef struct objectbasicmachine_t objectbasicmachine_t;
struct objectbasicmachine_t {
    objectmachine_t base; /* implements the objectmachine_t interface */
    object_t *object; /* I'm attached to this object */
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);
static object_t* get_object_instance(objectmachine_t *obj);




/* public methods */

/* empty machine - constructor */
objectmachine_t* objectbasicmachine_new(object_t *object)
{
    objectbasicmachine_t *me = mallocx(sizeof *me);
    objectmachine_t *obj = (objectmachine_t*)me;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = get_object_instance;
    me->object = object;

    return obj;
}


/* private methods */
void init(objectmachine_t *obj)
{
    ; /* empty */
}

void release(objectmachine_t *obj)
{
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    ; /* empty */
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    actor_t *act = ((objectbasicmachine_t*)obj)->object->actor; /*obj->get_object_instance(obj)->actor;*/
    v2d_t p = act->position;

    act->position.x = (int)act->position.x;
    act->position.y = (int)act->position.y;
    actor_render(act, camera_position);
    act->position = p;
}

object_t* get_object_instance(objectmachine_t *obj)
{
    objectbasicmachine_t *me = (objectbasicmachine_t*)obj;
    return me->object;
}

