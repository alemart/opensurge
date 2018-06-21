/*
 * Open Surge Engine
 * set_player_position.c - Defines a new position to the observed player
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

#include "set_player_position.h"
#include "../../core/util.h"
#include "../../entities/player.h"

/* objectdecorator_setplayerposition_t class */
typedef struct objectdecorator_setplayerposition_t objectdecorator_setplayerposition_t;
struct objectdecorator_setplayerposition_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *offset_x, *offset_y;
};

/* private methods */
static void setplayerposition_init(objectmachine_t *obj);
static void setplayerposition_release(objectmachine_t *obj);
static void setplayerposition_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setplayerposition_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setplayerposition_new(objectmachine_t *decorated_machine, expression_t *xpos, expression_t *ypos)
{
    objectdecorator_setplayerposition_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setplayerposition_init;
    obj->release = setplayerposition_release;
    obj->update = setplayerposition_update;
    obj->render = setplayerposition_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->offset_x = xpos;
    me->offset_y = ypos;

    return obj;
}





/* private methods */
void setplayerposition_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setplayerposition_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayerposition_t *me = (objectdecorator_setplayerposition_t*)obj;

    expression_destroy(me->offset_x);
    expression_destroy(me->offset_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setplayerposition_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setplayerposition_t *me = (objectdecorator_setplayerposition_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);
    v2d_t offset = v2d_new(expression_evaluate(me->offset_x), expression_evaluate(me->offset_y));

    player->actor->position = v2d_add(object->actor->position, offset);
    player->disable_wall = PLAYER_WALL_NONE;

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setplayerposition_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}
