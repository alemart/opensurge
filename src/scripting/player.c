/*
 * Open Surge Engine
 * player.c - scripting system: player bridge
 * Copyright (C) 2018  Alexandre Martins <alemartf@gmail.com>
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
#include <math.h>
#include "scripting.h"
#include "../core/util.h"
#include "../core/stringutil.h"
#include "../core/logfile.h"
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
static surgescript_var_t* fun_ontransformchange(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_onanimationchange(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* read-only properties */
static surgescript_var_t* fun_getname(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getanimation(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getactivity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getattacking(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmidair(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getblinking(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getsecondstodrown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettransform(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getcollider(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getdirection(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getinitiallives(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettopspeed(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getinput(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getdying(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getstopped(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getwalking(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getrunning(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getwaiting(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getjumping(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getspringing(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getrolling(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getcharging(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getpushing(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gethit(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getbraking(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getbalancing(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getdrowning(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getbreathing(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getcrouchingdown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlookingup(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getwinning(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* read-write properties */
static surgescript_var_t* fun_getanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getturbo(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setturbo(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getinvincible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setinvincible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getunderwater(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setunderwater(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfrozen(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setfrozen(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlayer(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setlayer(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getspeed(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setspeed(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getcollectibles(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setcollectibles(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlives(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setlives(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getscore(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setscore(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* methods */
static surgescript_var_t* fun_bounce(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_bounceback(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_hit(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_kill(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_breathe(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_springify(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_roll(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_focus(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_hasfocus(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_hlock(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* PlayerManager */
static surgescript_var_t* fun_manager_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_spawnplayers(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_getactive(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_getbyname(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_getbyid(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_getcount(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_getinitiallives(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_exists(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_get(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_call(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* internals */
#define SHOW_COLLIDERS 0 /* set it to 1 to display the colliders */
static const surgescript_heapptr_t NAME_ADDR = 0;
static const surgescript_heapptr_t TRANSFORM_ADDR = 1;
static const surgescript_heapptr_t COLLIDER_ADDR = 2;
static const surgescript_heapptr_t ANIMATION_ADDR = 3;
static const surgescript_heapptr_t INPUT_ADDR = 4;
static const surgescript_heapptr_t COMPANION_BASE_ADDR = 5;
static inline player_t* get_player(const surgescript_object_t* object);
static inline surgescript_object_t* get_collider(surgescript_object_t* object);
static inline surgescript_object_t* get_animation(surgescript_object_t* object);
static void update_player(surgescript_object_t* object);
static void update_collider(surgescript_object_t* object, int width, int height);
static void update_transform(surgescript_object_t* object, v2d_t world_position, float rotation_degrees);
static void update_animation(surgescript_object_t* object, int anim_id);
static void read_transform(surgescript_object_t* object, v2d_t* world_position);
static const double RAD2DEG = 57.2957795131;
#define FIXANG(rad) ((rad) >= 0.0 ? (rad) * RAD2DEG : 360.0 + (rad) * RAD2DEG)


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
    surgescript_tagsystem_add_tag(tag_system, "Player", "player");

    /* read-only properties */
    surgescript_vm_bind(vm, "Player", "get_name", fun_getname, 0);
    surgescript_vm_bind(vm, "Player", "get_activity", fun_getactivity, 0);
    surgescript_vm_bind(vm, "Player", "get_attacking", fun_getattacking, 0);
    surgescript_vm_bind(vm, "Player", "get_midair", fun_getmidair, 0);
    surgescript_vm_bind(vm, "Player", "get_blinking", fun_getblinking, 0);
    surgescript_vm_bind(vm, "Player", "get_secondsToDrown", fun_getsecondstodrown, 0);
    surgescript_vm_bind(vm, "Player", "get_transform", fun_gettransform, 0);
    surgescript_vm_bind(vm, "Player", "get_collider", fun_getcollider, 0);
    surgescript_vm_bind(vm, "Player", "get_direction", fun_getdirection, 0);
    surgescript_vm_bind(vm, "Player", "get_width", fun_getwidth, 0);
    surgescript_vm_bind(vm, "Player", "get_height", fun_getheight, 0);
    surgescript_vm_bind(vm, "Player", "get_animation", fun_getanimation, 0);
    surgescript_vm_bind(vm, "Player", "get_angle", fun_getangle, 0);
    surgescript_vm_bind(vm, "Player", "get_topspeed", fun_gettopspeed, 0);
    surgescript_vm_bind(vm, "Player", "get_input", fun_getinput, 0);
    surgescript_vm_bind(vm, "Player", "get_dying", fun_getdying, 0);
    surgescript_vm_bind(vm, "Player", "get_stopped", fun_getstopped, 0);
    surgescript_vm_bind(vm, "Player", "get_walking", fun_getwalking, 0);
    surgescript_vm_bind(vm, "Player", "get_running", fun_getrunning, 0);
    surgescript_vm_bind(vm, "Player", "get_waiting", fun_getwaiting, 0);
    surgescript_vm_bind(vm, "Player", "get_jumping", fun_getjumping, 0);
    surgescript_vm_bind(vm, "Player", "get_springing", fun_getspringing, 0);
    surgescript_vm_bind(vm, "Player", "get_rolling", fun_getrolling, 0);
    surgescript_vm_bind(vm, "Player", "get_charging", fun_getcharging, 0);
    surgescript_vm_bind(vm, "Player", "get_pushing", fun_getpushing, 0);
    surgescript_vm_bind(vm, "Player", "get_hit", fun_gethit, 0);
    surgescript_vm_bind(vm, "Player", "get_braking", fun_getbraking, 0);
    surgescript_vm_bind(vm, "Player", "get_balancing", fun_getbalancing, 0);
    surgescript_vm_bind(vm, "Player", "get_drowning", fun_getdrowning, 0);
    surgescript_vm_bind(vm, "Player", "get_breathing", fun_getbreathing, 0);
    surgescript_vm_bind(vm, "Player", "get_crouchingDown", fun_getcrouchingdown, 0);
    surgescript_vm_bind(vm, "Player", "get_lookingUp", fun_getlookingup, 0);
    surgescript_vm_bind(vm, "Player", "get_winning", fun_getwinning, 0);

    /* read-write properties */
    surgescript_vm_bind(vm, "Player", "get_anim", fun_getanim, 0);
    surgescript_vm_bind(vm, "Player", "set_anim", fun_setanim, 1);
    surgescript_vm_bind(vm, "Player", "get_shield", fun_getshield, 0);
    surgescript_vm_bind(vm, "Player", "set_shield", fun_setshield, 1);
    surgescript_vm_bind(vm, "Player", "get_invincible", fun_getinvincible, 0);
    surgescript_vm_bind(vm, "Player", "set_invincible", fun_setinvincible, 1);
    surgescript_vm_bind(vm, "Player", "get_turbo", fun_getturbo, 0);
    surgescript_vm_bind(vm, "Player", "set_turbo", fun_setturbo, 1);
    surgescript_vm_bind(vm, "Player", "get_underwater", fun_getunderwater, 0);
    surgescript_vm_bind(vm, "Player", "set_underwater", fun_setunderwater, 1);
    surgescript_vm_bind(vm, "Player", "get_frozen", fun_getfrozen, 0);
    surgescript_vm_bind(vm, "Player", "set_frozen", fun_setfrozen, 1);
    surgescript_vm_bind(vm, "Player", "get_layer", fun_getlayer, 0);
    surgescript_vm_bind(vm, "Player", "set_layer", fun_setlayer, 1);
    surgescript_vm_bind(vm, "Player", "get_visible", fun_getvisible, 0);
    surgescript_vm_bind(vm, "Player", "set_visible", fun_setvisible, 1);
    surgescript_vm_bind(vm, "Player", "get_speed", fun_getspeed, 0);
    surgescript_vm_bind(vm, "Player", "set_speed", fun_setspeed, 1);
    surgescript_vm_bind(vm, "Player", "get_gsp", fun_getgsp, 0);
    surgescript_vm_bind(vm, "Player", "set_gsp", fun_setgsp, 1);
    surgescript_vm_bind(vm, "Player", "get_xsp", fun_getxsp, 0);
    surgescript_vm_bind(vm, "Player", "set_xsp", fun_setxsp, 1);
    surgescript_vm_bind(vm, "Player", "get_ysp", fun_getysp, 0);
    surgescript_vm_bind(vm, "Player", "set_ysp", fun_setysp, 1);
    surgescript_vm_bind(vm, "Player", "get_collectibles", fun_getcollectibles, 0);
    surgescript_vm_bind(vm, "Player", "set_collectibles", fun_setcollectibles, 1);
    surgescript_vm_bind(vm, "Player", "get_lives", fun_getlives, 0);
    surgescript_vm_bind(vm, "Player", "set_lives", fun_setlives, 1);
    surgescript_vm_bind(vm, "Player", "get_score", fun_getscore, 0);
    surgescript_vm_bind(vm, "Player", "set_score", fun_setscore, 1);

    /* player-specific methods */
    surgescript_vm_bind(vm, "Player", "bounce", fun_bounce, 1);
    surgescript_vm_bind(vm, "Player", "bounceBack", fun_bounceback, 1);
    surgescript_vm_bind(vm, "Player", "hit", fun_hit, 1);
    surgescript_vm_bind(vm, "Player", "kill", fun_kill, 0);
    surgescript_vm_bind(vm, "Player", "breathe", fun_breathe, 0);
    surgescript_vm_bind(vm, "Player", "springify", fun_springify, 0);
    surgescript_vm_bind(vm, "Player", "roll", fun_roll, 0);
    surgescript_vm_bind(vm, "Player", "focus", fun_focus, 0);
    surgescript_vm_bind(vm, "Player", "hasFocus", fun_hasfocus, 0);
    surgescript_vm_bind(vm, "Player", "hlock", fun_hlock, 1);
    
    /* general-purpose methods */
    surgescript_vm_bind(vm, "Player", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Player", "__init", fun_init, 1);
    surgescript_vm_bind(vm, "Player", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Player", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Player", "onTransformChange", fun_ontransformchange, 1);
    surgescript_vm_bind(vm, "Player", "onAnimationChange", fun_onanimationchange, 1);

    /* misc */
    surgescript_vm_bind(vm, "PlayerManager", "state:main", fun_manager_main, 0);
    surgescript_vm_bind(vm, "PlayerManager", "destroy", fun_manager_destroy, 0);
    surgescript_vm_bind(vm, "PlayerManager", "__spawnPlayers", fun_manager_spawnplayers, 0);
    surgescript_vm_bind(vm, "PlayerManager", "get_count", fun_manager_getcount, 0);
    surgescript_vm_bind(vm, "PlayerManager", "get_active", fun_manager_getactive, 0);
    surgescript_vm_bind(vm, "PlayerManager", "__getById", fun_manager_getbyid, 1);
    surgescript_vm_bind(vm, "PlayerManager", "__getByName", fun_manager_getbyname, 1);
    surgescript_vm_bind(vm, "PlayerManager", "get_initialLives", fun_manager_getinitiallives, 0);
    surgescript_vm_bind(vm, "PlayerManager", "exists", fun_manager_exists, 1);
    surgescript_vm_bind(vm, "PlayerManager", "get", fun_manager_get, 1);
    surgescript_vm_bind(vm, "PlayerManager", "call", fun_manager_call, 1);
}

/*
 * scripting_player_ptr()
 * Returns a built-in player_t*, given a SurgeScript Player object
 * This will fail if no player_t* has been associated to the object
 */
player_t* scripting_player_ptr(const surgescript_object_t* object)
{
    player_t* player = get_player(object);

    if(player == NULL) {
        surgescript_heap_t* heap = surgescript_object_heap(object);
        const char* name = surgescript_var_fast_get_string(surgescript_heap_at(heap, NAME_ADDR));
        scripting_error(object, "Player not found - \"%s\"", name);
    }

    return player;
}

/* Player routines */

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t transform = surgescript_objectmanager_spawn(manager, me, "Transform2D", NULL);
    surgescript_objecthandle_t animation = surgescript_objectmanager_spawn(manager, me, "Animation", NULL);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    surgescript_var_t* tmp[5] = {
        surgescript_var_set_objecthandle(surgescript_var_create(), me),
        surgescript_var_set_number(surgescript_var_create(), 1.0f),
        surgescript_var_set_number(surgescript_var_create(), 1.0f),
        surgescript_var_set_number(surgescript_var_create(), 0.0f),
        surgescript_var_set_number(surgescript_var_create(), 0.0f)
    };

    ssassert(NAME_ADDR == surgescript_heap_malloc(heap));
    ssassert(TRANSFORM_ADDR == surgescript_heap_malloc(heap));
    ssassert(COLLIDER_ADDR == surgescript_heap_malloc(heap));
    ssassert(ANIMATION_ADDR == surgescript_heap_malloc(heap));
    ssassert(INPUT_ADDR == surgescript_heap_malloc(heap));

    surgescript_var_set_null(surgescript_heap_at(heap, NAME_ADDR));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, TRANSFORM_ADDR), transform);
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, ANIMATION_ADDR), animation);
    surgescript_var_set_null(surgescript_heap_at(heap, INPUT_ADDR));
    surgescript_object_set_userdata(object, NULL);

    /* spawn the collider */
    surgescript_object_call_function(
        scripting_util_surgeengine_component(surgescript_vm(), "Collisions"),
        "get_CollisionBox", NULL, 0, tmp[4]
    );
    surgescript_object_call_function(
        surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(tmp[4])),
        "__spawn", (const surgescript_var_t**)tmp, 3, tmp[3]
    );
    surgescript_var_copy(surgescript_heap_at(heap, COLLIDER_ADDR), tmp[3]);

    /* show the colliders? */
    #if SHOW_COLLIDERS != 0
    surgescript_var_set_bool(tmp[1], true);
    surgescript_object_call_function(
        surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(surgescript_heap_at(heap, COLLIDER_ADDR))),
        "set_visible", (const surgescript_var_t**)tmp+1, 1, NULL
    );
    #endif

    /* Player must be a child of PlayerManager */
    if(strcmp(surgescript_object_name(parent), "PlayerManager") != 0)
        scripting_error(object, "Object \"%s\" cannot be a child of \"%s\".", surgescript_object_name(object), surgescript_object_name(parent));

    /* done */
    surgescript_var_destroy(tmp[4]);
    surgescript_var_destroy(tmp[3]);
    surgescript_var_destroy(tmp[2]);
    surgescript_var_destroy(tmp[1]);
    surgescript_var_destroy(tmp[0]);
    return NULL;
}

/* __init: pass a character name */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* name = surgescript_heap_at(heap, NAME_ADDR);
    surgescript_objecthandle_t handle = surgescript_object_handle(object);
    player_t* player = NULL;

    /* grab player by name */
    surgescript_var_set_string(name, surgescript_var_fast_get_string(param[0]));
    update_player(object);

    /* initialize specifics */
    if((player = get_player(object)) != NULL) {
        /* initialize the Animation */
        surgescript_var_t* sprite_name = surgescript_var_set_string(surgescript_var_create(), player_sprite_name(player));
        surgescript_object_t* animation = get_animation(object);
        const surgescript_var_t* p[] = { sprite_name };
        surgescript_object_call_function(animation, "__init", p, 1, NULL);
        surgescript_var_destroy(sprite_name);

        /* initialize the Input object */
        surgescript_var_set_objecthandle(surgescript_heap_at(heap, INPUT_ADDR),
            surgescript_objectmanager_spawn(manager, handle, "Input", player->actor->input)
        );

        /* spawn the companion objects */
        if(player_companion_name(player, 0) != NULL) {
            const char* companion_name = NULL;
            surgescript_programpool_t* pool = surgescript_objectmanager_programpool(manager);
            surgescript_objecthandle_t companion, null_handle = surgescript_objectmanager_null(manager);
            for(int i = 0; (companion_name = player_companion_name(player, i)) != NULL; i++) {
                /* allocate memory */
                surgescript_heapptr_t addr = COMPANION_BASE_ADDR + i;
                if(!surgescript_heap_validaddress(heap, addr))
                    ssassert(addr == surgescript_heap_malloc(heap));

                /* spawn the object */
                if(surgescript_programpool_is_compiled(pool, companion_name)) {
                    /* spawn the companion in SurgeScript */
                    if(surgescript_object_child(object, companion_name) == null_handle) {
                        companion = surgescript_objectmanager_spawn(manager, handle, companion_name, NULL);
                        surgescript_var_set_objecthandle(surgescript_heap_at(heap, addr), companion);
                    }
                }
                else {
                    /* the companion doesn't exist in SurgeScript: use the legacy API */
                    logfile_message("Warning: no SurgeScript object found for companion \"%s\" (of \"%s\")", companion_name, player_name(player));
                    level_create_legacy_object(companion_name, v2d_new(0, 0));
                    surgescript_var_set_null(surgescript_heap_at(heap, addr));
                }
            }
        }
    }
    else
        scripting_error(object, "Player.__init(): can't get the Player pointer for \"%s\"", surgescript_var_fast_get_string(name));

    /* done! */
    return surgescript_var_set_bool(surgescript_var_create(), true);
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

/* onTransformChange(transform): the player transform was changed somewhere in the script */
surgescript_var_t* fun_ontransformchange(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* we'll tell the engine about the new position of the player;
       other parameters (angle, scale) will be ignored */
    v2d_t world_position;
    player_t* player = get_player(object);
    if(player != NULL) {
        read_transform(object, &world_position);
        player->actor->position = world_position;
    }
    return NULL;
}

/* gets the name of the player */
surgescript_var_t* fun_getname(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL)
        return surgescript_var_set_string(surgescript_var_create(), player_name(player));
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
                return surgescript_var_set_string(surgescript_var_create(), "dying");
            case PAS_BRAKING:
                return surgescript_var_set_string(surgescript_var_create(), "braking");
            case PAS_LEDGE:
                return surgescript_var_set_string(surgescript_var_create(), "balancing");
            case PAS_DROWNED:
                return surgescript_var_set_string(surgescript_var_create(), "drowning");
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

/* returns true if the player is dying or drowning */
surgescript_var_t* fun_getdying(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && player_is_dying(player));
}

/* player in midair? */
surgescript_var_t* fun_getmidair(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && player_is_in_the_air(player));
}

/* is the player blinking? */
surgescript_var_t* fun_getblinking(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && player_is_blinking(player));
}

/* seconds to drown, if underwater */
surgescript_var_t* fun_getsecondstodrown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(), player != NULL ? player_seconds_remaining_to_drown(player) : INFINITY);
}

/* returns true if the player is stopped */
surgescript_var_t* fun_getstopped(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_STOPPED);
}

/* returns true if the player is walking */
surgescript_var_t* fun_getwalking(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_WALKING);
}

/* returns true if the player is running */
surgescript_var_t* fun_getrunning(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_RUNNING);
}

/* returns true if the player is waiting */
surgescript_var_t* fun_getwaiting(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_WAITING);
}

/* returns true if the player is jumping */
surgescript_var_t* fun_getjumping(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_JUMPING);
}

/* returns true if the player is in the "springing" state */
surgescript_var_t* fun_getspringing(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_SPRINGING);
}

/* returns true if the player is rolling */
surgescript_var_t* fun_getrolling(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_ROLLING);
}

/* returns true if the player is charging a rolling movement */
surgescript_var_t* fun_getcharging(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_CHARGING);
}

/* returns true if the player is pushing a wall */
surgescript_var_t* fun_getpushing(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_PUSHING);
}

/* returns true if the player is getting hit */
surgescript_var_t* fun_gethit(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_GETTINGHIT);
}

/* returns true if the player is braking */
surgescript_var_t* fun_getbraking(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_BRAKING);
}

/* returns true if the player is balancing on a ledge */
surgescript_var_t* fun_getbalancing(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_LEDGE);
}

/* returns true if the player is drowning */
surgescript_var_t* fun_getdrowning(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_DROWNED);
}

/* returns true if the player is breathing an air bubble underwater */
surgescript_var_t* fun_getbreathing(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_BREATHING);
}

/* returns true if the player is crouching down */
surgescript_var_t* fun_getcrouchingdown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_DUCKING);
}

/* returns true if the player is looking up */
surgescript_var_t* fun_getlookingup(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_LOOKINGUP);
}

/* returns true if the player is in the "winning" state, displayed after clearing a level */
surgescript_var_t* fun_getwinning(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && physicsactor_get_state(player->pa) == PAS_WINNING);
}

/* Transform component */
surgescript_var_t* fun_gettransform(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, TRANSFORM_ADDR));
}

/* the collider */
surgescript_var_t* fun_getcollider(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, COLLIDER_ADDR));
}

/* the input object */
surgescript_var_t* fun_getinput(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, INPUT_ADDR));
}

/* direction is +1 if the player is facing right; -1 if facing left */
surgescript_var_t* fun_getdirection(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(), player == NULL || physicsactor_is_facing_right(player->pa) ? 1.0f : -1.0f);
}

/* sprite width */
surgescript_var_t* fun_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(), player != NULL ? image_width(actor_image(player->actor)) : 0.0f);
}

/* sprite height */
surgescript_var_t* fun_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(), player != NULL ? image_height(actor_image(player->actor)) : 0.0f);
}

/* player angle, in degrees */
surgescript_var_t* fun_getangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(),
        (player != NULL) ? FIXANG(player->actor->angle) : 0.0f
    );
}

/* top speed, in px/s */
surgescript_var_t* fun_gettopspeed(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(),
        (player != NULL) ? physicsactor_get_topspeed(player->pa) : 0.0f
    );
}

/* the initial number of lives */
surgescript_var_t* fun_getinitiallives(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), PLAYER_INITIAL_LIVES);
}

/* player speed, in px/s (maps to either xsp or gsp, if the player is in the air or not) */
surgescript_var_t* fun_getspeed(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        if(player_is_in_the_air(player) || player_is_getting_hit(player) || player_is_dying(player))
            return surgescript_var_set_number(surgescript_var_create(), physicsactor_get_xsp(player->pa));
        else
            return surgescript_var_set_number(surgescript_var_create(), physicsactor_get_gsp(player->pa));
    }
    else
        return surgescript_var_set_number(surgescript_var_create(), 0.0f);
}

/* set player speed, in px/s */
surgescript_var_t* fun_setspeed(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        float speed = surgescript_var_get_number(param[0]);
        player->actor->speed.x = speed;
        if(player_is_in_the_air(player) || player_is_getting_hit(player) || player_is_dying(player))
            physicsactor_set_xsp(player->pa, speed);
        else
            physicsactor_set_gsp(player->pa, speed);
    }
    return NULL;
}

/* ground speed, in px/s */
surgescript_var_t* fun_getgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(),
        (player != NULL) ? physicsactor_get_gsp(player->pa) : 0.0f
    );
}

/* set ground speed, in px/s */
surgescript_var_t* fun_setgsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    if(player != NULL) {
        if(!(player_is_in_the_air(player) || player_is_getting_hit(player) || player_is_dying(player)))
            player->actor->speed.x = surgescript_var_get_number(param[0]);
    }
    return NULL;
}

/* horizontal speed, in px/s */
surgescript_var_t* fun_getxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(),
        (player != NULL) ? physicsactor_get_xsp(player->pa) : 0.0f
    );
}

/* set horizontal speed, in px/s */
surgescript_var_t* fun_setxsp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    if(player != NULL) {
        if(player_is_in_the_air(player) || player_is_getting_hit(player) || player_is_dying(player))
            player->actor->speed.x = surgescript_var_get_number(param[0]);
    }
    return NULL;
}

/* vertical speed, in px/s */
surgescript_var_t* fun_getysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: fix adapter */
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(),
        (player != NULL) ? physicsactor_get_ysp(player->pa) : 0.0f
    );
}

/* set vertical speed, in px/s */
surgescript_var_t* fun_setysp(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        float ysp = surgescript_var_get_number(param[0]);
        player->actor->speed.y = ysp;

        /* hack */
        if(ysp < 0.0f && !player_is_in_the_air(player)) {
            if(!player_is_rolling(player))
                player->actor->position.y -= 2;
            else
                player->actor->position.y -= 4;
        }
    }
    return NULL;
}

/* set animation number */
surgescript_var_t* fun_setanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* call animation.set_id */
    surgescript_object_t* animation = get_animation(object);
    const surgescript_var_t* p[] = { param[0] };
    surgescript_object_call_function(animation, "set_id", p, 1, NULL);
    return NULL;
}

/* get animation number */
surgescript_var_t* fun_getanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* call animation.get_id */
    surgescript_object_t* animation = get_animation(object);
    surgescript_var_t* anim_id = surgescript_var_create();
    surgescript_object_call_function(animation, "get_id", NULL, 0, anim_id);
    return anim_id;
}

/* get animation object */
surgescript_var_t* fun_getanimation(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ANIMATION_ADDR));
}

/* animation change callback */
surgescript_var_t* fun_onanimationchange(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t animation_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* animation = surgescript_objectmanager_get(manager, animation_handle);
    player_t* player = get_player(object);
    if(player != NULL)
        player_override_animation(player, scripting_animation_ptr(animation));
    return NULL;
}

/* get the number of collectibles (shared between all players) */
surgescript_var_t* fun_getcollectibles(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), player_get_collectibles());
}

