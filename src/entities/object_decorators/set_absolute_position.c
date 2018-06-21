/*
 * Open Surge Engine
 * set_absolute_position.c - Defines a new absolute position to this object in the screen
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

#include "set_absolute_position.h"
#include "../../core/util.h"
#include "../../entities/player.h"

/* objectdecorator_setabsoluteposition_t class */
typedef struct objectdecorator_setabsoluteposition_t objectdecorator_setabsoluteposition_t;
struct objectdecorator_setabsoluteposition_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *pos_x, *pos_y;
};

/* private methods */
static void setabsoluteposition_init(objectmachine_t *obj);
static void setabsoluteposition_release(objectmachine_t *obj);
static void setabsoluteposition_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setabsoluteposition_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setabsoluteposition_new(objectmachine_t *decorated_machine, expression_t *xpos, expression_t *ypos)
{
    objectdecorator_setabsoluteposition_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setabsoluteposition_init;
    obj->release = setabsoluteposition_release;
    obj->update = setabsoluteposition_update;
    obj->render = setabsoluteposition_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->pos_x = xpos;
    me->pos_y = ypos;

    return obj;
}





/* private methods */
void setabsoluteposition_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setabsoluteposition_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setabsoluteposition_t *me = (objectdecorator_setabsoluteposition_t*)obj;

    expression_destroy(me->pos_x);
    expression_destroy(me->pos_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setabsoluteposition_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setabsoluteposition_t *me = (objectdecorator_setabsoluteposition_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    v2d_t pos = v2d_new(expression_evaluate(me->pos_x), expression_evaluate(me->pos_y));

    object->actor->position = pos;

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setabsoluteposition_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}
