/*
 * Open Surge Engine
 * level.c - scripting system: Level object
 * Copyright (C) 2018, 2019  Alexandre Martins <alemartf@gmail.com>
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
#include "scripting.h"
#include "../core/util.h"
#include "../core/audio.h"
#include "../core/stringutil.h"
#include "../scenes/level.h"
#include "../scenes/quest.h"
#include "../entities/player.h"
#include "../entities/background.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawnentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getwaterlevel(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setwaterlevel(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getspawnpoint(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setspawnpoint(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getcleared(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getname(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getact(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getauthor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlicense(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfile(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getbgtheme(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getbackground(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setbackground(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmusic(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getgravity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettime(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_clear(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_restart(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_quit(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_abort(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_pause(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_load(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_loadnext(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_entity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setup(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getnext(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setnext(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getonunload(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setonunload(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_callunloadfunctor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t MUSIC_ADDR = 0;
static const surgescript_heapptr_t SPAWNPOINT_ADDR = 1;
static const surgescript_heapptr_t SETUP_ADDR = 2;
static const surgescript_heapptr_t UNLOADFUNCTOR_ADDR = 3;
static const surgescript_heapptr_t IDX_ADDR = 4; /* must be the last address */
static const char code_in_surgescript[];
static void update_music(surgescript_object_t* object);

/*
 * scripting_register_level()
 * Register the Level object
 */
void scripting_register_level(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Level", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Level", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Level", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Level", "spawnEntity", fun_spawnentity, 2);
    surgescript_vm_bind(vm, "Level", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Level", "get_name", fun_getname, 0);
    surgescript_vm_bind(vm, "Level", "get_act", fun_getact, 0);
    surgescript_vm_bind(vm, "Level", "get_version", fun_getversion, 0);
    surgescript_vm_bind(vm, "Level", "get_author", fun_getauthor, 0);
    surgescript_vm_bind(vm, "Level", "get_license", fun_getlicense, 0);
    surgescript_vm_bind(vm, "Level", "get_file", fun_getfile, 0);
    surgescript_vm_bind(vm, "Level", "get_music", fun_getmusic, 0);
    surgescript_vm_bind(vm, "Level", "get_cleared", fun_getcleared, 0);
    surgescript_vm_bind(vm, "Level", "get_gravity", fun_getgravity, 0);
    surgescript_vm_bind(vm, "Level", "get_time", fun_gettime, 0);
    surgescript_vm_bind(vm, "Level", "get_bgtheme", fun_getbgtheme, 0);
    surgescript_vm_bind(vm, "Level", "set_waterlevel", fun_setwaterlevel, 1);
    surgescript_vm_bind(vm, "Level", "get_waterlevel", fun_getwaterlevel, 0);
    surgescript_vm_bind(vm, "Level", "set_spawnpoint", fun_setspawnpoint, 1);
    surgescript_vm_bind(vm, "Level", "get_spawnpoint", fun_getspawnpoint, 0);
    surgescript_vm_bind(vm, "Level", "set_background", fun_setbackground, 1);
    surgescript_vm_bind(vm, "Level", "get_background", fun_getbackground, 0);
    surgescript_vm_bind(vm, "Level", "set_next", fun_setnext, 1);
    surgescript_vm_bind(vm, "Level", "get_next", fun_getnext, 0);
    surgescript_vm_bind(vm, "Level", "set_onUnload", fun_setonunload, 1);
    surgescript_vm_bind(vm, "Level", "get_onUnload", fun_getonunload, 0);
    surgescript_vm_bind(vm, "Level", "clear", fun_clear, 0);
    surgescript_vm_bind(vm, "Level", "restart", fun_restart, 0);
    surgescript_vm_bind(vm, "Level", "quit", fun_quit, 0);
    surgescript_vm_bind(vm, "Level", "abort", fun_abort, 0);
    surgescript_vm_bind(vm, "Level", "pause", fun_pause, 0);
    surgescript_vm_bind(vm, "Level", "load", fun_load, 1);
    surgescript_vm_bind(vm, "Level", "loadNext", fun_loadnext, 0);
    surgescript_vm_bind(vm, "Level", "entity", fun_entity, 1);
    surgescript_vm_bind(vm, "Level", "setup", fun_setup, 1);
    surgescript_vm_bind(vm, "Level", "__callUnloadFunctor", fun_callunloadfunctor, 0);
    surgescript_vm_compile_code_in_memory(vm, code_in_surgescript);
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t spawnpoint = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
    surgescript_objecthandle_t setup = surgescript_objectmanager_spawn(manager, me, "Setup Level", NULL);

    /* Level music */
    ssassert(MUSIC_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, MUSIC_ADDR));

    /* spawn point */
    ssassert(SPAWNPOINT_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, SPAWNPOINT_ADDR), spawnpoint);

    /* Setup functor */
    ssassert(SETUP_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, SETUP_ADDR), setup);

    /* Unload functor */
    ssassert(UNLOADFUNCTOR_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, UNLOADFUNCTOR_ADDR));

    /*
     * Memory layout:
     * [ ... | IDX | obj_1 | obj_2 | ... | obj_N ]
     * only Level-spawned() objects come after IDX
     */
    ssassert(IDX_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_rawbits(surgescript_heap_at(heap, IDX_ADDR), 1 + IDX_ADDR);

    /* done */
    return NULL;
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_heapptr_t idx = surgescript_var_get_rawbits(surgescript_heap_at(heap, IDX_ADDR));
    size_t heap_size = surgescript_heap_size(heap);

    /* scan the memory continuously for broken references */
    if(idx > IDX_ADDR && idx < heap_size) {
        surgescript_objectmanager_t* manager = surgescript_object_manager(object);

        /* an object stored in heap[idx] has been destroyed */
        if(surgescript_heap_validaddress(heap, idx) && (
            surgescript_var_is_null(surgescript_heap_at(heap, idx)) ||
            !surgescript_objectmanager_exists(manager, surgescript_var_get_objecthandle(surgescript_heap_at(heap, idx)))
        ))
            surgescript_heap_free(heap, idx); /* release the memory, so it can be reused */
    }

    /* update the scan index on the object memory */
    idx = max(1 + IDX_ADDR, (idx + 1) % heap_size);
    surgescript_var_set_rawbits(surgescript_heap_at(heap, IDX_ADDR), idx);
    
    /* misc */
    update_music(object);

    /* done */
    return NULL;
}

