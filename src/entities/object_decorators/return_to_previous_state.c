/*
 * Open Surge Engine
 * return_to_previous_state.c - Returns to the previous state
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

#include "return_to_previous_state.h"
#include "../object_vm.h"
#include "../../core/util.h"
#include "../../scenes/level.h"

/* objectdecorator_returntopreviousstate_t class */
typedef struct objectdecorator_returntopreviousstate_t objectdecorator_returntopreviousstate_t;
struct objectdecorator_returntopreviousstate_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void returntopreviousstate_init(objectmachine_t *obj);
static void returntopreviousstate_release(objectmachine_t *obj);
static void returntopreviousstate_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void returntopreviousstate_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_returntopreviousstate_new(objectmachine_t *decorated_machine)
{
    objectdecorator_returntopreviousstate_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = returntopreviousstate_init;
    obj->release = returntopreviousstate_release;
    obj->update = returntopreviousstate_update;
    obj->render = returntopreviousstate_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}




/* private methods */
void returntopreviousstate_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void returntopreviousstate_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void returntopreviousstate_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    object_t *object = obj->get_object_instance(obj);
    objectvm_return_to_previous_state(object->vm);

    /*decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);*/
}

void returntopreviousstate_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

