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
#include <string.h>
#include "../core/util.h"
#include "../scenes/level.h"
#include "../entities/actor.h"
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
static surgescript_var_t* fun_getactivity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getattacking(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmidair(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getsecondstodrown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettransform(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* read-write properties */
static surgescript_var_t* fun_getshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getturbo(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setturbo(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getinvincible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setinvincible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getunderwater(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setunderwater(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* methods */
static surgescript_var_t* fun_bounce(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_bounceback(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_hit(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_kill(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_drown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_breathe(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_springify(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_roll(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* internals */
static const surgescript_heapptr_t NAME_ADDR = 0;
static const surgescript_heapptr_t TRANSFORM_ADDR = 1;
static inline player_t* get_player(const surgescript_object_t* object);
static void update_player(surgescript_object_t* object);
static void update_transform(surgescript_object_t* object, v2d_t world_position, float rotation_degrees);
extern actor_t* scripting_actor_ptr(const surgescript_object_t* object);
static const double RAD2DEG = 57.2957795131;


/*
 * scripting_register_player()
 * Register the routines for Player
 */
void scripting_register_player(surgescript_vm_t* vm)
{
    /* tag the object (class) */
    surgescript_tagsystem_t* tag_system = surgescript_vm_tagsystem(vm);
    surgescript_tagsystem_add_tag(tag_system, "Player", "entity");
    surgescript_tagsystem_add_tag(tag_system, "Player", "private");
    surgescript_tagsystem_add_tag(tag_system, "Player", "awake");

    /* read-only properties */
    surgescript_vm_bind(vm, "Player", "get_name", fun_getname, 0);
    surgescript_vm_bind(vm, "Player", "get_activity", fun_getactivity, 0);
    surgescript_vm_bind(vm, "Player", "get_attacking", fun_getattacking, 0);
    surgescript_vm_bind(vm, "Player", "get_midair", fun_getmidair, 0);
    surgescript_vm_bind(vm, "Player", "get_secondsToDrown", fun_getsecondstodrown, 0);
    surgescript_vm_bind(vm, "Player", "get_transform", fun_gettransform, 0);

    /* read-write properties */
    surgescript_vm_bind(vm, "Player", "get_shield", fun_getshield, 0);
    surgescript_vm_bind(vm, "Player", "set_shield", fun_setshield, 1);
    surgescript_vm_bind(vm, "Player", "get_invincible", fun_getinvincible, 0);
    surgescript_vm_bind(vm, "Player", "set_invincible", fun_setinvincible, 1);
    surgescript_vm_bind(vm, "Player", "get_turbo", fun_getturbo, 0);
    surgescript_vm_bind(vm, "Player", "set_turbo", fun_setturbo, 1);
    surgescript_vm_bind(vm, "Player", "get_underwater", fun_getunderwater, 0);
    surgescript_vm_bind(vm, "Player", "set_underwater", fun_setunderwater, 1);
    surgescript_vm_bind(vm, "Player", "get_gsp", fun_getgsp, 0);
    surgescript_vm_bind(vm, "Player", "set_gsp", fun_setgsp, 1);
    surgescript_vm_bind(vm, "Player", "get_xsp", fun_getxsp, 0);
    surgescript_vm_bind(vm, "Player", "set_xsp", fun_setxsp, 1);
    surgescript_vm_bind(vm, "Player", "get_ysp", fun_getysp, 0);
    surgescript_vm_bind(vm, "Player", "set_ysp", fun_setysp, 1);

    /* player-specific methods */
    surgescript_vm_bind(vm, "Player", "bounce", fun_bounce, 1);
    surgescript_vm_bind(vm, "Player", "bounceBack", fun_bounceback, 1);
    surgescript_vm_bind(vm, "Player", "hit", fun_hit, 1);
    surgescript_vm_bind(vm, "Player", "kill", fun_kill, 0);
    surgescript_vm_bind(vm, "Player", "drown", fun_drown, 0);
    surgescript_vm_bind(vm, "Player", "breathe", fun_breathe, 0);
    surgescript_vm_bind(vm, "Player", "springify", fun_springify, 0);
    surgescript_vm_bind(vm, "Player", "roll", fun_roll, 0);
    
    /* general-purpose methods */
    surgescript_vm_bind(vm, "Player", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Player", "__init", fun_init, 1);
    surgescript_vm_bind(vm, "Player", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Player", "destroy", fun_destroy, 0);
}

/* Player routines */

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t transform = surgescript_objectmanager_spawn(manager, me, "Transform2D", NULL);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);

    ssassert(NAME_ADDR == surgescript_heap_malloc(heap));
    ssassert(TRANSFORM_ADDR == surgescript_heap_malloc(heap));

    surgescript_var_set_null(surgescript_heap_at(heap, NAME_ADDR));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, TRANSFORM_ADDR), transform);
    surgescript_object_set_userdata(object, NULL);

    /* Player must be a child of Level */
    if(strcmp(surgescript_object_name(parent), "Level") != 0)
        fatal_error("Scripting Error: object \"%s\" cannot be a child of \"%s\".", surgescript_object_name(object), surgescript_object_name(parent));

    return NULL;
}

/* init: pass a character name, a player ID or null (active player)
   returns true on success */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* name = surgescript_heap_at(heap, NAME_ADDR);

    if(surgescript_var_is_null(param[0])) {
        /* grab active player */
        surgescript_var_set_null(name);
        update_player(object);
        return surgescript_var_set_bool(surgescript_var_create(), true);
    }
    else if(0 == surgescript_var_typecheck(param[0], surgescript_var_type2code("number"))) {
        /* grab player by ID */
        int id = (int)surgescript_var_get_number(param[0]);
        if(level_get_player_by_id(id) != NULL) {
            surgescript_var_set_string(name, level_get_player_by_id(id)->name);
            update_player(object);
            return surgescript_var_set_bool(surgescript_var_create(), true);
        }
    }
    else if(0 == surgescript_var_typecheck(param[0], surgescript_var_type2code("string"))) {
        /* grab player by name */
        surgescript_var_set_string(name, surgescript_var_fast_get_string(param[0]));
        update_player(object);
        return surgescript_var_set_bool(surgescript_var_create(), true);
    }

    /* error */
    surgescript_var_set_string(name, "");
    update_player(object);
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
surgescript_var_t* fun_getactivity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
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

/* is the player attacking? (jumping, etc.) */
surgescript_var_t* fun_getattacking(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && player_is_attacking(player));
}

/* player in midair? */
surgescript_var_t* fun_getmidair(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && player_is_in_the_air(player));
}

/* seconds to drown, if underwater */
surgescript_var_t* fun_getsecondstodrown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(), player != NULL ? player_seconds_remaining_to_drown(player) : INFINITY_FLT);
}

/* Transform component */
surgescript_var_t* fun_gettransform(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, TRANSFORM_ADDR));
}

/* ground speed, in px/s */
surgescript_var_t* fun_getgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(),
        (player != NULL && !player_is_in_the_air(player)) ? player->actor->speed.y : 0.0f
    );
}

/* set ground speed, in px/s */
surgescript_var_t* fun_setgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    if(player != NULL && !player_is_in_the_air(player))
        player->actor->speed.x = surgescript_var_get_number(param[0]);
    return NULL;
}

/* horizontal speed, in px/s */
surgescript_var_t* fun_getxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(),
        (player != NULL && player_is_in_the_air(player)) ? player->actor->speed.x : 0.0f
    );
}

/* set horizontal speed, in px/s */
surgescript_var_t* fun_setxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    if(player != NULL && player_is_in_the_air(player))
        player->actor->speed.x = surgescript_var_get_number(param[0]);
    return NULL;
}