/* spawn new object as a child of Level: prevent garbage collection */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* child_name = surgescript_var_fast_get_string(param[0]);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);

    /* spawn the new object */
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t child = surgescript_objectmanager_spawn(manager, me, child_name, NULL);

    /* must the new object be an entity? */
    /* well, no... setup objects may not be entities */
    /*if(1 || surgescript_tagsystem_has_tag(tag_system, child_name, "entity"))*/

    /* store its reference, so it won't be Garbage Collected */
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_heapptr_t ptr = surgescript_heap_malloc(heap);
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, ptr), child);
    surgescript_var_set_rawbits(surgescript_heap_at(heap, IDX_ADDR), 1 + IDX_ADDR); /* scan for broken references */

    /* done! */
    return surgescript_var_set_objecthandle(surgescript_var_create(), child);
}

/* spawn an entity at a certain position in world coordinates */
surgescript_var_t* fun_spawnentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_tagsystem_t* tag_system = surgescript_objectmanager_tagsystem(manager);
    const char* entity_name = surgescript_var_fast_get_string(param[0]);

    /* sanity check */
    if(surgescript_tagsystem_has_tag(tag_system, entity_name, "entity")) {
        surgescript_objecthandle_t pos = surgescript_var_get_objecthandle(param[1]);
        surgescript_object_t* v2 = surgescript_objectmanager_get(manager, pos);

        /* spawn entity */
        surgescript_var_t* ret = fun_spawn(object, param, 1);
        if(ret != NULL) {
            surgescript_objecthandle_t child = surgescript_var_get_objecthandle(ret);
            surgescript_var_destroy(ret);

            /* position entity */
            scripting_util_set_world_position(
                surgescript_objectmanager_get(manager, child),
                scripting_vector2_to_v2d(v2)
            );

            /* done */
            return surgescript_var_set_objecthandle(surgescript_var_create(), child);
        }
    }

    scripting_error(object, "%s.spawnEntity() requires object \"%s\" to be an entity.", surgescript_object_name(object), entity_name);
    return NULL;
}

/* can't destroy this object */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* the music of the level */
surgescript_var_t* fun_getmusic(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, MUSIC_ADDR));
}

/* the y-coordinate of the water, in pixels */
surgescript_var_t* fun_getwaterlevel(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), level_waterlevel());
}

/* set the y-coordinate of the water, in pixels */
surgescript_var_t* fun_setwaterlevel(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int waterlevel = (int)surgescript_var_get_number(param[0]);
    level_set_waterlevel(waterlevel);
    return NULL;
}

