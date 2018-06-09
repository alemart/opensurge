/*
 * Open Surge Engine
 * dialog_box.c - Shows/hides a dialog box
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

#include "dialog_box.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../scenes/level.h"

/* objectdecorator_dialogbox_t class */
typedef struct objectdecorator_dialogbox_t objectdecorator_dialogbox_t;
struct objectdecorator_dialogbox_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    char *title; /* dialog box title */
    char *message; /* dialog box message */
    void (*strategy)(objectdecorator_dialogbox_t*);
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* make_decorator(objectmachine_t *decorated_machine, const char *title, const char *message, void (*strategy)());

static void show_dialog_box(objectdecorator_dialogbox_t *me);
static void hide_dialog_box(objectdecorator_dialogbox_t *me);




/* public methods */
objectmachine_t* objectdecorator_showdialogbox_new(objectmachine_t *decorated_machine, const char *title, const char *message)
{
    return make_decorator(decorated_machine, title, message, show_dialog_box);
}

objectmachine_t* objectdecorator_hidedialogbox_new(objectmachine_t *decorated_machine)
{
    return make_decorator(decorated_machine, "dead", "beef", hide_dialog_box);
}


/* private methods */
objectmachine_t* make_decorator(objectmachine_t *decorated_machine, const char *title, const char *message, void (*strategy)())
{
    objectdecorator_dialogbox_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->title = str_dup(title);
    me->message = str_dup(message);
    me->strategy = strategy;

    return obj;
}

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
    objectdecorator_dialogbox_t *me = (objectdecorator_dialogbox_t*)obj;

    free(me->title);
    free(me->message);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_dialogbox_t *me = (objectdecorator_dialogbox_t*)obj;

    me->strategy(me);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

void show_dialog_box(objectdecorator_dialogbox_t *me)
{
    level_call_dialogbox(me->title, me->message);
}

void hide_dialog_box(objectdecorator_dialogbox_t *me)
{
    level_hide_dialogbox();
}