/* vertical speed, in px/s */
surgescript_var_t* fun_getysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(),
        (player != NULL && player_is_in_the_air(player)) ? player->actor->speed.y : 0.0f
    );
}

/* set vertical speed, in px/s */
surgescript_var_t* fun_setysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    if(player != NULL && player_is_in_the_air(player))
        player->actor->speed.y = surgescript_var_get_number(param[0]);
    return NULL;
}

/* returns the name of the current shield, or "none" if no shield is present */
surgescript_var_t* fun_getshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        switch(player_shield_type(player)) {
            case SH_NONE:
                return surgescript_var_set_string(surgescript_var_create(), "none");
            case SH_SHIELD:
                return surgescript_var_set_string(surgescript_var_create(), "shield");
            case SH_FIRESHIELD:
                return surgescript_var_set_string(surgescript_var_create(), "fire");
            case SH_THUNDERSHIELD:
                return surgescript_var_set_string(surgescript_var_create(), "thunder");
            case SH_WATERSHIELD:
                return surgescript_var_set_string(surgescript_var_create(), "water");
            case SH_ACIDSHIELD:
                return surgescript_var_set_string(surgescript_var_create(), "acid");
            case SH_WINDSHIELD:
                return surgescript_var_set_string(surgescript_var_create(), "wind");
        }
    }
    return surgescript_var_set_string(surgescript_var_create(), "none");
}

