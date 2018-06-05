/*
 * Open Surge Engine
 * change_closest_object_state.c - Changes the state of the closest object with a given name
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "change_closest_object_state.h"
#include "../object_vm.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/nanocalcext.h"

/* objectdecorator_changeclosestobjectstate_t class */
typedef struct objectdecorator_changeclosestobjectstate_t objectdecorator_changeclosestobjectstate_t;
struct objectdecorator_changeclosestobjectstate_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *object_name; /* I'll find the closest object called object_name... */
    char *new_state_name; /* ...and change its state to new_state_name */
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);

static object_t *find_closest_object(object_t *me, object_list_t *list, const char* desired_name, float *distance);



/* public methods */
objectmachine_t* objectdecorator_changeclosestobjectstate_new(objectmachine_t *decorated_machine, const char *object_name, const char *new_state_name)
{
    objectdecorator_changeclosestobjectstate_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->object_name = str_dup(object_name);
    me->new_state_name = str_dup(new_state_name);

    return obj;
}


/* private methods */
void init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void release(objectmachine_t *obj)
{
    objectdecorator_changeclosestobjectstate_t *me = (objectdecorator_changeclosestobjectstate_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    free(me->object_name);
    free(me->new_state_name);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_changeclosestobjectstate_t *me = (objectdecorator_changeclosestobjectstate_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    object_t *target = find_closest_object(object, object_list, me->object_name, NULL);

    if(target != NULL) {
        objectvm_set_current_state(target->vm, me->new_state_name);
        enemy_update(target, team, team_size, brick_list, item_list, object_list); /* important to exchange data between objects */
        nanocalcext_set_target_object(object, brick_list, item_list, object_list); /* restore nanocalc's target object */
    }

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

object_t *find_closest_object(object_t *me, object_list_t *list, const char* desired_name, float *distance)
{
    float min_dist = INFINITY_FLT;
    object_list_t *it;
    object_t *ret = NULL;
    v2d_t v;

    for(it=list; it; it=it->next) { /* this list must be small enough */
        if(str_icmp(it->data->name, desired_name) == 0) {
            v = v2d_subtract(it->data->actor->position, me->actor->position);
            if(v2d_magnitude(v) < min_dist) {
                ret = it->data;
                min_dist = v2d_magnitude(v);
            }
        }
    }

    if(distance)
        *distance = min_dist;

    return ret;
}
