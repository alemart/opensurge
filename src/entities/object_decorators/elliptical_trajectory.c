/*
 * Open Surge Engine
 * elliptical_trajectory.h - This decorator makes the object follow an elliptical trajectory
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#include <math.h>
#include "elliptical_trajectory.h"
#include "../../core/util.h"
#include "../../core/timer.h"

/* objectdecorator_ellipticaltrajectory_t class */
typedef struct objectdecorator_ellipticaltrajectory_t objectdecorator_ellipticaltrajectory_t;
struct objectdecorator_ellipticaltrajectory_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *amplitude_x, *amplitude_y; /* distance from the center of the ellipsis (actor's spawn point) */
    expression_t *angularspeed_x, *angularspeed_y; /* speed, in radians per second */
    expression_t *initialphase_x, *initialphase_y; /* initial phase in degrees */
    float elapsed_time;
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_ellipticaltrajectory_new(objectmachine_t *decorated_machine, expression_t *amplitude_x, expression_t *amplitude_y, expression_t *angularspeed_x, expression_t *angularspeed_y, expression_t *initialphase_x, expression_t *initialphase_y)
{
    objectdecorator_ellipticaltrajectory_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->amplitude_x = amplitude_x;
    me->amplitude_y = amplitude_y;
    me->angularspeed_x = angularspeed_x;
    me->angularspeed_y = angularspeed_y;
    me->initialphase_x = initialphase_x;
    me->initialphase_y = initialphase_y;
    me->elapsed_time = 0.0f;

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
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_ellipticaltrajectory_t *me = (objectdecorator_ellipticaltrajectory_t*)obj;

    expression_destroy(me->amplitude_x);
    expression_destroy(me->amplitude_y);
    expression_destroy(me->angularspeed_x);
    expression_destroy(me->angularspeed_y);
    expression_destroy(me->initialphase_x);
    expression_destroy(me->initialphase_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_ellipticaltrajectory_t *me = (objectdecorator_ellipticaltrajectory_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    actor_t *act = object->actor;
    float dt = timer_get_delta();
    brick_t *up = NULL, *upright = NULL, *right = NULL, *downright = NULL;
    brick_t *down = NULL, *downleft = NULL, *left = NULL, *upleft = NULL;
    float elapsed_time = (me->elapsed_time += dt);
    v2d_t old_position = act->position;

    /* elliptical trajectory */
    /*
        let C: R -> R^2 be such that:
            C(t) = (
                Ax * cos( Ix + Sx*t ) + Px,
                Ay * sin( Iy + Sy*t ) + Py
            )

        where:
            t  = elapsed_time       (in seconds)
            Ax = amplitude_x        (in pixels)
            Ay = amplitude_y        (in pixels)
            Sx = angularspeed_x     (in radians per second)
            Sy = angularspeed_y     (in radians per second)
            Ix = initialphase_x     (in radians)
            Iy = initialphase_y     (in radians)
            Px = act->spawn_point.x (in pixels)
            Py = act->spawn_point.y (in pixels)

        then:
            C'(t) = dC(t) / dt = (
                -Ax * Sx * sin( Ix + Sx*t ),
                 Ay * Sy * cos( Iy + Sy*t )
            )
    */

    float amplitude_x = expression_evaluate(me->amplitude_x);
    float amplitude_y = expression_evaluate(me->amplitude_y);
    float angularspeed_x = expression_evaluate(me->angularspeed_x) * (2.0f * PI);
    float angularspeed_y = expression_evaluate(me->angularspeed_y) * (2.0f * PI);
    float initialphase_x = (expression_evaluate(me->initialphase_x) * PI) / 180.0f;
    float initialphase_y = (expression_evaluate(me->initialphase_y) * PI) / 180.0f;

    act->position.x += (-amplitude_x * angularspeed_x * sin( initialphase_x + angularspeed_x * elapsed_time)) * dt;
    act->position.y += ( amplitude_y * angularspeed_y * cos( initialphase_y + angularspeed_y * elapsed_time)) * dt;

    /* sensors */
    actor_sensors(act, brick_list, &up, &upright, &right, &downright, &down, &downleft, &left, &upleft);

    /* I don't want to get stuck into walls */
    if(right != NULL) {
        if(act->position.x > old_position.x)
            act->position.x = act->hot_spot.x - image_width(actor_image(act)) + right->x;
    }

    if(left != NULL) {
        if(act->position.x < old_position.x)
            act->position.x = act->hot_spot.x + left->x + image_width(brick_image(left));
    }

    if(down != NULL) {
        if(act->position.y > old_position.y)
            act->position.y = act->hot_spot.y - image_height(actor_image(act)) + down->y;
    }

    if(up != NULL) {
        if(act->position.y < old_position.y)
            act->position.y = act->hot_spot.y + up->y + image_height(brick_image(up));
    }

    /* decorator pattern */
    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