/* set the number of collectibles (shared between all players) */
surgescript_var_t* fun_setcollectibles(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int collectibles = (int)surgescript_var_get_number(param[0]);
    player_set_collectibles(max(collectibles, 0));
    return NULL;
}

/* get the number of lives (shared between all players) */
surgescript_var_t* fun_getlives(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), player_get_lives());
}

/* set the number of lives (shared between all players) */
surgescript_var_t* fun_setlives(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int lives = (int)surgescript_var_get_number(param[0]);
    player_set_lives(max(lives, 0));
    return NULL;
}

/* get the score (shared between all players) */
surgescript_var_t* fun_getscore(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), player_get_score());
}

/* set the score (shared between all players) */
surgescript_var_t* fun_setscore(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int score = (int)surgescript_var_get_number(param[0]);
    player_set_score(max(score, 0));
    return NULL;
}

/* is the player visible? */
surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_number(surgescript_var_create(), player != NULL && player_is_visible(player));
}

/* set the visibility of the player */
surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        bool visible = surgescript_var_get_bool(param[0]);
        player_set_visible(player, visible);
    }
    return NULL;
}

/* returns the name of the current shield, or "none" if no shield is present */
surgescript_var_t* fun_getshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        switch(player_shield_type(player)) {
            case SH_NONE:
                return surgescript_var_set_null(surgescript_var_create());
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
    return surgescript_var_set_null(surgescript_var_create());
}

