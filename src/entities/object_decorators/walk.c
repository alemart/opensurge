/*
 * Open Surge Engine
 * walk.c - This decorator makes the object walk (move left and right)
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

#include "walk.h"
#include "../../core/util.h"
#include "../../core/timer.h"

/* objectdecorator_walk_t class */
typedef struct objectdecorator_walk_t objectdecorator_walk_t;
struct objectdecorator_walk_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *speed; /* movement speed */
    float direction; /* -1.0f or 1.0f */
};

/* private methods */
static void walk_init(objectmachine_t *obj);
static void walk_release(objectmachine_t *obj);
static void walk_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void walk_render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_walk_new(objectmachine_t *decorated_machine, expression_t *speed)
{
    objectdecorator_walk_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = walk_init;
    obj->release = walk_release;
    obj->update = walk_update;
    obj->render = walk_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->speed = speed;

    return obj;
}




/* private methods */
void walk_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_walk_t *me = (objectdecorator_walk_t*)obj;

    me->direction = (random(2) == 0) ? -1.0f : 1.0f;

    decorated_machine->init(decorated_machine);
}

void walk_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_walk_t *me = (objectdecorator_walk_t*)obj;

    expression_destroy(me->speed);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void walk_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_walk_t *me = (objectdecorator_walk_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    actor_t *act = object->actor;
    float dt = timer_get_delta();
    brick_t *up = NULL, *upright = NULL, *right = NULL, *downright = NULL;
    brick_t *down = NULL, *downleft = NULL, *left = NULL, *upleft = NULL;
    float speed = expression_evaluate(me->speed);

    /* move! */
    act->position.x += (me->direction * speed) * dt;

    /* sensors */
    actor_sensors(act, brick_list, &up, &upright, &right, &downright, &down, &downleft, &left, &upleft);

    /* swap direction when a wall is touched */
    if(right != NULL) {
        if(me->direction > 0.0f) {
            act->position.x = act->hot_spot.x - image_width(actor_image(act)) + right->x;
            me->direction = -1.0f;
        }
    }

    if(left != NULL) {
        if(me->direction < 0.0f) {
            act->position.x = act->hot_spot.x + left->x + image_width(brick_image(left));
            me->direction = 1.0f;
        }
    }

    /* I don't want to fall from the platforms! */
    if(down != NULL) {
        if(downright == NULL && downleft != NULL && me->direction > 0.0f)
            me->direction = -1.0f;
        else if(downleft == NULL && downright != NULL && me->direction < 0.0f)
            me->direction = 1.0f;
    }

    /* decorator pattern */
    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void walk_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

