/*
 * Open Surge Engine
 * observe_player.c - Observes a player
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#include "observe_player.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../entities/player.h"

typedef struct objectdecorator_observeplayer_t objectdecorator_observeplayer_t;
typedef struct observeplayerstrategy_t observeplayerstrategy_t;

/* objectdecorator_observeplayer_t class */
struct objectdecorator_observeplayer_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    observeplayerstrategy_t *strategy;
};

/* observeplayerstrategy_t class */
struct observeplayerstrategy_t {
    char *player_name; /* player name */
    object_t *object; /* pointer to the object instance */
    void (*run)(observeplayerstrategy_t*, player_t**, int);
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* make_decorator(objectmachine_t *decorated_machine, observeplayerstrategy_t *strategy);
static observeplayerstrategy_t* make_strategy(const char *player_name, object_t *object, void (*run_func)(observeplayerstrategy_t*,player_t**,int));

static void observe_player(observeplayerstrategy_t *strategy, player_t **team, int team_size);
static void observe_current_player(observeplayerstrategy_t *strategy, player_t **team, int team_size);
static void observe_active_player(observeplayerstrategy_t *strategy, player_t **team, int team_size);
static void observe_all_players(observeplayerstrategy_t *strategy, player_t **team, int team_size);



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_observeplayer_new(objectmachine_t *decorated_machine, const char *player_name)
{
    object_t *object = decorated_machine->get_object_instance(decorated_machine);
    return make_decorator(decorated_machine, make_strategy(player_name, object, observe_player));
}

objectmachine_t* objectdecorator_observecurrentplayer_new(objectmachine_t *decorated_machine)
{
    object_t *object = decorated_machine->get_object_instance(decorated_machine);
    return make_decorator(decorated_machine, make_strategy("foo", object, observe_current_player));
}

objectmachine_t* objectdecorator_observeactiveplayer_new(objectmachine_t *decorated_machine)
{
    object_t *object = decorated_machine->get_object_instance(decorated_machine);
    return make_decorator(decorated_machine, make_strategy("bar", object, observe_active_player));
}

objectmachine_t* objectdecorator_observeallplayers_new(objectmachine_t *decorated_machine)
{
    object_t *object = decorated_machine->get_object_instance(decorated_machine);
    return make_decorator(decorated_machine, make_strategy("boo", object, observe_all_players));
}


/* private methods */
objectmachine_t* make_decorator(objectmachine_t *decorated_machine, observeplayerstrategy_t *strategy)
{
    objectdecorator_observeplayer_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->strategy = strategy;

    return obj;
}

observeplayerstrategy_t* make_strategy(const char *player_name, object_t *object, void (*run_func)(observeplayerstrategy_t*,player_t**,int))
{
    observeplayerstrategy_t *x = mallocx(sizeof *x);

    x->player_name = str_dup(player_name);
    x->object = object;
    x->run = run_func;

    return x;
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
    objectdecorator_observeplayer_t *me = (objectdecorator_observeplayer_t*)obj;

    free(me->strategy->player_name);
    free(me->strategy);

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_observeplayer_t *me = (objectdecorator_observeplayer_t*)obj;

    me->strategy->run(me->strategy, team, team_size);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

void observe_player(observeplayerstrategy_t *strategy, player_t **team, int team_size)
{
    int i;
    player_t *player = NULL;

    for(i=0; i<team_size; i++) {
        if(str_icmp(team[i]->name, strategy->player_name) == 0)
            player = team[i];
    }

    if(player == NULL)
        fatal_error("Can't observe player \"%s\": player does not exist!", strategy->player_name);

    enemy_observe_player(strategy->object, player);
}

void observe_current_player(observeplayerstrategy_t *strategy, player_t **team, int team_size)
{
    enemy_observe_current_player(strategy->object);
}

void observe_active_player(observeplayerstrategy_t *strategy, player_t **team, int team_size)
{
    enemy_observe_active_player(strategy->object);
}

void observe_all_players(observeplayerstrategy_t *strategy, player_t **team, int team_size)
{
    player_t *observed_player = enemy_get_observed_player(strategy->object);
    int i;

    for(i=0; i<team_size; i++) {
        if(team[i] == observed_player) {
            enemy_observe_player(strategy->object, team[(i+1)%team_size]);
            break;
        }
    }
}
