/*
 * Open Surge Engine
 * level.c - scripting system: Level object
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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
#include "../core/logfile.h"
#include "../core/audio.h"
#include "../core/video.h"
#include "../core/timer.h"
#include "../util/darray.h"
#include "../util/util.h"
#include "../util/stringutil.h"
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
static surgescript_var_t* fun_setact(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
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
static surgescript_var_t* fun_undo_clear(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
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
static surgescript_var_t* fun_onload(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_onunload(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_callunloadfunctor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_unload(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getplayermanager(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_registersetupobjectname(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawnsetupobjects(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawnassetupobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t MUSIC_ADDR = 0; /* Level.music */
static const surgescript_heapptr_t SPAWNPOINT_ADDR = 1; /* Level.spawnpoint */
static const surgescript_heapptr_t SETUPFUNCTOR_ADDR = 2; /* object "LevelSetupFunctor" */
static const surgescript_heapptr_t UNLOADFUNCTOR_ADDR = 3; /* Level.onUnload functor */
static const surgescript_heapptr_t TIME_ADDR = 4; /* Level.time */
static const surgescript_heapptr_t CONTAINER_ADDR = 5; /* LevelObjectContainer */
static const surgescript_heapptr_t ENTITYMANAGER_ADDR = 6; /* EntityManager */
static const surgescript_heapptr_t PLAYERMANAGER_ADDR = 7; /* PlayerManager */
#define LAST_ADDR PLAYERMANAGER_ADDR /* must be an alias to the last address */
static const char code_in_surgescript[];

/* level info */
typedef struct levelinfo_t levelinfo_t;
struct levelinfo_t {

    /* quick reference to the EntityManager */
    surgescript_object_t* entity_manager;

    /* setup objects */
    bool can_spawn_setup_objects; /* a lock that indicates whether or not we can spawn setup objects */
    DARRAY(char*, setup_object_name); /* names of setup objects */

};

/* helpers */
const char* DEFAULT_SETUP_OBJECTS[] = { "Default Setup" }; /* for backwards compatibility */
static levelinfo_t* level_info_ctor(surgescript_object_t* entity_manager);
static levelinfo_t* level_info_dtor(levelinfo_t* level_info);
static inline levelinfo_t* get_level_info(const surgescript_object_t* level);
static inline surgescript_object_t* get_entity_manager(const surgescript_object_t* level);
static inline surgescript_object_t* get_container(const surgescript_object_t* level);
static void update_music(surgescript_object_t* object);
static void update_time(surgescript_object_t* object);
static void warn_about_entity_descendant(surgescript_objecthandle_t entity_handle, void* data);
static inline bool is_in_debug_mode(const surgescript_object_t* level);
static inline void pause_containers(const surgescript_object_t* level, bool pause);



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
    surgescript_vm_bind(vm, "Level", "set_act", fun_setact, 1);
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
    surgescript_vm_bind(vm, "Level", "undoClear", fun_undo_clear, 0);
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
    surgescript_vm_bind(vm, "Level", "__onLoad", fun_onload, 0);
    surgescript_vm_bind(vm, "Level", "__onUnload", fun_onunload, 0);
    surgescript_vm_bind(vm, "Level", "__callUnloadFunctor", fun_callunloadfunctor, 0);
    surgescript_vm_bind(vm, "Level", "__releaseChildren", fun_unload, 0);
    surgescript_vm_bind(vm, "Level", "get___playerManager", fun_getplayermanager, 0);
    surgescript_vm_bind(vm, "Level", "__registerSetupObjectName", fun_registersetupobjectname, 1);
    surgescript_vm_bind(vm, "Level", "__spawnSetupObjects", fun_spawnsetupobjects, 0);
    surgescript_vm_bind(vm, "Level", "__spawnAsSetupObject", fun_spawnassetupobject, 1);

    surgescript_vm_compile_code_in_memory(vm, code_in_surgescript);
}

/*
 * scripting_level_entitymanager()
 * Get the SurgeScript EntityManager
 */
surgescript_object_t* scripting_level_entitymanager(const surgescript_object_t* level)
{
    return get_entity_manager(level);
}

