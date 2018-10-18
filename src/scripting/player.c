/*
 * Open Surge Engine
 * player.c - scripting system: player bridge
 * Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <surgescript.h>
#include "../entities/player.h"
#include "../physics/physicsactor.h"

/* private */

/* generic */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* read-only properties */
static surgescript_var_t* fun_getname(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getcurrentstate(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getattacking(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmidair(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getsecondstodrown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* read-write properties */
static surgescript_var_t* fun_getshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getturbo(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setturbo(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getinvincible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setinvincible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getunderwater(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setunderwater(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getworldx(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setworldy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getworldy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setworldy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* methods */
static surgescript_var_t* fun_bounce(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_hit(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_kill(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_drown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_breathe(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_springfy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_roll(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_push(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* internals */
static const surgescript_heapptr_t NAME_ADDR = 0;
static void update_player(surgescript_object_t* object);
static inline player_t* get_player(const surgescript_object_t* object);


/*
 * scripting_register_player()
 * Register the routines for Player
 */
void scripting_register_player(surgescript_vm_t* vm)
{
    /* read-only properties */
    surgescript_vm_bind(vm, "Player", "get_name", fun_getname, 0);

    /* read-write properties */

    /* methods */
}

/* Player routines */

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);

    ssassert(NAME_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, NAME_ADDR));
    surgescript_object_set_userdata(object, NULL);

    return NULL;
}

/* init: pass a character name, a player ID or null (active player)
   returns true on success */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* name = surgescript_heap_at(heap, NAME_ADDR);

    if(surgescript_var_is_null(param[0])) {
        /* grab active player */
        surgescript_var_set_null(name);
        update_player(object);
        return surgescript_var_set_bool(surgescript_var_create(), true);
    }
    else if(0 == surgescript_var_typecheck(surgescript_var_typecode(param[0]), surgescript_var_type2code("number"))) {
        /* grab player by ID */
        int id = (int)surgescript_var_get_number(param[0]);
        if(level_get_player_by_id(id) != NULL) {
            surgescript_var_set_string(name, level_get_player_by_id(id)->name);
            update_player(object);
            return surgescript_var_set_bool(surgescript_var_create(), true);
        }
        else {
            /* error */
            return surgescript_var_set_bool(surgescript_var_create(), false);
        }
    }
    else if(0 == surgescript_var_typecheck(surgescript_var_typecode(param[0]), surgescript_var_type2code("string"))) {
        /* grab player by name */
        surgescript_var_set_string(name, surgescript_var_fast_get_string(param[0]));
        update_player(object);
        return surgescript_var_set_bool(surgescript_var_create(), true);
    }

    /* error */
    return surgescript_var_set_bool(surgescript_var_create(), false);
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    update_player(object);
    return NULL;
}

/* can't destroy the player controller */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* gets the name of the player */
surgescript_var_t* fun_getname(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL)
        return surgescript_var_set_string(surgescript_var_create(), player->name);
    else
        return NULL;
}

/* get a string representing the state of the player */
surgescript_var_t* fun_getcurrentstate(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        switch(physicsactor_get_state(player->pa)) {
            case PAS_STOPPED:
                return surgescript_var_set_string(surgescript_var_create(), "stopped");
            case PAS_WALKING:
                return surgescript_var_set_string(surgescript_var_create(), "walking");
            case PAS_RUNNING:
                return surgescript_var_set_string(surgescript_var_create(), "running");
            case PAS_JUMPING:
                return surgescript_var_set_string(surgescript_var_create(), "jumping");
            case PAS_SPRINGING:
                return surgescript_var_set_string(surgescript_var_create(), "springing");
            case PAS_ROLLING:
                return surgescript_var_set_string(surgescript_var_create(), "rolling");
            case PAS_CHARGING:
                return surgescript_var_set_string(surgescript_var_create(), "charging");
            case PAS_PUSHING:
                return surgescript_var_set_string(surgescript_var_create(), "pushing");
            case PAS_GETTINGHIT:
                return surgescript_var_set_string(surgescript_var_create(), "gettinghit");
            case PAS_DEAD:
                return surgescript_var_set_string(surgescript_var_create(), "dead");
            case PAS_BRAKING:
                return surgescript_var_set_string(surgescript_var_create(), "braking");
            case PAS_LEDGE:
                return surgescript_var_set_string(surgescript_var_create(), "ledge");
            case PAS_DROWNED:
                return surgescript_var_set_string(surgescript_var_create(), "drowned");
            case PAS_BREATHING:
                return surgescript_var_set_string(surgescript_var_create(), "breathing");
            case PAS_DUCKING:
                return surgescript_var_set_string(surgescript_var_create(), "ducking");
            case PAS_LOOKINGUP:
                return surgescript_var_set_string(surgescript_var_create(), "lookingup");
            case PAS_WAITING:
                return surgescript_var_set_string(surgescript_var_create(), "waiting");
            case PAS_WINNING:
                return surgescript_var_set_string(surgescript_var_create(), "winning");
            default:
                return NULL;
        }
    }
    else
        return NULL;
}

surgescript_var_t* fun_getattacking(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL)
        return surgescript_var_set_bool(surgescript_var_create(), player_is_attacking(player));
    else
        return NULL;
}

surgescript_var_t* fun_getmidair(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
surgescript_var_t* fun_getsecondstodrown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
surgescript_var_t* fun_getangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
surgescript_var_t* fun_getgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
surgescript_var_t* fun_getxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
surgescript_var_t* fun_getysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);



/* internals */

/* 
 * gets a pointer to the player_t structure.
 * >> May return NULL <<
 */
player_t* get_player(const surgescript_object_t* object)
{
    return (player_t*)surgescript_object_userdata(object);
}

/* updates the player pointer */
void update_player(surgescript_object_t* object);
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* name = surgescript_heap_at(heap, NAME_ADDR);
    player_t* player = NULL;

    if(!surgescript_var_is_null(name)) {
        /* we're dealing with a specific player */
        const char* player_name = surgescript_var_fast_get_string(name);
        player = level_get_player_by_name(player_name); /* may be NULL */
    }
    else {
        /* active player */
        player = level_player();
    }

    /* update player pointer */
    surgescript_object_set_userdata(object, player);
}