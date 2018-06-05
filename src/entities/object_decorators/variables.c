/*
 * Open Surge Engine
 * variables.c - Basic variables support
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <math.h>
#include "variables.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../scenes/level.h"
#include "../object_vm.h"

/* objectdecorator_variables_t class */
typedef struct objectdecorator_variables_t objectdecorator_variables_t;
struct objectdecorator_variables_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *expr;
    char *new_state_name;
    int (*must_change_state)(float); /* strategy */
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);

/* private strategies */
static int let_strategy(float expr_result) { return FALSE; }
static int if_strategy(float expr_result) { return fabs(expr_result) >= 1e-5; }
static int unless_strategy(float expr_result) { return fabs(expr_result) < 1e-5; }




/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_let_new(objectmachine_t *decorated_machine, expression_t *expr)
{
    objectdecorator_variables_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->expr = expr;
    me->new_state_name = NULL;
    me->must_change_state = let_strategy;

    return obj;
}

objectmachine_t* objectdecorator_if_new(objectmachine_t *decorated_machine, expression_t *expr, const char *new_state_name)
{
    objectdecorator_variables_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->expr = expr;
    me->new_state_name = str_dup(new_state_name);
    me->must_change_state = if_strategy;

    return obj;
}

objectmachine_t* objectdecorator_unless_new(objectmachine_t *decorated_machine, expression_t *expr, const char *new_state_name)
{
    objectdecorator_variables_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->expr = expr;
    me->new_state_name = str_dup(new_state_name);
    me->must_change_state = unless_strategy;

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
    objectdecorator_variables_t *me = (objectdecorator_variables_t*)obj;

    expression_destroy(me->expr);
    if(me->new_state_name != NULL)
        free(me->new_state_name);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_variables_t *me = (objectdecorator_variables_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    float result = expression_evaluate(me->expr);

    if(me->must_change_state(result))
        objectvm_set_current_state(object->vm, me->new_state_name);
    else
        decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

