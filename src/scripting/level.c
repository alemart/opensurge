/*
 * Open Surge Engine
 * level.c - scripting system: Level object
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
#include <string.h>
#include "scripting.h"
#include "../core/util.h"
#include "../core/audio.h"
#include "../core/video.h"
#include "../core/timer.h"
#include "../core/stringutil.h"
#include "../core/darray.h"
#include "../scenes/level.h"
#include "../scenes/quest.h"
#include "../entities/player.h"
#include "../entities/background.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
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
static surgescript_var_t* fun_settime(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_clear(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_restart(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_quit(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_abort(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_pause(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_load(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_loadnext(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_loadandreturn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_entity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_entityid(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_findentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_findentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_activeentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setup(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getnext(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setnext(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_get_debugmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_set_debugmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getonunload(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setonunload(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_callunloadfunctor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_releasechildren(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_storereference(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_child(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_children(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_childwithtag(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_childrenwithtag(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t MUSIC_ADDR = 0; /* Level.music */
static const surgescript_heapptr_t SPAWNPOINT_ADDR = 1; /* Level.spawnpoint */
static const surgescript_heapptr_t SETUP_ADDR = 2; /* object "Setup Level" */
static const surgescript_heapptr_t UNLOADFUNCTOR_ADDR = 3; /* Level.onUnload functor */
static const surgescript_heapptr_t TIME_ADDR = 4; /* Level.time */
static const surgescript_heapptr_t ISDEBUGGING_ADDR = 5; /* Debug Mode on/off flag (fast access) */
static const surgescript_heapptr_t STORAGE_ADDR = 6; /* LevelStorage object */
static const surgescript_heapptr_t ENTITYMANAGER_ADDR = 7; /* EntityManager */
#define LAST_ADDR ENTITYMANAGER_ADDR /* must be an alias to the last address */
#define WANT_BACKWARDS_COMPATIBLE_CHILD_FUNCTION 1 /* backwards compatibility with engine versions 0.5.x - 0.6.0.x */
static const char DEBUG_MODE_OBJECT_NAME[] = "Debug Mode";
static const char code_in_surgescript[];
static void update_music(surgescript_object_t* object);
static void update_time(surgescript_object_t* object);
static inline surgescript_object_t* get_entity_manager(const surgescript_object_t* object);

/*
 * scripting_register_level()
 * Register the Level object
 */
void scripting_register_level(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Level", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Level", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Level", "destructor", fun_destructor, 0);
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
    surgescript_vm_bind(vm, "Level", "set_time", fun_settime, 1);
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
    surgescript_vm_bind(vm, "Level", "loadAndReturn", fun_loadandreturn, 1);
    surgescript_vm_bind(vm, "Level", "entity", fun_entity, 1);
    surgescript_vm_bind(vm, "Level", "entityId", fun_entityid, 1);
    surgescript_vm_bind(vm, "Level", "findEntity", fun_findentity, 1);
    surgescript_vm_bind(vm, "Level", "findEntities", fun_findentities, 1);
    surgescript_vm_bind(vm, "Level", "activeEntities", fun_activeentities, 0);
    surgescript_vm_bind(vm, "Level", "setup", fun_setup, 1);
    surgescript_vm_bind(vm, "Level", "get_debugMode", fun_get_debugmode, 0);
    surgescript_vm_bind(vm, "Level", "set_debugMode", fun_set_debugmode, 1);
    surgescript_vm_bind(vm, "Level", "__callUnloadFunctor", fun_callunloadfunctor, 0);
    surgescript_vm_bind(vm, "Level", "__releaseChildren", fun_releasechildren, 0);
    surgescript_vm_bind(vm, "Level", "__storeReference", fun_storereference, 1);
    surgescript_vm_bind(vm, "Level", "child", fun_child, 1);
    surgescript_vm_bind(vm, "Level", "children", fun_children, 1);
    surgescript_vm_bind(vm, "Level", "childWithTag", fun_childwithtag, 1);
    surgescript_vm_bind(vm, "Level", "childrenWithTag", fun_childrenwithtag, 1);
    surgescript_vm_compile_code_in_memory(vm, code_in_surgescript);
}

/*
 * scripting_level_entitymanager()
 * Get the SurgeScript EntityManager
 */
surgescript_object_t* scripting_level_entitymanager(const surgescript_object_t* object)
{
    return get_entity_manager(object);
}