/* grants the player a shield */
surgescript_var_t* fun_setshield(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        const char* shield = surgescript_var_fast_get_string(param[0]);
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

/* is the player frozen (i.e., with its movement disabled)? */
surgescript_var_t* fun_getfrozen(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && player_is_frozen(player));
}

/* enable/disable the movement of the player */
surgescript_var_t* fun_setfrozen(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        bool frozen = surgescript_var_get_bool(param[0]);
        player_set_frozen(player, frozen);
    }
    return NULL;
}

/* the current layer of the player. One of the following: "green", "yellow", "default" */
surgescript_var_t* fun_getlayer(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    const char* layer = "default";
    if(player != NULL) {
        switch(player_layer(player)) {
            case BRL_GREEN:
                layer = "green";
                break;
            case BRL_YELLOW:
                layer = "yellow";
                break;
            default:
                break;
        }
    }
    return surgescript_var_set_string(surgescript_var_create(), layer);
}

/* set the current layer of the player to one of the following: "green", "yellow", "default" */
surgescript_var_t* fun_setlayer(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        const char* layer = surgescript_var_fast_get_string(param[0]);
        if(strcmp(layer, "green") == 0)
            player_set_layer(player, BRL_GREEN);
        else if(strcmp(layer, "yellow") == 0)
            player_set_layer(player, BRL_YELLOW);
        else
            player_set_layer(player, BRL_DEFAULT);
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
                scripting_error(object, "%s.bounce(hazard) requires hazard to be an Actor | null, but hazard is %s.", surgescript_object_name(object), surgescript_object_name(hazard));
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
                scripting_error(object, "%s.bounceBack(hazard) requires hazard to be an Actor, but hazard is %s.", surgescript_object_name(object), surgescript_object_name(hazard));
        }
        else
            scripting_error(object, "%s.bounceBack(hazard) requires hazard to be an Actor, but hazard is null.", surgescript_object_name(object));
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
                scripting_error(object, "%s.hit(hazard) requires hazard to be an Actor | null, but hazard is %s.", surgescript_object_name(object), surgescript_object_name(hazard));
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
    if(player != NULL) {
        if(!player_is_underwater(player))
            player_kill(player);
        else
            player_drown(player);
    }
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