/*
 * scripting_level_setupobjects_iterator()
 * Use it to iterate over the names of the level setup objects
 */
iterator_t* scripting_level_setupobjects_iterator(const surgescript_object_t* level)
{
    levelinfo_t* level_info = get_level_info(level);
    return darray_iterator(level_info->setup_object_name);
}

/*
 * scripting_level_issetupobjectname()
 * Checks if the given name is a Level setup object name
 */
bool scripting_level_issetupobjectname(const surgescript_object_t* level, const char* object_name)
{
    /* this method must be fast, as it's used often (i.e., when spawning entities) */
    const levelinfo_t* level_info = get_level_info(level);
    bool found = false;

    for(int i = darray_length(level_info->setup_object_name) - 1; i >= 0; i--) {
        if(0 == strcmp(level_info->setup_object_name[i], object_name)) {
            found = true;
            break;
        }
    }

    return found;
}




/*
 * SurgeScript API
 */

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t spawnpoint = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
    surgescript_objecthandle_t setup = surgescript_objectmanager_spawn(manager, me, "LevelSetupFunctor", NULL);
    surgescript_objecthandle_t container = surgescript_objectmanager_spawn(manager, me, "LevelObjectContainer", scripting_levelobjectcontainer_token());
    surgescript_objecthandle_t entity_manager = surgescript_objectmanager_spawn(manager, me, "EntityManager", NULL);
    surgescript_objecthandle_t player_manager = surgescript_objectmanager_spawn(manager, me, "PlayerManager", NULL);

    /* Level music */
    ssassert(MUSIC_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, MUSIC_ADDR));

    /* spawn point */
    ssassert(SPAWNPOINT_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, SPAWNPOINT_ADDR), spawnpoint);

    /* Setup functor */
    ssassert(SETUPFUNCTOR_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, SETUPFUNCTOR_ADDR), setup);

    /* Unload functor */
    ssassert(UNLOADFUNCTOR_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, UNLOADFUNCTOR_ADDR));

    /* Level time */
    ssassert(TIME_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_number(surgescript_heap_at(heap, TIME_ADDR), 0.0);

    /* LevelObjectContainer for non-entities */
    ssassert(CONTAINER_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, CONTAINER_ADDR), container);

    /* EntityManager - includes containers for entities */
    ssassert(ENTITYMANAGER_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, ENTITYMANAGER_ADDR), entity_manager);

    /* PlayerManager */
    ssassert(PLAYERMANAGER_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, PLAYERMANAGER_ADDR), player_manager);

    /* allocate a level info structure */
    surgescript_object_t* entity_manager_ptr = surgescript_objectmanager_get(manager, entity_manager);
    levelinfo_t* level_info = level_info_ctor(entity_manager_ptr);
    surgescript_object_set_userdata(object, level_info);

    /* done */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* deallocate the level info structure */
    levelinfo_t* level_info = get_level_info(object);
    level_info_dtor(level_info);
    surgescript_object_set_userdata(object, NULL);

    /* done! */
    return NULL;
}

/* called as soon as the level is loaded */
surgescript_var_t* fun_onload(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
#if 0
    const surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_object_t* entity_manager = get_entity_manager(object);
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };

    /* spawn the PlayerGroup in a container */
    surgescript_var_set_string(arg, "PlayerGroup");
    surgescript_object_call_function(object, "spawn", args, 1, ret);
    surgescript_var_copy(surgescript_heap_at(heap, PLAYERMANAGER_ADDR), ret);

    /* done */
    surgescript_var_destroy(arg);
    surgescript_var_destroy(ret);
#endif
    return NULL;
}