/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t spawnpoint = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
    surgescript_objecthandle_t setup = surgescript_objectmanager_spawn(manager, me, "Setup Level", NULL);
    surgescript_objecthandle_t storage = surgescript_objectmanager_spawn(manager, me, "LevelStorage", NULL);
    surgescript_objecthandle_t entity_manager = surgescript_objectmanager_spawn(manager, me, "EntityManager", NULL);

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

    /* Level time */
    ssassert(TIME_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_number(surgescript_heap_at(heap, TIME_ADDR), 0.0);

    /* Fast-access Debug Mode on/off flag */
    ssassert(ISDEBUGGING_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_bool(surgescript_heap_at(heap, ISDEBUGGING_ADDR), false);

    /* LevelStorage */
    ssassert(STORAGE_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, STORAGE_ADDR), storage);

    /* EntityManager */
    ssassert(ENTITYMANAGER_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, ENTITYMANAGER_ADDR), entity_manager);

    /* store the pointer to the EntityManager for fast access */
    surgescript_object_t* entity_manager_ptr = surgescript_objectmanager_get(manager, entity_manager);
    surgescript_object_set_userdata(object, entity_manager_ptr);

    /* done */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* clear the pointer to the EntityManager */
    surgescript_object_set_userdata(object, NULL);

    /* done! */
    return NULL;
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* entity_manager = get_entity_manager(object);

    /* internal updates */
    update_music(object);
    update_time(object);

    /* update entities */
    surgescript_object_call_function(entity_manager, "update", NULL, 0, NULL);

    /* done! */
    return NULL;
}

/* spawn new object as a child of Level: prevent garbage collection */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const char* child_name = surgescript_var_fast_get_string(param[0]);

    /* must the new object be an entity? */
    /* well, no... setup objects may not be entities */

    /* is the new object an entity? call Level.spawnEntity() instead */
    surgescript_tagsystem_t* tag_system = surgescript_objectmanager_tagsystem(manager);
    if(surgescript_tagsystem_has_tag(tag_system, child_name, "entity")) {
        surgescript_objecthandle_t v2_handle = surgescript_objectmanager_spawn_temp(manager, "Vector2");
        surgescript_object_t* v2 = surgescript_objectmanager_get(manager, v2_handle);
        scripting_vector2_update(v2, 0.0, 0.0); /* v2 = Vector2(0,0) */

        /* delegate to Level.spawnEntity() */
        surgescript_var_t* ret = surgescript_var_create();
        surgescript_var_t* v2_var = surgescript_var_set_objecthandle(surgescript_var_create(), v2_handle);

        const surgescript_var_t* args[] = { param[0], v2_var };
        surgescript_object_call_function(object, "spawnEntity", args, 2, ret);
        surgescript_objecthandle_t child = surgescript_var_get_objecthandle(ret);

        surgescript_var_destroy(v2_var);
        surgescript_var_destroy(ret);

        /* show warning: since it's delegated, the parent object won't be Level */
        video_showmessage("Use %s.spawnEntity() to spawn \"%s\"", surgescript_object_name(object), child_name);

        /* done */
        surgescript_object_kill(v2);
        return surgescript_var_set_objecthandle(surgescript_var_create(), child);
    }

    /* The new object isn't an entity.
       What if a descendant of that new object is an entity?

       In this case we don't support it. It will behave just like
       any other SurgeScript object and it will not be handled by
       the Entity Manager.

       We don't traverse the sub-tree to find possible entities,
       as that case is unusual and this operation is expensive. */

    /* spawn the new object */
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t child = surgescript_objectmanager_spawn(manager, me, child_name, NULL);

    /* store its reference, so it won't be Garbage Collected */
    surgescript_var_t* arg = surgescript_var_set_objecthandle(surgescript_var_create(), child);
    const surgescript_var_t* args[] = { arg };
    surgescript_object_call_function(object, "__storeReference", args, 1, NULL);
    surgescript_var_destroy(arg);

    /* done! */
    return surgescript_var_set_objecthandle(surgescript_var_create(), child);
}

/* spawn an entity at a certain position in world coordinates */
surgescript_var_t* fun_spawnentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_var_t* new_entity_var = surgescript_var_create();

    /* get the entityManager */
    surgescript_var_t* entity_manager_handle_var = surgescript_heap_at(heap, ENTITYMANAGER_ADDR);
    surgescript_objecthandle_t entity_manager_handle = surgescript_var_get_objecthandle(entity_manager_handle_var);
    surgescript_object_t* entity_manager = surgescript_objectmanager_get(manager, entity_manager_handle);

    /* delegate to entityManager.spawnEntity() */
    surgescript_object_call_function(entity_manager, "spawnEntity", param, 2, new_entity_var);

    /* store the reference to the new entity, so it won't be Garbage Collected */
    const surgescript_var_t* args[] = { new_entity_var };
    surgescript_object_call_function(object, "__storeReference", args, 1, NULL);

    /* done! */
    return new_entity_var;
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
    return surgescript_var_set_number(surgescript_var_create(), level_gravity());
}