/* bring the focus to this player: returns true on success */
surgescript_var_t* fun_focus(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    if(player != NULL) {
        player_t* p;

        /* error if player is dying */
        for(int i = 0; (p = level_get_player_by_id(i)) != NULL; i++) {
            if(player_is_dying(p))
                return surgescript_var_set_bool(surgescript_var_create(), false);
        }

        /* error if player is in the air, etc. */
        if(player_is_in_the_air(player) || player_is_frozen(player) || player->in_locked_area || player->on_movable_platform)
            return surgescript_var_set_bool(surgescript_var_create(), false);

        /* error if level cleared */
        if(level_has_been_cleared())
            return surgescript_var_set_bool(surgescript_var_create(), false);

        /* success */
        level_change_player(player);
        return surgescript_var_set_bool(surgescript_var_create(), true);
    }

    /* error */
    return surgescript_var_set_bool(surgescript_var_create(), false);
}

/* checks if this player has focus */
surgescript_var_t* fun_hasfocus(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* player = get_player(object);
    return surgescript_var_set_bool(surgescript_var_create(), player != NULL && level_player() == player);
}

/* hlock: locks the horizontal input of the player for a few seconds */
surgescript_var_t* fun_hlock(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    float seconds = surgescript_var_get_number(param[0]);
    player_t* player = get_player(object);

    if(player != NULL && seconds > 0.0f)
        player_lock_horizontally_for(player, seconds);

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

/* get the Animation SurgeScript object (child object) */
surgescript_object_t* get_animation(surgescript_object_t* object)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t animation_handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, ANIMATION_ADDR));
    surgescript_object_t* animation = surgescript_objectmanager_get(manager, animation_handle);
    return animation;
}