/* called as soon as the level is unloaded */
surgescript_var_t* fun_onunload(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_heap_t* heap = surgescript_object_heap(object);
    const surgescript_var_t* player_manager_handle_var = surgescript_heap_at(heap, PLAYERMANAGER_ADDR);
    surgescript_objecthandle_t player_manager_handle = surgescript_var_get_objecthandle(player_manager_handle_var);
    surgescript_object_t* player_manager = surgescript_objectmanager_get(manager, player_manager_handle);

    /* call Level.onUnload(), if applicable */
    surgescript_object_call_function(object, "__callUnloadFunctor", NULL, 0, NULL);

    /* unload the PlayerManager */
    surgescript_object_call_function(player_manager, "__unload", NULL, 0, NULL);

    /* release all user-added children of the Level, but not the Level object itself */
    surgescript_object_call_function(object, "__releaseChildren", NULL, 0, NULL);

    #if 0
    /*
    If we destroy the PlayerManager now, we can't get any Player instance in an
    object destructor when unloading the level. We get a crash.
    */
    surgescript_object_kill(player_manager);
    surgescript_var_set_null(surgescript_heap_at(heap, PLAYERMANAGER_ADDR));
    #endif

    /* done */
    return NULL;
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_heap_t* heap = surgescript_object_heap(object);
    bool debug_mode = is_in_debug_mode(object);

    /* update music */
    update_music(object);

    /* update Level.time */
    if(!level_has_been_cleared()) {
        const player_t* player = level_player();
        if(player != NULL && !player_is_dying(player))
            update_time(object);
    }

    /* pause the containers when in Debug Mode */
    if(debug_mode) {
        pause_containers(object, true);
    }
    else {
        pause_containers(object, false);
    }

    /* update built-ins */
    for(surgescript_heapptr_t ptr = 0; ptr <= LAST_ADDR; ptr++) {
        const surgescript_var_t* builtin_var = surgescript_heap_at(heap, ptr);
        if(surgescript_var_is_objecthandle(builtin_var)) {
            surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(builtin_var);
            surgescript_object_t* builtin = surgescript_objectmanager_get(manager, handle);

            surgescript_object_traverse_tree(builtin, surgescript_object_update);
        }
    }

    /* don't visit my children */
    surgescript_object_set_active(object, false); /* my parent will wake me up */
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

        /* show warning */
        if(!surgescript_tagsystem_has_tag(tag_system, child_name, "detached")) {
            /* sometimes we don't want a warning, as with Level.spawn("Fader");
               entity "Fader" is detached */
            video_showmessage("Use %s.spawnEntity() to spawn \"%s\"", surgescript_object_name(object), child_name);
        }

        /* done */
        surgescript_object_kill(v2);
        return surgescript_var_set_objecthandle(surgescript_var_create(), child);
    }
    else {
        /*

        The new object isn't an entity.
        What if a descendant of that new object is an entity?

        In this case we don't support it. It will behave just like
        any other SurgeScript object and it will not be handled by
        the Entity Manager.

        */

        /* spawn the new object */
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        surgescript_objecthandle_t child_handle = surgescript_objectmanager_spawn(manager, me, child_name, NULL);
        surgescript_object_t* child = surgescript_objectmanager_get(manager, child_handle);

        /* warn the user about spawning entities as descendants of non-entities that are children of Level */
        /*surgescript_object_find_tagged_descendants(child, "entity", child, warn_entity_descendant);*/ /* potentially expensive; can't take time into account */
        surgescript_object_tagged_children(child, "entity", child, warn_about_entity_descendant); /* cheap, reasonable approximation */

        /* add to the LevelObjectContainer */
        surgescript_object_t* container = get_container(object);
        surgescript_var_t* arg = surgescript_var_set_objecthandle(surgescript_var_create(), child_handle);
        const surgescript_var_t* args[] = { arg };
        surgescript_object_call_function(container, "addObject", args, 1, NULL);
        surgescript_var_destroy(arg);

        /* done! */
        return surgescript_var_set_objecthandle(surgescript_var_create(), child_handle);
    }
}

/* spawn an entity at a certain position in world coordinates */
surgescript_var_t* fun_spawnentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_var_t* new_entity_var = surgescript_var_create();

    /* delegate to entityManager.spawnEntity() */
    surgescript_object_t* entity_manager = get_entity_manager(object);
    surgescript_object_call_function(entity_manager, "spawnEntity", param, 2, new_entity_var);

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

/* will be true if the level has been cleared (i.e., Level.clear() was called).
   A Level Cleared animation is typically played when this flag is enabled */
surgescript_var_t* fun_getcleared(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_bool(surgescript_var_create(), level_has_been_cleared());
}