/* grants the player a shield */
surgescript_var_t* fun_setshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    player_t* player = get_player(object);
    if(player != NULL) {
        char* shield = surgescript_var_get_string(param[0], manager);
        if(strcmp(shield, "none") == 0)
            player_grant_shield(player, SH_NONE);
        else if(strcmp(shield, "shield") == 0)
            player_grant_shield(player, SH_SHIELD);
        else if(strcmp(shield, "fire") == 0)
            player_grant_shield(player, SH_FIRESHIELD);
        else if(strcmp(shield, "thunder") == 0)
            player_grant_shield(player, SH_THUNDERSHIELD);
        else if(strcmp(shield, "water") == 0)
            player_grant_shield(player, SH_WATERSHIELD);
        else if(strcmp(shield, "acid") == 0)
            player_grant_shield(player, SH_ACIDSHIELD);
        else if(strcmp(shield, "wind") == 0)
            player_grant_shield(player, SH_WINDSHIELD);
        ssfree(shield);
    }
    return NULL;
}

/* is turbo mode enabled? */
surgescript_var_t* fun_getturbo(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && player_is_ultrafast(player));
}

/* enable/disable turbo mode */
surgescript_var_t* fun_setturbo(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        bool turbo = surgescript_var_get_bool(param[0]);
        if(turbo && !player_is_ultrafast(player))
            player_set_ultrafast(player, TRUE);
        else if(!turbo && player_is_ultrafast(player))
            player_set_ultrafast(player, FALSE);
    }
    return NULL;
}

/* is the player invincible? */
surgescript_var_t* fun_getinvincible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && player_is_invincible(player));
}

/* give the player invincibility */
surgescript_var_t* fun_setinvincible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        bool invincible = surgescript_var_get_bool(param[0]);
        if(invincible && !player_is_invincible(player))
            player_set_invincible(player, TRUE);
        else if(!invincible && player_is_invincible(player))
            player_set_invincible(player, FALSE);
    }
    return NULL;
}

/* is the player underwater? */
surgescript_var_t* fun_getunderwater(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && player_is_underwater(player));
}

/* makes the player enter/leave the water */
surgescript_var_t* fun_setunderwater(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        bool underwater = surgescript_var_get_bool(param[0]);
        if(underwater && !player_is_underwater(player))
            player_enter_water(player);
        else if(!underwater && player_is_underwater(player))
            player_leave_water(player);
    }
    return NULL;
}

/* rebound: bounce(hazard) - will bounce upwards */
surgescript_var_t* fun_bounce(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    player_t* player = get_player(object);
    if(player != NULL) {
        if(!surgescript_var_is_null(param[0])) {
            surgescript_objecthandle_t hazard_handle = surgescript_var_get_objecthandle(param[0]);
            surgescript_object_t* hazard = surgescript_objectmanager_get(manager, hazard_handle);
            if(strcmp(surgescript_object_name(hazard), "Actor") == 0) {
                actor_t* hazard_actor = scripting_actor_ptr(hazard);
                player_bounce_ex(player, hazard_actor, FALSE);
            }
            else
                fatal_error("Scripting Error: %s.bounce(hazard) requires hazard to be an Actor | null, but hazard is %s.", surgescript_object_name(object), surgescript_object_name(hazard));
        }
        else
            player_bounce(player, -1.0f, FALSE);
    }
    return NULL;
}

