/*
 * Open Surge Engine
 * bullet_trajectory.c - This decorator makes the object follow a bullet trajectory
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

#include "bullet_trajectory.h"
#include "../../core/util.h"
#include "../../core/timer.h"

/* objectdecorator_bullettrajectory_t class */
typedef struct objectdecorator_bullettrajectory_t objectdecorator_bullettrajectory_t;
struct objectdecorator_bullettrajectory_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *speed_x, *speed_y; /* bullet speed */
};

/* private methods */
static void bullettrajectory_init(objectmachine_t *obj);
static void bullettrajectory_release(objectmachine_t *obj);
static void bullettrajectory_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void bullettrajectory_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_bullettrajectory_new(objectmachine_t *decorated_machine, expression_t *speed_x, expression_t *speed_y)
{
    objectdecorator_bullettrajectory_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = bullettrajectory_init;
    obj->release = bullettrajectory_release;
    obj->update = bullettrajectory_update;
    obj->render = bullettrajectory_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->speed_x = speed_x;
    me->speed_y = speed_y;

    return obj;
}





/* private methods */
void bullettrajectory_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void bullettrajectory_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_bullettrajectory_t *me = (objectdecorator_bullettrajectory_t*)obj;

    expression_destroy(me->speed_x);
    expression_destroy(me->speed_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void bullettrajectory_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_bullettrajectory_t *me = (objectdecorator_bullettrajectory_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    float dt = timer_get_delta();
    v2d_t ds, speed;

    speed = v2d_new(expression_evaluate(me->speed_x), expression_evaluate(me->speed_y));
    ds = v2d_multiply(speed, dt);
    object->actor->position = v2d_add(object->actor->position, ds);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void bullettrajectory_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