/* get the spawn point, a Vector2 */
surgescript_var_t* fun_getspawnpoint(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t spawnpoint = surgescript_var_get_objecthandle(surgescript_heap_at(heap, SPAWNPOINT_ADDR));
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, spawnpoint);

    /* update data, as the spawn point may have been changed inside the engine */
    v2d_t sp = level_spawnpoint();
    scripting_vector2_update(v2, sp.x, sp.y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), spawnpoint);
}

/* set the spawn point */
surgescript_var_t* fun_setspawnpoint(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, handle);

    level_set_spawnpoint(scripting_vector2_to_v2d(v2));
    level_save_state(); /* if we don't save the state, changing the spawn point means nothing */

    return NULL;
}

/* get the background path currently in use */
surgescript_var_t* fun_getbackground(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* path = background_filepath(level_background());
    return surgescript_var_set_string(surgescript_var_create(), path);
}

/* change the background */
surgescript_var_t* fun_setbackground(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* path = surgescript_var_fast_get_string(param[0]);
    level_change_background(path);
    return NULL;
}

/* get the original background of the level */
surgescript_var_t* fun_getbgtheme(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* path = level_bgtheme();
    return surgescript_var_set_string(surgescript_var_create(), path);
}

/* get the number of the next level in the current quest (1: first level, 2: second level, and so on) */
surgescript_var_t* fun_getnext(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int next = quest_next_level() + 1; /* the engine counts from zero */
    return surgescript_var_set_number(surgescript_var_create(), next);
}

/* set the next level in the current quest, identified by a number (1: first level, 2: second level, etc.) */
surgescript_var_t* fun_setnext(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int next = max(1, surgescript_var_get_number(param[0]));
    quest_set_next_level(next - 1);
    return NULL;
}

/* gets onUnload, a functor called when unloading the level */
surgescript_var_t* fun_getonunload(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, UNLOADFUNCTOR_ADDR));
}

/* sets onUnload, a functor called when unloading the level */
surgescript_var_t* fun_setonunload(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* onunload = surgescript_heap_at(heap, UNLOADFUNCTOR_ADDR);
    surgescript_var_copy(onunload, param[0]);
    return NULL;
}

/* will be true if the level has been cleared (will show the cleared animation) */
surgescript_var_t* fun_getcleared(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_bool(surgescript_var_create(), level_has_been_cleared());
}

/* the name of the level */
surgescript_var_t* fun_getname(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_string(surgescript_var_create(), level_name());
}

/* the act of the level, like 1, 2, 3... */
surgescript_var_t* fun_getact(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), level_act());
}

/* the version of the level, defined in the .lev file */
surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_string(surgescript_var_create(), level_version());
}

/* the author of the level, defined in the .lev file */
surgescript_var_t* fun_getauthor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_string(surgescript_var_create(), level_author());
}

/* the license of the level, defined in the .lev file */
surgescript_var_t* fun_getlicense(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_string(surgescript_var_create(), level_license());
}

/* the relative filepath of the level */
surgescript_var_t* fun_getfile(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_string(surgescript_var_create(), level_file());
}

/* level gravity in px/s^s */
surgescript_var_t* fun_getgravity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO: grab value from the physics engine */
    return surgescript_var_set_number(surgescript_var_create(), 828.0);
}

/* level time, in seconds */
surgescript_var_t* fun_gettime(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), level_time());
}

/* clears the level (will show the level cleared animation) */
surgescript_var_t* fun_clear(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    level_clear(NULL);
    return NULL;
}

/* restarts the current level */
surgescript_var_t* fun_restart(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    level_restart();
    return NULL;
}

/* prompts the user to see if he/she wants to quit the level */
surgescript_var_t* fun_quit(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    level_ask_to_leave();
    return NULL;
}

/* quit the level, without prompting the user */
surgescript_var_t* fun_abort(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    level_abort();
    return NULL;
}

/* pauses the game. Note: the game will not be paused if one of the players is dying */
surgescript_var_t* fun_pause(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    level_pause();
    return NULL;
}

/* loads the specified level.
   You may also pass the path to a quest; then the specified quest will be loaded, and
   when it's completed, the system will make you go back to the level you were before. */
surgescript_var_t* fun_load(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* filepath = surgescript_var_get_string(param[0], manager);
    const char* ext = strrchr(filepath, '.');
    bool is_quest = (ext != NULL && str_icmp(ext, ".qst") == 0);

    if(!is_quest)
        level_change(filepath);
    else
        level_push_quest(filepath);

    ssfree(filepath);
    return NULL;
}

