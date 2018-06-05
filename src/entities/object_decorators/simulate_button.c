/*
 * Open Surge Engine
 * simulate_button.h - Simulates that an user is pressing (or not) a button of the input device of the observed player
 * Copyright (C) 2010, 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "simulate_button.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/input.h"
#include "../../entities/player.h"

/* objectdecorator_simulatebutton_t class */
typedef struct objectdecorator_simulatebutton_t objectdecorator_simulatebutton_t;
struct objectdecorator_simulatebutton_t {
    objectdecorator_t base; /* inherits from objectdecorator_t */
    inputbutton_t button; /* button to be simulated */
    void (*callback)(input_t*,inputbutton_t); /* strategy */
};

/* private methods */
static void init(objectmachine_t *obj);
static void release(objectmachine_t *obj);
static void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list);
static void render(objectmachine_t *obj, v2d_t camera_position);

static objectmachine_t* make_decorator(objectmachine_t *decorated_machine, const char *button_name, void (*callback)(input_t*,inputbutton_t));



/* public methods */

/* class constructor */
objectmachine_t* objectdecorator_simulatebuttondown_new(objectmachine_t *decorated_machine, const char *button_name)
{
    return make_decorator(decorated_machine, button_name, input_simulate_button_down);
}

objectmachine_t* objectdecorator_simulatebuttonup_new(objectmachine_t *decorated_machine, const char *button_name)
{
    return make_decorator(decorated_machine, button_name, input_simulate_button_up);
}

objectmachine_t* make_decorator(objectmachine_t *decorated_machine, const char *button_name, void (*callback)(input_t*,inputbutton_t))
{
    objectdecorator_simulatebutton_t *me = mallocx(sizeof *me);
    objectdecorator_t *dec = (objectdecorator_t*)me;
    objectmachine_t *obj = (objectmachine_t*)dec;

    obj->init = init;
    obj->release = release;
    obj->update = update;
    obj->render = render;
    obj->get_object_instance = objectdecorator_get_object_instance; /* inherits from superclass */
    dec->decorated_machine = decorated_machine;
    me->callback = callback;
    me->button = IB_UP;

    if(str_icmp(button_name, "up") == 0)
        me->button = IB_UP;
    else if(str_icmp(button_name, "right") == 0)
        me->button = IB_RIGHT;
    else if(str_icmp(button_name, "down") == 0)
        me->button = IB_DOWN;
    else if(str_icmp(button_name, "left") == 0)
        me->button = IB_LEFT;
    else if(str_icmp(button_name, "fire1") == 0)
        me->button = IB_FIRE1;
    else if(str_icmp(button_name, "fire2") == 0)
        me->button = IB_FIRE2;
    else if(str_icmp(button_name, "fire3") == 0)
        me->button = IB_FIRE3;
    else if(str_icmp(button_name, "fire4") == 0)
        me->button = IB_FIRE4;
    else if(str_icmp(button_name, "fire5") == 0)
        me->button = IB_FIRE5;
    else if(str_icmp(button_name, "fire6") == 0)
        me->button = IB_FIRE6;
    else if(str_icmp(button_name, "fire7") == 0)
        me->button = IB_FIRE7;
    else if(str_icmp(button_name, "fire8") == 0)
        me->button = IB_FIRE8;
    else
        fatal_error("Invalid button '%s' in simulate_button", button_name);

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

    ; /* empty */

    decorated_machine->release(decorated_machine);
    free(obj);
}

void update(objectmachine_t *obj, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, object_list_t *object_list)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;
    objectdecorator_simulatebutton_t *me = (objectdecorator_simulatebutton_t*)obj;
    object_t *object = obj->get_object_instance(obj);
    player_t *player = enemy_get_observed_player(object);

    input_restore(player->actor->input); /* so that non-active players will respond to this command */
    me->callback(player->actor->input, me->button);

    decorated_machine->update(decorated_machine, team, team_size, brick_list, item_list, object_list);
}

void render(objectmachine_t *obj, v2d_t camera_position)
{
    objectdecorator_t *dec = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = dec->decorated_machine;

    ; /* empty */

    decorated_machine->render(decorated_machine, camera_position);
}

