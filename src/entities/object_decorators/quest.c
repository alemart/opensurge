/*
 * Open Surge Engine
 * quest.c - Quest management
 * Copyright (C) 2013  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "quest.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../scenes/level.h"

/* objectdecorator_quest_t class */
typedef struct objectdecorator_quest_t objectdecorator_quest_t;
struct objectdecorator_quest_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    void (*update)(objectdecorator_quest_t*); /* update function */
};

typedef struct objectdecorator_pushquest_t objectdecorator_pushquest_t;
struct objectdecorator_pushquest_t {
    objectdecorator_quest_t base;
    char filepath[1024];
};

typedef struct objectdecorator_popquest_t objectdecorator_popquest_t;
struct objectdecorator_popquest_t {
    objectdecorator_quest_t base;
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);


static objectmachine_t* setup_decorator(objectdecorator_quest_t *me, objectmachine_t *decorated_machine, void (*update_fun)(objectdecorator_quest_t*));


static void pushquest(objectdecorator_quest_t *q);
static void popquest(objectdecorator_quest_t *q);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_pushquest_new(objectmachine_t *decorated_machine, const char *path_to_qst_file)
{
    objectdecorator_pushquest_t *me = mallocx(sizeof *me);
    str_cpy(me->filepath,  path_to_qst_file, sizeof(me->filepath));
    return setup_decorator((objectdecorator_quest_t*)me, decorated_machine, pushquest);
}

objectmachine_t* objectdecorator_popquest_new(objectmachine_t *decorated_machine)
{
    objectdecorator_popquest_t *me = mallocx(sizeof *me);
    return setup_decorator((objectdecorator_quest_t*)me, decorated_machine, popquest);
}



/* private methods */
objectmachine_t* setup_decorator(objectdecorator_quest_t *me, objectmachine_t *decorated_machine, void (*update_fun)(objectdecorator_quest_t*))
{
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;

    me->update = update_fun;

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

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_quest_t *me = (objectdecorator_quest_t*)obj;
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    me->update(me);

    /*decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);*/
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    /*objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;*/

    /* empty */

    /*decorated_machine->render(decorated_machine, camera_position);*/
}





/* functions */
void pushquest(objectdecorator_quest_t *q)
{
    level_push_quest(((objectdecorator_pushquest_t*)q)->filepath);
}

void popquest(objectdecorator_quest_t *q)
{
    level_pop_quest();
}