/* get elapsed time in the level, given in seconds */
surgescript_var_t* fun_gettime(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* Usage of level_time() is obsolete. It's better to keep track of time independently.
       In this way, we can make Level.time a read-write property */
    /*return surgescript_var_set_number(surgescript_var_create(), level_time());*/

    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, TIME_ADDR));
}

/* set elapsed time in the level, given in seconds */
surgescript_var_t* fun_settime(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* level_time = surgescript_heap_at(heap, TIME_ADDR);

    double elapsed_time = surgescript_var_get_number(param[0]);
    surgescript_var_set_number(level_time, max(elapsed_time, 0.0));

    return NULL;
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
   You may also pass the path to a quest; then that quest will be loaded, and when it's
   completed or aborted, the system will make you go back to the level you were before. */
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

/* loads the specified level and returns to the level you were before after you quit
   or exit the loaded level. You may pass the path to a level or to a quest file. If
   you pass a quest file, this function behaves exactly like fun_load() */
surgescript_var_t* fun_loadandreturn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* filepath = surgescript_var_get_string(param[0], manager);

    /* a single .lev file implicitly defines a single-level quest */
    level_push_quest(filepath);

    ssfree(filepath);
    return NULL;
}

/* get an entity given its ID in the .lev file; returns null if not found */
surgescript_var_t* fun_entity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* entity_manager = get_entity_manager(object);

    /* delegate to the entity manager */
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_object_call_function(entity_manager, "entity", param, 1, ret);

    /* done! */
    return ret;
}

/* get the .lev file ID of the given entity. If no such ID exists, an empty string is returned */
surgescript_var_t* fun_entityid(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* entity_manager = get_entity_manager(object);

    /* delegate to the entity manager */
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_object_call_function(entity_manager, "entity", param, 1, ret);

    /* done! */
    return ret;
}

/* find by name an entity that was spawned with this.spawnEntity() */
surgescript_var_t* fun_findentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* entity_manager = get_entity_manager(object);

    /* delegate to the entity manager */
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_object_call_function(entity_manager, "findEntity", param, 1, ret);

    /* done! */
    return ret;
}

/* find all entities with the given name that were spawned with this.spawnEntity() */
surgescript_var_t* fun_findentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* entity_manager = get_entity_manager(object);

    /* delegate to the entity manager */
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_object_call_function(entity_manager, "findEntities", param, 1, ret);

    /* done! */
    return ret;
}

/* get active entities: those that are inside the region of interest, as well as the awake (and detached) ones */
surgescript_var_t* fun_activeentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* entity_manager = get_entity_manager(object);

    /* delegate to the entity manager */
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_object_call_function(entity_manager, "activeEntities", NULL, 0, ret);

    /* done! */
    return ret;
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

/* is the Debug Mode activated? */
surgescript_var_t* fun_get_debugmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* this method must be fast! It's used often. */
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* flag = surgescript_heap_at(heap, ISDEBUGGING_ADDR);

    /* we just read a flag */
    return surgescript_var_clone(flag);
}

/* enable/disable the Debug Mode */
surgescript_var_t* fun_set_debugmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* flag = surgescript_heap_at(heap, ISDEBUGGING_ADDR);
    bool new_value = surgescript_var_get_bool(param[0]);
    bool old_value = surgescript_var_get_bool(flag);

#if 1
    /* skip - nothing to do */
    if(new_value == old_value)
        return NULL;
#endif

    /* update the flag */
    surgescript_var_set_bool(flag, new_value);

    /* enter or exit the Debug Mode */
    const surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t debug_mode_handle = surgescript_object_child(object, DEBUG_MODE_OBJECT_NAME);
    surgescript_objecthandle_t null = surgescript_objectmanager_null(manager);

    bool want_debug_mode = new_value;
    if(want_debug_mode) {
        if(debug_mode_handle == null) {

            /* enter the Debug Mode: call this.spawn("Debug Mode") */
            surgescript_var_t* debug_mode_name = surgescript_var_set_string(surgescript_var_create(), DEBUG_MODE_OBJECT_NAME);
            const surgescript_var_t* spawn_param[] = { debug_mode_name };
            surgescript_object_call_function(object, "spawn", spawn_param, 1, NULL);
            surgescript_var_destroy(debug_mode_name);

        }
    }
    else {
        if(debug_mode_handle != null) {

            /* exit the Debug Mode: call debug.exit() */
            surgescript_object_t* debug_mode = surgescript_objectmanager_get(manager, debug_mode_handle);
            surgescript_object_call_function(debug_mode, "exit", NULL, 0, NULL);

        }
    }

    /* done! */
    return NULL;
}

