/*
 * Open Surge Engine
 * look.c - This decorator makes the object look at a given direction
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

#include "look.h"
#include "../../core/util.h"
#include "../../entities/player.h"

/* objectdecorator_look_t class */
typedef struct objectdecorator_look_t objectdecorator_look_t;
struct objectdecorator_look_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    float old_x;
    void (*look_strategy)(objectdecorator_look_t*);
};

/* private methods */
static objectmachine_t* objectdecorator_look_new(objectmachine_t *decorated_machine, void (*look_strategy)(objectdecorator_look_t*));
static void look_init(objectmachine_t *obj);
static void look_release(objectmachine_t *obj);
static void look_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void look_render(objectmachine_t *obj, v2d_t camera_position);

/* private strategies */
static void look_left(objectdecorator_look_t *me);
static void look_right(objectdecorator_look_t *me);
static void look_at_player(objectdecorator_look_t *me);
static void look_at_walking_direction(objectdecorator_look_t *me);





/* public methods */
objectmachine_t* objectdecorator_lookleft_new(objectmachine_t *decorated_machine)
{
    return objectdecorator_look_new(decorated_machine, look_left);
}

objectmachine_t* objectdecorator_lookright_new(objectmachine_t *decorated_machine)
{
    return objectdecorator_look_new(decorated_machine, look_right);
}

objectmachine_t* objectdecorator_lookatplayer_new(objectmachine_t *decorated_machine)
{
    return objectdecorator_look_new(decorated_machine, look_at_player);
}

objectmachine_t* objectdecorator_lookatwalkingdirection_new(objectmachine_t *decorated_machine)
{
    return objectdecorator_look_new(decorated_machine, look_at_walking_direction);
}






/* private methods */
objectmachine_t* objectdecorator_look_new(objectmachine_t *decorated_machine, void (*look_strategy)(objectdecorator_look_t*))
{
    objectdecorator_look_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = look_init;
    obj->release = look_release;
    obj->update = look_update;
    obj->render = look_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->look_strategy = look_strategy;

    return obj;
}

void look_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_look_t *me = (objectdecorator_look_t*)obj;

    me->old_x = 0.0f;

    decorated_machine->init(decorated_machine);
}

void look_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void look_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_look_t *me = (objectdecorator_look_t*)obj;

    me->look_strategy(me);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void look_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}



/* private strategies */
void look_left(objectdecorator_look_t *me)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);

    object->actor->mirror |= IF_HFLIP;
}

void look_right(objectdecorator_look_t *me)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);

    object->actor->mirror &= ~IF_HFLIP;
}

void look_at_player(objectdecorator_look_t *me)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);

    if(object->actor->position.x < player->actor->position.x)
        object->actor->mirror &= ~IF_HFLIP;
    else
        object->actor->mirror |= IF_HFLIP;
}

void look_at_walking_direction(objectdecorator_look_t *me)
{
    objectmachine_t *obj = (objectmachine_t*)me;
    object_t *object = obj->get_object_instance(obj);

    if(object->actor->position.x > me->old_x)
        object->actor->mirror &= ~IF_HFLIP;
    else
        object->actor->mirror |= IF_HFLIP;

    me->old_x = object->actor->position.x;
}