/* returns the collider of the player */
surgescript_object_t* get_collider(surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* col = surgescript_heap_at(heap, COLLIDER_ADDR);
    return surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(col));
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
        update_transform(object, player->actor->position, FIXANG(player->actor->angle));
    else
        update_transform(object, v2d_new(0.0f, 0.0f), 0.0f);

    /* update the collider */
    if(player != NULL) {
        int width = 1, height = 1;
        physicsactor_bounding_box(player->pa, &width, &height, NULL);
        update_collider(object, width, height);
    }
    else
        update_collider(object, 1, 1);

    /* update the animation */
    if(player != NULL)
        update_animation(object, player_animation(player)->id);
    else
        update_animation(object, 0);

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
    surgescript_transform_setposition2d(&transform, world_position.x, world_position.y); /* this assumes local position == world position */
    surgescript_transform_setrotation2d(&transform, rotation_degrees);
    surgescript_transform_setscale2d(&transform, 1.0f, 1.0f);
    surgescript_object_poke_transform(object, &transform);
}

/* read the player transform */
void read_transform(surgescript_object_t* object, v2d_t* world_position)
{
    surgescript_transform_t transform;
    surgescript_object_peek_transform(object, &transform);
    *world_position = v2d_new(transform.position.x, transform.position.y); /* assuming local position == world position */
}