/* this function gets called when the level is unloaded */
surgescript_var_t* fun_callunloadfunctor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* onunload = surgescript_heap_at(heap, UNLOADFUNCTOR_ADDR);

    /* we require Level.onUnload to be an existing function object;
       if it's not, do nothing */
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

/* release all children, which will call their destructors on the next update cycle */
surgescript_var_t* fun_releasechildren(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    DARRAY(surgescript_objecthandle_t, handles);
    darray_init(handles);

    /* call entityManager.__releaseChildren() */
    surgescript_var_t* entity_manager_var = surgescript_heap_at(heap, ENTITYMANAGER_ADDR);
    surgescript_objecthandle_t entity_manager_handle = surgescript_var_get_objecthandle(entity_manager_var);
    surgescript_object_t* entity_manager = surgescript_objectmanager_get(manager, entity_manager_handle);
    surgescript_object_call_function(entity_manager, "__releaseChildren", NULL, 0, NULL);

    /* for each child of Level */
    int child_count = surgescript_object_child_count(object);
    for(int i = child_count - 1; i >= 0; i--) {
        surgescript_objecthandle_t child_handle = surgescript_object_nth_child(object, i);

        /* is the child a built-in child of Level? TODO make this more efficient? */
        bool is_builtin = false;
        for(surgescript_heapptr_t j = 0; j <= LAST_ADDR; j++) { /* maybe the compiler unrolls this little loop? */
            const surgescript_var_t* builtin_var = surgescript_heap_at(heap, j);
            surgescript_objecthandle_t builtin_handle = surgescript_var_get_objecthandle(builtin_var);
            is_builtin = is_builtin || (child_handle == builtin_handle /*&& surgescript_var_is_objecthandle(builtin_var)*/);
        }

        /* the child is not a built-in */
        if(!is_builtin) {
            darray_push(handles, child_handle);
        }
    }

    /* release children immediately and call their destructors (if any) */
    for(int j = 0; j < darray_length(handles); j++) {
        surgescript_objecthandle_t child_handle = handles[j];
        surgescript_object_t* child = surgescript_objectmanager_get(manager, child_handle);
        surgescript_object_kill(child);
        surgescript_objectmanager_delete(manager, child_handle); /* release immediately */
    }

    /* done */
    darray_release(handles);
    return NULL;
}

/* store a reference to the SurgeScript object passed as a parameter, so it won't be Garbage Collected */
surgescript_var_t* fun_storereference(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t reference = surgescript_var_get_objecthandle(param[0]); /* the reference to be stored */

    /* get the LevelStorage object */
    surgescript_var_t* storage_var = surgescript_heap_at(heap, STORAGE_ADDR);
    surgescript_objecthandle_t storage_handle = surgescript_var_get_objecthandle(storage_var);
    surgescript_object_t* storage = surgescript_objectmanager_get(manager, storage_handle);

    /* call LevelStorage.storeReference() */
    surgescript_var_t* arg = surgescript_var_set_objecthandle(surgescript_var_create(), reference); /* we make sure it's an object handle */
    const surgescript_var_t* args[] = { arg };
    surgescript_object_call_function(storage, "storeReference", args, 1, NULL);
    surgescript_var_destroy(arg);

    /* done! */
    return NULL;
}

/* override Object.child() for backwards compatibility */
surgescript_var_t* fun_child(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_var_t* ret = surgescript_var_create();

    /* call method of the super class */
    surgescript_object_call_super_function(object, "child", param, num_params, ret);

#if WANT_BACKWARDS_COMPATIBLE_CHILD_FUNCTION

    /* in Open Surge 0.5.x - 0.6.0.x, entities were direct children of Level.
       That is no longer the case. We can look for entities in this function
       to preserve backwards compatibility. However, we're making it return
       possibly "incorrect" results (strictly speaking), as it isn't supposed
       to return anything other than direct children of this object */

    /* make sure that the method of the super class didn't find anything */
    if(surgescript_var_is_null(ret)) {

        surgescript_objectmanager_t* manager = surgescript_object_manager(object);
        surgescript_tagsystem_t* tag_system = surgescript_objectmanager_tagsystem(manager);
        const char* object_name = surgescript_var_fast_get_string(param[0]);

        /* run backwards compatible code only if the requested object is an entity */
        if(surgescript_tagsystem_has_tag(tag_system, object_name, "entity")) {

            /* call this.findEntity() */
            surgescript_object_call_function(object, "findEntity", param, num_params, ret);

        }

    }

#endif

    /* done! */
    return ret;
}

