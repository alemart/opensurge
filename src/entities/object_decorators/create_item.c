/*
 * Open Surge Engine
 * create_item.c - This decorator makes the object create an item
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

#include "create_item.h"
#include "../../core/util.h"
#include "../../scenes/level.h"

/* objectdecorator_createitem_t class */
typedef struct objectdecorator_createitem_t objectdecorator_createitem_t;
struct objectdecorator_createitem_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *item_id; /* I'll create an item which id is item_id... */
    expression_t *offset_x, *offset_y; /* ...at this offset */
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);



/* public methods */
objectmachine_t* objectdecorator_createitem_new(objectmachine_t *decorated_machine, expression_t *item_id, expression_t *offset_x, expression_t *offset_y)
{
    objectdecorator_createitem_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->item_id = item_id;
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
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_createitem_t *me = (objectdecorator_createitem_t*)obj;

    expression_destroy(me->item_id);
    expression_destroy(me->offset_x);
    expression_destroy(me->offset_y);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_createitem_t *me = (objectdecorator_createitem_t*)obj;
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *object = obj->get_object_instance(obj);
    int item_id;
    v2d_t offset;

    item_id = (int)expression_evaluate(me->item_id);
    offset.x = expression_evaluate(me->offset_x);
    offset.y = expression_evaluate(me->offset_y);

    level_create_item(item_id, v2d_add(object->actor->position, offset));

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}