/* update the collider */
void update_collider(surgescript_object_t* object, int width, int height)
{
    surgescript_object_t* collider = get_collider(object);
    surgescript_var_t* w = surgescript_var_set_number(surgescript_var_create(), width);
    surgescript_var_t* h = surgescript_var_set_number(surgescript_var_create(), height);
    const surgescript_var_t* tmp[2] = { w, h };
    surgescript_object_call_function(collider, "set_width", tmp+0, 1, NULL);
    surgescript_object_call_function(collider, "set_height", tmp+1, 1, NULL);
    surgescript_var_destroy(h);
    surgescript_var_destroy(w);
}

/* update the animation */
void update_animation(surgescript_object_t* object, int anim_id)
{
    surgescript_object_t* animation = get_animation(object);
    scripting_animation_overwrite_id(animation, anim_id);
}

/* PlayerManager: main state */
surgescript_var_t* fun_manager_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
}

/* can't destroy the PlayerManager */
surgescript_var_t* fun_manager_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* spawn (initial) Player objects */
surgescript_var_t* fun_manager_spawnplayers(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_var_t* player_handle = surgescript_var_create();
    surgescript_var_t* my_param = surgescript_var_create();
    surgescript_heap_t* heap = surgescript_object_heap(object);
    player_t* player;

    /* spawn player i = 0, 1, ... */
    for(int i = 0; (player = level_get_player_by_id(i)) != NULL; i++) {
        const surgescript_var_t* p[] = { my_param };
        surgescript_var_set_string(my_param, "Player");
        surgescript_object_call_function(object, "spawn", p, 1, player_handle);
        surgescript_var_copy(surgescript_heap_at(heap, surgescript_heap_malloc(heap)), player_handle); /* heap_malloc */
        surgescript_var_set_string(my_param, player_name(player));
        surgescript_object_call_function(
            surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(player_handle)),
            "__init", p, 1, NULL
        );
    }

    /* done */
    surgescript_var_destroy(my_param);
    surgescript_var_destroy(player_handle);
    return NULL;
}

