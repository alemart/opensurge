/*
 * Open Surge Engine
 * jump.c - Jumps
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

#include "jump.h"
#include "../../core/util.h"

/* objectdecorator_jump_t class */
typedef struct objectdecorator_jump_t objectdecorator_jump_t;
struct objectdecorator_jump_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *jump_strength; /* jump strength */
};

/* private methods */
static void jump_init(objectmachine_t *obj);
static void jump_release(objectmachine_t *obj);
static void jump_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void jump_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_jump_new(objectmachine_t *decorated_machine, expression_t *jump_strength)
{
    objectdecorator_jump_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = jump_init;
    obj->release = jump_release;
    obj->update = jump_update;
    obj->render = jump_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->jump_strength = jump_strength;

    return obj;
}




/* private methods */
void jump_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void jump_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_jump_t *me = (objectdecorator_jump_t*)obj;

    expression_destroy(me->jump_strength);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void jump_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_jump_t *me = (objectdecorator_jump_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    actor_t *act = object->actor;
    brick_t *down = NULL;
    float jump_strength = expression_evaluate(me->jump_strength);

    /* sensors */
    actor_sensors(act, brick_list, NULL, NULL, NULL, NULL, &down, NULL, NULL, NULL);

    /* jump! */
    if(down != NULL)
        act->speed.y = -jump_strength;

    /* TODO */
    /*act->jump_strength = jump_strength;
    input_simulate_button_down(act->input, IB_FIRE1);*/

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void jump_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

