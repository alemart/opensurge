/*
 * Open Surge Engine
 * showhide.c - showhide-in/out
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

#include "showhide.h"
#include "../object_vm.h"
#include "../../core/util.h"

/* objectdecorator_showhide_t class */
typedef struct objectdecorator_showhide_t objectdecorator_showhide_t;
struct objectdecorator_showhide_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    int show; /* TRUE or FALSE */
};

/* private methods */
static void showhide_init(objectmachine_t *obj);
static void showhide_release(objectmachine_t *obj);
static void showhide_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void showhide_render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* make_decorator(objectmachine_t *decorated_machine, int show);


/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_show_new(objectmachine_t *decorated_machine)
{
    return make_decorator(decorated_machine, TRUE);
}

objectmachine_t* objectdecorator_hide_new(objectmachine_t *decorated_machine)
{
    return make_decorator(decorated_machine, FALSE);
}


/* make decorator */
static objectmachine_t* make_decorator(objectmachine_t *decorated_machine, int show)
{
    objectdecorator_showhide_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = showhide_init;
    obj->release = showhide_release;
    obj->update = showhide_update;
    obj->render = showhide_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->show = show;

    return obj;
}




/* private methods */
void showhide_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void showhide_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void showhide_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    object_t *object = obj->get_object_instance(obj);
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectdecorator_showhide_t *me = (objectdecorator_showhide_t*)dec;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    object->actor->visible = me->show;

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void showhide_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