/* the number of players in the scene */
surgescript_var_t* fun_manager_getcount(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), surgescript_object_child_count(object));
}

/* get the active player (i-th child) */
surgescript_var_t* fun_manager_getactive(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    player_t* current_player = level_player(), *player;

    for(int i = 0; (player = level_get_player_by_id(i)) != NULL; i++) {
        if(player == current_player) {
            surgescript_objecthandle_t handle = surgescript_object_nth_child(object, i);
            return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
        }
    }

    return NULL;
}

/* get player by id (0, 1, ..., NUM_PLAYERS - 1) */
surgescript_var_t* fun_manager_getbyid(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int id = (int)surgescript_var_get_number(param[0]);
    player_t* player = level_get_player_by_id(id);

    if(player != NULL) {
        surgescript_objecthandle_t handle = surgescript_object_nth_child(object, id);
        return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
    }

    return NULL;
}

/* get player by name (returns null if not found) */
surgescript_var_t* fun_manager_getbyname(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* name = surgescript_var_fast_get_string(param[0]);
    player_t* player;

    for(int i = 0; (player = level_get_player_by_id(i)) != NULL; i++) {
        if(str_icmp(player_name(player), name) == 0) { /* will accept case-insensitive matches (e.g. "none" is "None") */
            surgescript_objecthandle_t handle = surgescript_object_nth_child(object, i);
            return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
        }
    }

    return NULL;
}