/* rebound: bounceBack(hazard) - will bounce upwards if the player is coming from above the hazard, or downwards if coming from below */
surgescript_var_t* fun_bounceback(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    player_t* player = get_player(object);
    if(player != NULL) {
        if(!surgescript_var_is_null(param[0])) {
            surgescript_objecthandle_t hazard_handle = surgescript_var_get_objecthandle(param[0]);
            surgescript_object_t* hazard = surgescript_objectmanager_get(manager, hazard_handle);
            if(strcmp(surgescript_object_name(hazard), "Actor") == 0) {
                actor_t* hazard_actor = scripting_actor_ptr(hazard);
                player_bounce_ex(player, hazard_actor, TRUE);
            }
            else
                fatal_error("Scripting Error: %s.bounceBack(hazard) requires hazard to be an Actor, but hazard is %s.", surgescript_object_name(object), surgescript_object_name(hazard));
        }
        else
            fatal_error("Scripting Error: %s.bounceBack(hazard) requires hazard to be an Actor, but hazard is null.", surgescript_object_name(object));
    }
    return NULL;
}

/* get hit: hit(hazard), where hazard: Actor | null */
surgescript_var_t* fun_hit(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    player_t* player = get_player(object);
    if(player != NULL) {
        if(!surgescript_var_is_null(param[0])) {
            surgescript_objecthandle_t hazard_handle = surgescript_var_get_objecthandle(param[0]);
            surgescript_object_t* hazard = surgescript_objectmanager_get(manager, hazard_handle);
            if(strcmp(surgescript_object_name(hazard), "Actor") == 0) {
                actor_t* hazard_actor = scripting_actor_ptr(hazard);
                player_hit_ex(player, hazard_actor);
            }
            else
                fatal_error("Scripting Error: %s.hit(hazard) requires hazard to be an Actor | null, but hazard is %s.", surgescript_object_name(object), surgescript_object_name(hazard));
        }
        else {
            float direction = physicsactor_is_facing_right(player->pa) ? -1.0f : 1.0f;
            player_hit(player, direction);
        }
    }
    return NULL;
}

/* kill the player */
surgescript_var_t* fun_kill(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL)
        player_kill(player);
    return NULL;
}

/* drown the player */
surgescript_var_t* fun_drown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL)
        player_drown(player);
    return NULL;
}

/* breathe (underwater) */
surgescript_var_t* fun_breathe(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL)
        player_breathe(player);
    return NULL;
}

/* springify */
surgescript_var_t* fun_springify(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL)
        player_spring(player);
    return NULL;
}

/* roll */
surgescript_var_t* fun_roll(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL)
        player_roll(player);
    return NULL;
}


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
void update_player(surgescript_object_t* object)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* name = surgescript_heap_at(heap, NAME_ADDR);
    player_t* player = NULL;

    if(!surgescript_var_is_null(name)) {
        /* we're dealing with a specific player */
        const char* player_name = surgescript_var_fast_get_string(name);
        if(*player_name)
            player = level_get_player_by_name(player_name); /* may be NULL */
    }
    else {
        /* active player */
        player = level_player();
    }

    /* update the transform */
    if(player != NULL)
        update_transform(object, player->actor->position, player->actor->angle * RAD2DEG);
    else
        update_transform(object, v2d_new(0.0f, 0.0f), 0.0f);

    /* update player pointer */
    surgescript_object_set_userdata(object, player);
}

/* update the player transform */
void update_transform(surgescript_object_t* object, v2d_t world_position, float rotation_degrees)
{
    /*surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t transform = surgescript_var_get_objecthandle(surgescript_heap_at(heap, TRANSFORM_ADDR));
    call set_worldX, set_worldY*/
    surgescript_transform_t transform;
    surgescript_transform_reset(&transform);
    surgescript_transform_setposition2d(&transform, world_position.x, world_position.y); /* this assumes local position = world position */
    surgescript_transform_setrotation2d(&transform, rotation_degrees);
    surgescript_transform_setscale2d(&transform, 1.0f, 1.0f);
    surgescript_object_poke_transform(object, &transform);
}