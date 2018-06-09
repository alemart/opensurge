/*
 * Open Surge Engine
 * attach_to_player.c - Attaches this object to the observed player
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

#include "attach_to_player.h"
#include "../../core/util.h"
#include "../../entities/player.h"

/* objectdecorator_attachtoplayer_t class */
typedef struct objectdecorator_attachtoplayer_t objectdecorator_attachtoplayer_t;
struct objectdecorator_attachtoplayer_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *offset_x, *offset_y;
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_attachtoplayer_new(objectmachine_t *decorated_machine, expression_t *offset_x, expression_t *offset_y)
{
    objectdecorator_attachtoplayer_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->offset_x = offset_x;
    me->offset_y = offset_y;

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
    objectdecorator_attachtoplayer_t *me = (objectdecorator_attachtoplayer_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    expression_destroy(me->offset_x);
    expression_destroy(me->offset_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_attachtoplayer_t *me = (objectdecorator_attachtoplayer_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);
    float player_direction = (player->actor->mirror & IF_HFLIP) ? -1.0f : 1.0f;
    v2d_t offset = v2d_new(player_direction * expression_evaluate(me->offset_x), expression_evaluate(me->offset_y));

    object->attached_to_player = TRUE;
    object->attached_to_player_offset = v2d_rotate(offset, -old_school_angle(player->actor->angle));
    object->actor->position = v2d_add(player->actor->position, object->attached_to_player_offset);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

