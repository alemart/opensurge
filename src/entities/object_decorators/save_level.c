/*
 * Open Surge Engine
 * save_level.c - Saves the current level
 * Copyright (C) 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "save_level.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../scenes/level.h"

/* objectdecorator_savelevel_t class */
typedef struct objectdecorator_savelevel_t objectdecorator_savelevel_t;
struct objectdecorator_savelevel_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
};

/* private methods */
static void savelevel_init(objectmachine_t *obj);
static void savelevel_release(objectmachine_t *obj);
static void savelevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void savelevel_render(objectmachine_t *obj, v2d_t camera_position);
static void fix_objects(object_t *obj, void *any_data); /* will fix obj and its children (ie, set ptr->created_from_editor to TRUE) */
static void unfix_objects(object_t *obj, void *any_data); /* will undo whatever fix_objects() did */





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_savelevel_new(objectmachine_t *decorated_machine)
{
    objectdecorator_savelevel_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = savelevel_init;
    obj->release = savelevel_release;
    obj->update = savelevel_update;
    obj->render = savelevel_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    return obj;
}





/* private methods */
void savelevel_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void savelevel_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void savelevel_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    object_t *o = obj->get_object_instance(obj);

    fix_objects(o, NULL);
    level_persist();
    unfix_objects(o, NULL);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void savelevel_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* ---------------------------- */

void fix_objects(object_t *obj, void *any_data)
{
    obj->created_from_editor ^= 0x10;
    enemy_visit_children(obj, any_data, (void(*)(enemy_t*,void*))fix_objects);
}

void unfix_objects(object_t *obj, void *any_data)
{
    obj->created_from_editor ^= 0x10;
    enemy_visit_children(obj, any_data, (void(*)(enemy_t*,void*))fix_objects);
}