/* the name of the level */
surgescript_var_t* fun_getname(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_string(surgescript_var_create(), level_name());
}

/* the act number of the level. Typically 1, 2 or 3. */
surgescript_var_t* fun_getact(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), level_act());
}

/* change the act number of the level */
surgescript_var_t* fun_setact(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int new_act_number = surgescript_var_get_number(param[0]);

    level_set_act(new_act_number);
    return NULL;
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

/* clear the level: set the Level.cleared flag and disable player input */
surgescript_var_t* fun_clear(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    level_clear(NULL);
    return NULL;
}

/* undo a previous call to Level.clear() */
surgescript_var_t* fun_undo_clear(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    level_undo_clear();
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
    surgescript_object_call_function(entity_manager, "entityId", param, 1, ret);

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
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, SETUPFUNCTOR_ADDR));
    surgescript_object_t* setup = surgescript_objectmanager_get(manager, handle);

    /* validate */
    surgescript_objecthandle_t config_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* config = surgescript_objectmanager_get(manager, config_handle);
    if(0 != strcmp("Dictionary", surgescript_object_name(config))) {
        char* str = surgescript_var_get_string(param[0], manager);
        video_showmessage("Level.setup() expects a Dictionary, but received %s", str);
        ssfree(str);

        return surgescript_var_set_null(surgescript_var_create());
    }

    /* call setupFunctor.call(config) */
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_object_call_function(setup, "call", param, num_params, ret);
    return ret;
}

/* is the Debug Mode activated? */
surgescript_var_t* fun_get_debugmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* this method must be fast! It's used often. */
    surgescript_object_t* entity_manager = get_entity_manager(object);
    surgescript_var_t* ret = surgescript_var_create();

    surgescript_object_call_function(entity_manager, "isInDebugMode", NULL, 0, ret);

    return ret;
}

/* enable/disable the Debug Mode */
surgescript_var_t* fun_set_debugmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* entity_manager = get_entity_manager(object);
    bool want_to_enter = surgescript_var_get_bool(param[0]);

    if(want_to_enter)
        surgescript_object_call_function(entity_manager, "enterDebugMode", NULL, 0, NULL);
    else
        surgescript_object_call_function(entity_manager, "exitDebugMode", NULL, 0, NULL);

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
surgescript_var_t* fun_unload(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    DARRAY(surgescript_objecthandle_t, handles);
    darray_init(handles);

    /* call entityManager.exitDebugMode() */
    surgescript_var_t* entity_manager_var = surgescript_heap_at(heap, ENTITYMANAGER_ADDR);
    surgescript_objecthandle_t entity_manager_handle = surgescript_var_get_objecthandle(entity_manager_var);
    surgescript_object_t* entity_manager = surgescript_objectmanager_get(manager, entity_manager_handle);
    surgescript_object_call_function(entity_manager, "exitDebugMode", NULL, 0, NULL);

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

/* get the PlayerManager */
surgescript_var_t* fun_getplayermanager(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_heap_t* heap = surgescript_object_heap(object);
    const surgescript_var_t* player_group_var = surgescript_heap_at(heap, PLAYERMANAGER_ADDR);
 
    return surgescript_var_clone(player_group_var);
}

/* register the name of a Level setup object */
surgescript_var_t* fun_registersetupobjectname(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* object_name = surgescript_var_fast_get_string(param[0]);
    levelinfo_t* level_info = get_level_info(object);

    /* skip if locked */
    if(!level_info->can_spawn_setup_objects)
        return NULL;

    /* skip duplicates */
    for(int i = darray_length(level_info->setup_object_name) - 1; i >= 0; i--) {
        if(0 == strcmp(level_info->setup_object_name[i], object_name))
            return NULL;
    }

    /* register the name */
    char* cloned_object_name = str_dup(object_name);
    darray_push(level_info->setup_object_name, cloned_object_name);

    /* done */
    return NULL;
}

/* spawn setup objects */
surgescript_var_t* fun_spawnsetupobjects(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    levelinfo_t* level_info = get_level_info(object);
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };

    /* if there are no registered setup objects, register the
       default setup objects for backwards compatibility */
    if(0 == darray_length(level_info->setup_object_name)) {
        const int n = sizeof(DEFAULT_SETUP_OBJECTS) / sizeof(DEFAULT_SETUP_OBJECTS[0]);
        for(int i = 0; i < n; i++) {
            char* cloned_object_name = str_dup(DEFAULT_SETUP_OBJECTS[i]);
            darray_push(level_info->setup_object_name, cloned_object_name);
        }
    }

    /* for each registered level setup object name */
    for(int i = 0; i < darray_length(level_info->setup_object_name); i++) {

        /* call this.__spawnAsSetupObject() */
        surgescript_var_set_string(arg, level_info->setup_object_name[i]);
        surgescript_object_call_function(object, "__spawnAsSetupObject", args, 1, NULL);

    }

    /* lock */
    level_info->can_spawn_setup_objects = false;

    /* done */
    surgescript_var_destroy(arg);
    return NULL;
}

