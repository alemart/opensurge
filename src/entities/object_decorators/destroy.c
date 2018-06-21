/*
 * Open Surge Engine
 * destroy.c - This decorator makes the object destroy itself
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

#include "destroy.h"
#include "../../core/util.h"
#include "../../scenes/level.h"

/* objectdecorator_destroy_t class */
typedef struct objectdecorator_destroy_t objectdecorator_destroy_t;
struct objectdecorator_destroy_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void destroy_init(objectmachine_t *obj);
static void destroy_release(objectmachine_t *obj);
static void destroy_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void destroy_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_destroy_new(objectmachine_t *decorated_machine)
{
    objectdecorator_destroy_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = destroy_init;
    obj->release = destroy_release;
    obj->update = destroy_update;
    obj->render = destroy_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}





/* private methods */
void destroy_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void destroy_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void destroy_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    object_t *object = obj->get_object_instance(obj);
    object->state = ES_DEAD;

    /* suspend the execution */
    /*decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);*/
}

void destroy_render(objectmachine_t *obj, v2d_t camera_position)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    ; /* empty */

    /* suspend the execution */
    /*decorated_machine->render(decorated_machine, camera_position);*/
}