/* override Object.children() for backwards compatibility */
surgescript_var_t* fun_children(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_var_t* ret = surgescript_var_create();

    /* call method of the super class */
    surgescript_object_call_super_function(object, "children", param, num_params, ret);

#if WANT_BACKWARDS_COMPATIBLE_CHILD_FUNCTION

    /* in Open Surge 0.5.x - 0.6.0.x, entities were direct children of Level.
       That is no longer the case. We can look for entities in this function
       to preserve backwards compatibility. However, we're making it return
       possibly "incorrect" results (strictly speaking), as it isn't supposed
       to return anything other than direct children of this object */

    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_tagsystem_t* tag_system = surgescript_objectmanager_tagsystem(manager);
    const char* object_name = surgescript_var_fast_get_string(param[0]);

    /* run backwards compatible code only if the requested object is an entity */
    if(surgescript_tagsystem_has_tag(tag_system, object_name, "entity")) {

        /* get the length of the array returned by the method of the super class */
        surgescript_objecthandle_t array_handle = surgescript_var_get_objecthandle(ret);
        surgescript_object_t* array = surgescript_objectmanager_get(manager, array_handle);
        surgescript_var_t* array_length_var = surgescript_var_create();
        surgescript_object_call_function(array, "get_length", NULL, 0, array_length_var);
        int array_length = (int)(surgescript_var_get_number(array_length_var));
        surgescript_var_destroy(array_length_var);

        /* just make sure that we've got no direct children (this should be the case) */
        if(array_length == 0) {

            /* we'll no longer need the previous array */
            surgescript_object_kill(array);

            /* call this.findEntity() */
            surgescript_object_call_function(object, "findEntities", param, num_params, ret);

        }

    }

#endif

    /* done! */
    return ret;
}

/* override Object.childWithTag() for backwards compatibility */
surgescript_var_t* fun_childwithtag(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_var_t* ret = surgescript_var_create();

    /* call method of the super class */
    surgescript_object_call_super_function(object, "childWithTag", param, num_params, ret);

#if WANT_BACKWARDS_COMPATIBLE_CHILD_FUNCTION
#if 1
    /* not worthwhile? keep the correct behavior */
#else
    /* slow fallback that breaks the expected behavior */
    if(surgescript_var_is_null(ret)) {
        const char* tag_name = surgescript_var_fast_get_string(param[0]);
        if(strcmp(tag_name, "entity") == 0) {
            /* call entityManager.findObjectWithTag("entity") */
            surgescript_object_t* entity_manager = get_entity_manager(object);
            surgescript_object_call_function(entity_manager, "findObjectWithTag", param, num_params, ret);
        }
    }
#endif
#endif

    /* done! */
    return ret;
}

/* override Object.childrenWithTag() for backwards compatibility */
surgescript_var_t* fun_childrenwithtag(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_var_t* ret = surgescript_var_create();

    /* call method of the super class */
    surgescript_object_call_super_function(object, "childrenWithTag", param, num_params, ret);

#if WANT_BACKWARDS_COMPATIBLE_CHILD_FUNCTION
#if 1
    /* not worthwhile? keep the correct behavior */
#else
    /* slow fallback that breaks the expected behavior */
    const char* tag_name = surgescript_var_fast_get_string(param[0]);
    if(strcmp(tag_name, "entity") == 0) {
        /* call entityManager.findObjectsWithTag("entity") */
        surgescript_object_t* entity_manager = get_entity_manager(object);
        surgescript_object_call_function(entity_manager, "findObjectsWithTag", param, num_params, ret);
    }
#endif
#endif

    /* done! */
    return ret;
}


/* -- utils -- */

/* get the EntityManager */
surgescript_object_t* get_entity_manager(const surgescript_object_t* object)
{
#if 1
    return (surgescript_object_t*)surgescript_object_userdata(object);
#else
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);

    surgescript_var_t* entity_manager_var = surgescript_heap_at(heap, ENTITYMANAGER_ADDR);
    surgescript_objecthandle_t entity_manager_handle = surgescript_var_get_objecthandle(entity_manager_var);
    surgescript_object_t* entity_manager = surgescript_objectmanager_get(manager, entity_manager_handle);

    return entity_manager;
#endif
}

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

/* update the elapsed Level.time */
void update_time(surgescript_object_t* object)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* level_time = surgescript_heap_at(heap, TIME_ADDR);

    double elapsed_time = surgescript_var_get_number(level_time);
    elapsed_time += timer_get_delta();

    surgescript_var_set_number(level_time, elapsed_time);
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
