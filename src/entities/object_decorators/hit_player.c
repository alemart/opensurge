/*
 * Open Surge Engine
 * hit_player.c - This decorator makes the object hurt the player when touched
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

#include "hit_player.h"
#include "../../core/util.h"
#include "../../entities/player.h"

/* objectdecorator_hitplayer_t class */
typedef struct objectdecorator_hitplayer_t objectdecorator_hitplayer_t;
struct objectdecorator_hitplayer_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    int (*should_hit_the_player)(player_t*); /* strategy pattern */
};

/* private strategies */
static int hit_strategy(player_t *p);
static int burn_strategy(player_t *p);
static int shock_strategy(player_t *p);
static int acid_strategy(player_t *p);

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);
static objectmachine_t *make_decorator(objectmachine_t *decorated_machine, int (*strategy)(player_t*));


/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_hitplayer_new(objectmachine_t *decorated_machine)
{
    return make_decorator(decorated_machine, hit_strategy);
}

objectmachine_t* objectdecorator_burnplayer_new(objectmachine_t *decorated_machine)
{
    return make_decorator(decorated_machine, burn_strategy);
}

objectmachine_t* objectdecorator_shockplayer_new(objectmachine_t *decorated_machine)
{
    return make_decorator(decorated_machine, shock_strategy);
}

objectmachine_t* objectdecorator_acidplayer_new(objectmachine_t *decorated_machine)
{
    return make_decorator(decorated_machine, acid_strategy);
}


/* private methods */

/* builder */
objectmachine_t *make_decorator(objectmachine_t *decorated_machine, int (*strategy)(player_t*))
{
    objectdecorator_hitplayer_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->should_hit_the_player = strategy;

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
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_hitplayer_t *me = (objectdecorator_hitplayer_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(obj->get_object_instance(obj));

    if(!player->invincible && me->should_hit_the_player(player))
        player_hit(player, object->actor);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

/* private strategies */
int hit_strategy(player_t *p)
{
    return TRUE;
}

int burn_strategy(player_t *p)
{
    return p->shield_type != SH_FIRESHIELD && p->shield_type != SH_WATERSHIELD;
}

int shock_strategy(player_t *p)
{
    return p->shield_type != SH_THUNDERSHIELD;
}

int acid_strategy(player_t *p)
{
    return p->shield_type != SH_ACIDSHIELD;
}