/* spawn an object as a Level setup object */
surgescript_var_t* fun_spawnassetupobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_tagsystem_t* tag_system = surgescript_objectmanager_tagsystem(manager);
    const char* setup_object_name = surgescript_var_fast_get_string(param[0]);

    /* skip if locked */
    const levelinfo_t* level_info = get_level_info(object);
    if(!level_info->can_spawn_setup_objects)
        return surgescript_var_set_null(surgescript_var_create());

    /* skip if the object doesn't exist */
    if(!surgescript_objectmanager_class_exists(manager, setup_object_name)) {
        video_showmessage("Missing setup object: \"%s\"", setup_object_name);
        return surgescript_var_set_null(surgescript_var_create());
    }

    /* check if the setup object is already tagged "setup" */
    if(!surgescript_tagsystem_has_tag(tag_system, setup_object_name, "setup")) {
        logfile_message("Setup object \"%s\" isn't tagged \"setup\"", setup_object_name);
        surgescript_tagsystem_add_tag(tag_system, setup_object_name, "setup");
    }

    /* make the setup object an awake entity for backwards-compatibility purposes */
    if(!surgescript_tagsystem_has_tag(tag_system, setup_object_name, "entity")) {
        surgescript_tagsystem_add_tag(tag_system, setup_object_name, "entity");
        surgescript_tagsystem_add_tag(tag_system, setup_object_name, "awake");
        surgescript_tagsystem_add_tag(tag_system, setup_object_name, "detached");
        surgescript_tagsystem_add_tag(tag_system, setup_object_name, "private");
    }
    else if(
        /* validate: setup object was already an entity */
        !surgescript_tagsystem_has_tag(tag_system, setup_object_name, "awake") &&
        !surgescript_tagsystem_has_tag(tag_system, setup_object_name, "detached")
    ) {
        /* this should not be a setup object */
        video_showmessage("Setup object \"%s\" is an entity, but not awake nor detached", setup_object_name);
    }

    /* call entityManager.spawn(setup_object_name) */
    surgescript_var_t* new_entity_var = surgescript_var_create();
    surgescript_object_t* entity_manager = get_entity_manager(object);
    surgescript_object_call_function(entity_manager, "spawn", param, 1, new_entity_var);

    /* done! */
    return new_entity_var;
}



/*
 * Helpers
 */

/* create a level info struct */
levelinfo_t* level_info_ctor(surgescript_object_t* entity_manager)
{
    levelinfo_t* level_info = mallocx(sizeof *level_info);

    level_info->entity_manager = entity_manager;

    level_info->can_spawn_setup_objects = true; /* start unlocked */
    darray_init(level_info->setup_object_name);

    return level_info;
}

/* destroy a level info struct */
levelinfo_t* level_info_dtor(levelinfo_t* level_info)
{
    /* release setup_object_name[] */
    for(int i = darray_length(level_info->setup_object_name) - 1; i >= 0; i--)
        free(level_info->setup_object_name[i]);
    darray_release(level_info->setup_object_name);

    /* release the struct */
    free(level_info);
    return NULL;
}

/* get the level info */
levelinfo_t* get_level_info(const surgescript_object_t* level)
{
    return (levelinfo_t*)surgescript_object_userdata(level);
}

