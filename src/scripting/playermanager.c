/*
 * Open Surge Engine
 * playermanager.c - scripting system: PlayerManager
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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
#include <stdbool.h>
#include <string.h>
#include "scripting.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../entities/player.h"
#include "../scenes/level.h"
#include "../core/video.h"


/*
 * The PlayerManager manages Player objects. It includes utility
 * functions, such as getting a Player object by name or by ID.
 */


static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_unload(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawnplayers(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getactive(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getcount(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getinitiallives(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_exists(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_get(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_call(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t PLAYERCOUNT_ADDR = 0;
static const surgescript_heapptr_t PLAYERBASE_ADDR = 1; /* must be the last address */
static bool get_player_by_id(const surgescript_object_t* player_manager, int id, surgescript_objecthandle_t* out_handle);
static bool get_player_by_name(const surgescript_object_t* player_manager, const char* name, surgescript_objecthandle_t* out_handle);

/*

heap layout:

[ PLAYER_COUNT | handle_to_first_player | handle_to_second_player | ... ]

                    ^ base_addr

*/




/*
 * scripting_register_playermanager()
 * Register the PlayerManager
 */
void scripting_register_playermanager(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "PlayerManager", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "PlayerManager", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "PlayerManager", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "PlayerManager", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "PlayerManager", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "PlayerManager", "__spawnPlayers", fun_spawnplayers, 0);
    surgescript_vm_bind(vm, "PlayerManager", "__unload", fun_unload, 0);
    surgescript_vm_bind(vm, "PlayerManager", "get_count", fun_getcount, 0);
    surgescript_vm_bind(vm, "PlayerManager", "get_active", fun_getactive, 0);
    surgescript_vm_bind(vm, "PlayerManager", "get_initialLives", fun_getinitiallives, 0);
    surgescript_vm_bind(vm, "PlayerManager", "exists", fun_exists, 1);
    surgescript_vm_bind(vm, "PlayerManager", "get", fun_get, 1); /* get by ID */
    surgescript_vm_bind(vm, "PlayerManager", "call", fun_call, 1); /* get by name */
}



/*
 * SurgeScript API
 */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* validate */
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    const char* parent_name = surgescript_object_name(parent);
    ssassert(0 == strcmp(parent_name, "Level"));

    /* allocate variables */
    ssassert(PLAYERCOUNT_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_number(surgescript_heap_at(heap, PLAYERCOUNT_ADDR), 0);

    /* done */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /*video_showmessage("Called PlayerManager.destructor()");*/
    return NULL;
}

/* unload the PlayerManager */
surgescript_var_t* fun_unload(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* player_count_var = surgescript_heap_at(heap, PLAYERCOUNT_ADDR);

    /* release all user-added children of all instances of Player (e.g., companions),
       but don't destroy any Player instance, nor the PlayerManager itself,
       otherwise we will get crashes on object destructors that try to access them. */
    int player_count = (int)surgescript_var_get_number(player_count_var);
    for(int i = player_count - 1; i >= 0; i--) {
        surgescript_var_t* player_var = surgescript_heap_at(heap, PLAYERBASE_ADDR + i);
        surgescript_objecthandle_t player_handle = surgescript_var_get_objecthandle(player_var);
        surgescript_object_t* player = surgescript_objectmanager_get(manager, player_handle);

        surgescript_object_call_function(player, "__releaseChildren", NULL, 0, NULL);

        surgescript_var_set_null(player_var);
    }

    /* reset the counter */
    surgescript_var_set_number(player_count_var, 0);
    return NULL;
}

/* can't destroy the PlayerManager */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
}

/* spawn */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
}

/* spawn initial Player objects */
surgescript_var_t* fun_spawnplayers(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_var_t* my_param = surgescript_var_create();
    surgescript_var_t* v2_var = surgescript_var_create();
    const surgescript_var_t* p[] = { my_param, v2_var };

    /* get player count */
    surgescript_var_t* player_count_var = surgescript_heap_at(heap, PLAYERCOUNT_ADDR);
    int player_count = (int)surgescript_var_get_number(player_count_var);
    ssassert(0 == player_count); /* validate */

    /* get the Level object */
    surgescript_objecthandle_t level_handle = surgescript_object_parent(object);
    surgescript_object_t* level = surgescript_objectmanager_get(manager, level_handle);

    /* v2 = Vector2(0, 0) */
    surgescript_objecthandle_t v2_handle = surgescript_objectmanager_spawn_temp(manager, "Vector2");
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, v2_handle);
    scripting_vector2_update(v2, 0.0, 0.0); /* the Player object will reposition itself */
    surgescript_var_set_objecthandle(v2_var, v2_handle);

    /* spawn player i = 0, 1, ... */
    const player_t* player;
    for(int i = 0; (player = level_get_player_by_id(i)) != NULL; i++) {

        /* spawn the player as a child of Level */
        surgescript_var_set_string(my_param, "Player");
        surgescript_object_call_function(level, "spawnEntity", p, 2, ret);
        surgescript_objecthandle_t player_handle = surgescript_var_get_objecthandle(ret);

        /* store the player in the heap */
        surgescript_heapptr_t player_addr = surgescript_heap_malloc(heap);
        ssassert(player_addr == PLAYERBASE_ADDR + player_count); /* validate */
        surgescript_var_t* player_var = surgescript_heap_at(heap, player_addr);
        surgescript_var_set_objecthandle(player_var, player_handle);

        /* initialize the player */
        surgescript_var_set_number(my_param, player_id(player));
        surgescript_object_call_function(
            surgescript_objectmanager_get(manager, player_handle),
            "__init", p, 1, NULL
        );

        /* increment player count */
        surgescript_var_set_number(player_count_var, ++player_count);

    }

    /* release v2 */
    surgescript_object_kill(v2);

    /* done */
    surgescript_var_destroy(v2_var);
    surgescript_var_destroy(my_param);
    surgescript_var_destroy(ret);
    return NULL;
}

/* the number of players in the Level */
surgescript_var_t* fun_getcount(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_heap_t* heap = surgescript_object_heap(object);
    const surgescript_var_t* player_count_var = surgescript_heap_at(heap, PLAYERCOUNT_ADDR);

    return surgescript_var_clone(player_count_var);
}

/* get the active player */
surgescript_var_t* fun_getactive(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_heap_t* heap = surgescript_object_heap(object);
    const player_t* current_player = level_player(), *player;

    /* find the i-th player p such that p is the active player */
    for(int i = 0; (player = level_get_player_by_id(i)) != NULL; i++) {
        if(player == current_player) {
            const surgescript_var_t* handle_var = surgescript_heap_at(heap, PLAYERBASE_ADDR + i);
            surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(handle_var);

            return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
        }
    }

    return NULL;
}

/* the initial number of lives */
surgescript_var_t* fun_getinitiallives(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), PLAYER_INITIAL_LIVES);
}

/* does the given player exist in the scene? */
surgescript_var_t* fun_exists(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_objectmanager_null(manager);
    const char* name = surgescript_var_fast_get_string(param[0]);

    bool exists = get_player_by_name(object, name, &handle);

    return surgescript_var_set_bool(surgescript_var_create(), exists);
}

/* [] operator: get player by ID. Crash on error */
surgescript_var_t* fun_get(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_objectmanager_null(manager);
    int id = (int)surgescript_var_get_number(param[0]);

    bool exists = get_player_by_id(object, id, &handle);
    if(!exists) {
        scripting_error(object, "Can't find Player #%d: no such player in the scene.", id);
        return surgescript_var_set_null(surgescript_var_create());
    }

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* () operator: get player by name. Crash on error */
surgescript_var_t* fun_call(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_objectmanager_null(manager);
    const char* name = surgescript_var_fast_get_string(param[0]);

    bool exists = get_player_by_name(object, name, &handle);
    if(!exists) {
        scripting_error(object, "Can't find Player \"%s\": no such player in the scene.", name);
        return surgescript_var_set_null(surgescript_var_create());
    }

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}




/*
 * Helpers
 */

/* get player by id (0, 1, 2...) - returns true on success */
bool get_player_by_id(const surgescript_object_t* player_manager, int id, surgescript_objecthandle_t* out_handle)
{
    const player_t* player = level_get_player_by_id(id);

    if(player != NULL) {
        const surgescript_heap_t* heap = surgescript_object_heap(player_manager);
        const surgescript_var_t* handle_var = surgescript_heap_at(heap, PLAYERBASE_ADDR + id);

        /* success */
        *out_handle = surgescript_var_get_objecthandle(handle_var);
        return true;
    }

    /* not found */
    return false;
}

/* get player by name - returns true on success */
bool get_player_by_name(const surgescript_object_t* player_manager, const char* name, surgescript_objecthandle_t* out_handle)
{
    const surgescript_heap_t* heap = surgescript_object_heap(player_manager);
    const player_t* player;

    for(int i = 0; (player = level_get_player_by_id(i)) != NULL; i++) {

        /* we accept case-insensitive matches (e.g. "none" is "None") */
        if(str_icmp(player_name(player), name) == 0) {
            const surgescript_var_t* handle_var = surgescript_heap_at(heap, PLAYERBASE_ADDR + i);

            /* success */
            *out_handle = surgescript_var_get_objecthandle(handle_var);
            return true;
        }

    }

    /* not found */
    return false;
}