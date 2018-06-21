/*
 * Open Surge Engine
 * set_alpha.h - Sets the alpha value of the object (translucency)
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

#include "set_alpha.h"
#include "../../core/v2d.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../actor.h"

/* objectdecorator_setalpha_t class */
typedef struct objectdecorator_setalpha_t objectdecorator_setalpha_t;
struct objectdecorator_setalpha_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    expression_t *alpha; /* 0.0f (invisible) <= alpha <= 1.0f (opaque) */
};

/* private methods */
static void setalpha_init(objectmachine_t *obj);
static void setalpha_release(objectmachine_t *obj);
static void setalpha_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void setalpha_render(objectmachine_t *obj, v2d_t camera_position);





/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_setalpha_new(objectmachine_t *decorated_machine, expression_t *alpha)
{
    objectdecorator_setalpha_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = setalpha_init;
    obj->release = setalpha_release;
    obj->update = setalpha_update;
    obj->render = setalpha_render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->alpha = alpha;

    return obj;
}





/* private methods */
void setalpha_init(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->init(decorated_machine);
}

void setalpha_release(objectmachine_t *obj)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setalpha_t *me = (objectdecorator_setalpha_t*)obj;

    expression_destroy(me->alpha);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void setalpha_update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_setalpha_t *me = (objectdecorator_setalpha_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    float alpha = clip(expression_evaluate(me->alpha), 0.0f, 1.0f);

    object->actor->alpha = alpha;

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void setalpha_render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