/* get the EntityManager */
surgescript_object_t* get_entity_manager(const surgescript_object_t* level)
{
#if 1
    return get_level_info(level)->entity_manager;
#else
    surgescript_heap_t* heap = surgescript_object_heap(level);
    surgescript_objectmanager_t* manager = surgescript_object_manager(level);

    surgescript_var_t* entity_manager_var = surgescript_heap_at(heap, ENTITYMANAGER_ADDR);
    surgescript_objecthandle_t entity_manager_handle = surgescript_var_get_objecthandle(entity_manager_var);
    surgescript_object_t* entity_manager = surgescript_objectmanager_get(manager, entity_manager_handle);

    return entity_manager;
#endif
}

/* get the LevelObjectContainer */
surgescript_object_t* get_container(const surgescript_object_t* level)
{
    const surgescript_objectmanager_t* manager = surgescript_object_manager(level);
    const surgescript_heap_t* heap = surgescript_object_heap(level);

    const surgescript_var_t* container_var = surgescript_heap_at(heap, CONTAINER_ADDR);
    surgescript_objecthandle_t container_handle = surgescript_var_get_objecthandle(container_var);
    surgescript_object_t* container = surgescript_objectmanager_get(manager, container_handle);

    return container;
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

/* warn the user about spawning entities as descendants of non-entities that are children of Level */
void warn_about_entity_descendant(surgescript_objecthandle_t entity_handle, void* data)
{
    surgescript_object_t* ascendant = (surgescript_object_t*)data;
    surgescript_objectmanager_t* manager = surgescript_object_manager(ascendant);
    surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

    /* violates ECS */
    video_showmessage("Entity \"%s\" must not be a descendant of \"%s\" (non-entity)", surgescript_object_name(entity), surgescript_object_name(ascendant));
}

/* are we in the Debug Mode? */
bool is_in_debug_mode(const surgescript_object_t* level)
{
    surgescript_object_t* entity_manager = get_entity_manager(level);
    surgescript_var_t* ret = surgescript_var_create();

    surgescript_object_call_function(entity_manager, "isInDebugMode", NULL, 0, ret);
    bool value = surgescript_var_get_bool(ret);

    surgescript_var_destroy(ret);
    return value;
}

/* pause or resume the object containers */
void pause_containers(const surgescript_object_t* level, bool pause)
{
    surgescript_object_t* entity_manager = get_entity_manager(level);
    surgescript_object_t* container = get_container(level);

    if(pause) {
        surgescript_object_call_function(entity_manager, "pauseContainers", NULL, 0, NULL);
        surgescript_object_call_function(container, "pause", NULL, 0, NULL);
    }
    else {
        surgescript_object_call_function(container, "resume", NULL, 0, NULL);
        surgescript_object_call_function(entity_manager, "resumeContainers", NULL, 0, NULL);
    }
}





/*
 * SurgeScript code
 */
static const char code_in_surgescript[] = "\
object 'LevelSetupFunctor' \n\
{ \n\
    level = parent; \n\
\n\
    fun call(config) \n\
    { \n\
        config = config || { }; \n\
        entities = { }; \n\
\n\
        foreach(entry in config) { \n\
            if(level.entity(entry.key) !== null) { \n\
"               /* select entity by ID */ "\
                entities[entry.key] = entry.value; \n\
            } \n\
            else if(System.tags.hasTag(entry.key, 'entity')) { \n\
"               /* select entity by name */ "\
                objs = level.findEntities(entry.key); \n\
                for(i = 0; i < objs.length; i++) \n\
                    setup(objs[i], entry.value); \n\
            } \n\
            else { \n\
"               /* select object by name */ "\
                objs = level.children(entry.key); \n\
                for(i = 0; i < objs.length; i++) \n\
                    setup(objs[i], entry.value); \n\
            } \n\
        } \n\
 \n\
 "      /* entity selected by ID have higher priority */ "\
        foreach(entry in entities) { \n\
            obj = level.entity(entry.key); \n\
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
\n\
    fun constructor() \n\
    { \n\
        assert(level.__name == 'Level'); \n\
    } \n\
} \n\
";
