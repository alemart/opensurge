/*
 * Open Surge Engine
 * player_movement.c - Enables/disables the movement of the player
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

#include "player_movement.h"
#include "../../core/util.h"
#include "../../scenes/level.h"

/* objectdecorator_playermovement_t class */
typedef struct objectdecorator_playermovement_t objectdecorator_playermovement_t;
struct objectdecorator_playermovement_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    int enable; /* should the movement be enabled? */
};

/* private methods */
static void playermovement_init(objectmachine_t *obj);
static void playermovement_release(objectmachine_t *obj);
static void playermovement_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void playermovement_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t *make_decorator(objectmachine_t *decorated_machine, int enable);




/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_enableplayermovement_new(objectmachine_t *decorated_machine)
{
    return make_decorator(decorated_machine, TRUE);
}

objectmachine_t* objectdecorator_disableplayermovement_new(objectmachine_t *decorated_machine)
{
    return make_decorator(decorated_machine, FALSE);
}





/* private methods */

objectmachine_t* make_decorator(objectmachine_t *decorated_machine, int enable)
{
    objectdecorator_playermovement_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = playermovement_init;
    obj->release = playermovement_release;
    obj->update = playermovement_update;
    obj->render = playermovement_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->enable = enable;

    return obj;
}


void playermovement_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void playermovement_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void playermovement_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_playermovement_t *me = (objectdecorator_playermovement_t*)obj;
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    player->disable_movement = !me->enable;

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void playermovement_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