/* loads the next level in the quest */
surgescript_var_t* fun_loadnext(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    level_jump_to_next_stage();
    return NULL;
}

/* get an entity given its ID in the .lev file;
   it's recommended to cache the return value of this function */
surgescript_var_t* fun_entity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* entity_id = surgescript_var_fast_get_string(param[0]);
    surgescript_object_t* entity = level_get_entity_by_id(entity_id);

    if(entity != NULL) {
        surgescript_objecthandle_t handle = surgescript_object_handle(entity);
        return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
    }
    else
        return NULL;
}

/* Level.setup(config): configure level entities using a config Dictionary */
surgescript_var_t* fun_setup(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, SETUP_ADDR));
    surgescript_object_t* setup = surgescript_objectmanager_get(manager, handle);

    /* call setup.call(config) */
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_object_call_function(setup, "call", param, num_params, ret);
    return ret;
}

/* this function gets called when the level is unloaded */
surgescript_var_t* fun_callunloadfunctor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* onunload = surgescript_heap_at(heap, UNLOADFUNCTOR_ADDR);

    /* we require Level.onUnload to be an existing function object;
       otherwise, do nothing */
    if(surgescript_var_is_objecthandle(onunload)) {
        surgescript_objectmanager_t* manager = surgescript_object_manager(object);
        surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(onunload);
        if(surgescript_objectmanager_exists(manager, handle)) {
            surgescript_object_t* functor = surgescript_objectmanager_get(manager, handle);
            if(surgescript_object_has_function(functor, "call"))
                surgescript_object_call_function(functor, "call", NULL, 0, NULL);
        }
    }

    return NULL;
}



/* -- utils -- */

/* updates the reference to Level.music */
void update_music(surgescript_object_t* object)
{
    music_t* music = level_music(); /* we'd like this to be not-NULL, but it may not be the case (i.e., no level music specified) */
    surgescript_heap_t* heap = surgescript_object_heap(object);

    if(surgescript_var_is_null(surgescript_heap_at(heap, MUSIC_ADDR)) ||
        (music != NULL && music != scripting_music_ptr(
            surgescript_objectmanager_get(surgescript_object_manager(object),
                surgescript_var_get_objecthandle(surgescript_heap_at(heap, MUSIC_ADDR))
            )
        ))
    ) {
        surgescript_object_t* music_component = scripting_util_get_component(
            scripting_util_surgeengine_component(surgescript_vm(), "Audio"), "Music"
        );
        surgescript_var_t* tmp[] = {
            surgescript_var_create(), surgescript_var_create(), surgescript_var_create()
        };

        surgescript_var_set_objecthandle(tmp[0], surgescript_object_handle(object));
        surgescript_var_set_string(tmp[1], music != NULL ? music_path(music) : "");
        surgescript_object_call_function(music_component, "__spawn", (const surgescript_var_t**)tmp, 2, tmp[2]);
        surgescript_var_set_objecthandle(surgescript_heap_at(heap, MUSIC_ADDR), surgescript_var_get_objecthandle(tmp[2]));

        surgescript_var_destroy(tmp[2]);
        surgescript_var_destroy(tmp[1]);
        surgescript_var_destroy(tmp[0]);
    }
}

/* SurgeScript code */
static const char code_in_surgescript[] = "\
using SurgeEngine.Level; \n\
\n\
object 'Setup Level' \n\
{ \n\
    state 'main' \n\
    { \n\
    } \n\
\n\
    fun call(config) \n\
    { \n\
        config = config || { }; \n\
        entities = { }; \n\
\n\
        foreach(entry in config) { \n\
            if(Level.entity(entry.key) == null) { \n\
                objs = Level.findObjects(entry.key); \n\
                for(i = 0; i < objs.length; i++) \n\
                    setup(objs[i], entry.value); \n\
            } \n\
            else \n\
                entities[entry.key] = entry.value; \n\
        } \n\
 \n\
        foreach(entry in entities) { \n\
            obj = Level.entity(entry.key); \n\
            setup(obj, entry.value); \n\
        } \n\
    } \n\
 \n\
    fun setup(obj, properties) \n\
    { \n\
        foreach(entry in properties) { \n\
            if(obj.hasFunction('set_' + entry.key)) \n\
                obj.__invoke('set_' + entry.key, [ entry.value ]); \n\
        } \n\
    } \n\
} \n\
";