/* the initial number of lives */
surgescript_var_t* fun_manager_getinitiallives(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return fun_getinitiallives(object, param, num_params); /* just an alias */
}

/* does the given player exist in the scene? */
surgescript_var_t* fun_manager_exists(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_var_t* ret = fun_manager_getbyname(object, param, num_params);
    bool exists = (ret != NULL) && !surgescript_var_is_null(ret);

    if(ret != NULL)
        surgescript_var_destroy(ret);

    return surgescript_var_set_bool(surgescript_var_create(), exists);
}

/* [] operator: get player by ID. Crash on error */
surgescript_var_t* fun_manager_get(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int id = (int)surgescript_var_get_number(param[0]);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t null_handle = surgescript_objectmanager_null(manager);
    surgescript_objecthandle_t handle = null_handle;

    surgescript_var_t* ret = fun_manager_getbyid(object, param, num_params);
    if(ret != NULL) {
        handle = surgescript_var_get_objecthandle(ret);
        surgescript_var_destroy(ret);
    }

    if(handle == null_handle)
        scripting_error(object, "Can't find Player #%d: no such player in the scene.", id);

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* () operator: get player by name. Crash on error */
surgescript_var_t* fun_manager_call(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* name = surgescript_var_fast_get_string(param[0]);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t null_handle = surgescript_objectmanager_null(manager);
    surgescript_objecthandle_t handle = null_handle;

    surgescript_var_t* ret = fun_manager_getbyname(object, param, num_params);
    if(ret != NULL) {
        handle = surgescript_var_get_objecthandle(ret);
        surgescript_var_destroy(ret);
    }

    if(handle == null_handle)
        scripting_error(object, "Can't find Player \"%s\": no such player in the scene.", name);

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}