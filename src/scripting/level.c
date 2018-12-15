/*
 * Open Surge Engine
 * level.c - scripting system: Level object
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
#include "scripting.h"
#include "../core/util.h"
#include "../core/audio.h"
#include "../core/stringutil.h"
#include "../scenes/level.h"
#include "../entities/player.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmusic(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getwaterlevel(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setwaterlevel(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getcleared(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getname(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getact(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getversion(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getauthor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlicense(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfile(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_clear(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_restart(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_quit(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_abort(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_pause(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_load(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_finish(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t MUSIC_ADDR = 0;
static const surgescript_heapptr_t IDX_ADDR = 1; /* must be the last address */
static void update_music(surgescript_object_t* object);
extern music_t* scripting_music_ptr(const surgescript_object_t* object);

/*
 * scripting_register_level()
 * Register the Level object
 */
void scripting_register_level(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Level", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Level", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Level", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Level", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Level", "get_name", fun_getname, 0);
    surgescript_vm_bind(vm, "Level", "get_act", fun_getact, 0);
    surgescript_vm_bind(vm, "Level", "get_version", fun_getversion, 0);
    surgescript_vm_bind(vm, "Level", "get_author", fun_getauthor, 0);
    surgescript_vm_bind(vm, "Level", "get_license", fun_getlicense, 0);
    surgescript_vm_bind(vm, "Level", "get_file", fun_getfile, 0);
    surgescript_vm_bind(vm, "Level", "get_music", fun_getmusic, 0);
    surgescript_vm_bind(vm, "Level", "set_waterlevel", fun_setwaterlevel, 1);
    surgescript_vm_bind(vm, "Level", "get_waterlevel", fun_getwaterlevel, 0);
    surgescript_vm_bind(vm, "Level", "get_cleared", fun_getcleared, 0);
    surgescript_vm_bind(vm, "Level", "clear", fun_clear, 0);
    surgescript_vm_bind(vm, "Level", "restart", fun_restart, 0);
    surgescript_vm_bind(vm, "Level", "quit", fun_quit, 0);
    surgescript_vm_bind(vm, "Level", "abort", fun_abort, 0);
    surgescript_vm_bind(vm, "Level", "pause", fun_pause, 0);
    surgescript_vm_bind(vm, "Level", "load", fun_load, 1);
    surgescript_vm_bind(vm, "Level", "finish", fun_finish, 0);
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* Level music */
    ssassert(MUSIC_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, MUSIC_ADDR));

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
    if(level_music() != NULL)
        update_music(object);

    /* done */
    return NULL;
}

/* spawn new object: entities will have special treatment */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* child_name = surgescript_var_fast_get_string(param[0]);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_tagsystem_t* tag_system = surgescript_objectmanager_tagsystem(manager);

    /* spawn the new object */
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t child = surgescript_objectmanager_spawn(manager, me, child_name, NULL);

    /* is the new object an entity? */
    /* exception: startup objects may not be entities */
    if(1 || surgescript_tagsystem_has_tag(tag_system, child_name, "entity")) {
        /* store its reference, so it won't be Garbage Collected */
        surgescript_heap_t* heap = surgescript_object_heap(object);
        surgescript_heapptr_t ptr = surgescript_heap_malloc(heap);
        surgescript_var_set_objecthandle(surgescript_heap_at(heap, ptr), child);
        surgescript_var_set_rawbits(surgescript_heap_at(heap, IDX_ADDR), 1 + IDX_ADDR); /* scan for broken references */
    }

    /* done! */
    return surgescript_var_set_objecthandle(surgescript_var_create(), child);
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
surgescript_var_t* fun_finish(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    level_jump_to_next_stage();
    return NULL;
}



/* -- utils -- */

/* updates the reference to Level.music */
void update_music(surgescript_object_t* object)
{
    music_t* music = level_music(); /* assumed to be not-NULL <-- important! */
    surgescript_heap_t* heap = surgescript_object_heap(object);

    if(surgescript_var_is_null(surgescript_heap_at(heap, MUSIC_ADDR)) ||
        music != scripting_music_ptr(
            surgescript_objectmanager_get(surgescript_object_manager(object),
                surgescript_var_get_objecthandle(surgescript_heap_at(heap, MUSIC_ADDR))
            )
        )
    ) {
        surgescript_object_t* music_component = scripting_util_get_component(
            scripting_util_surgeengine_component(surgescript_vm(), "Audio"), "Music"
        );
        surgescript_var_t* tmp[] = {
            surgescript_var_create(), surgescript_var_create(), surgescript_var_create()
        };

        surgescript_var_set_objecthandle(tmp[0], surgescript_object_handle(object));
        surgescript_var_set_string(tmp[1], music_path(music));
        surgescript_object_call_function(music_component, "__spawn", (const surgescript_var_t**)tmp, 2, tmp[2]);
        surgescript_var_set_objecthandle(surgescript_heap_at(heap, MUSIC_ADDR), surgescript_var_get_objecthandle(tmp[2]));

        surgescript_var_destroy(tmp[2]);
        surgescript_var_destroy(tmp[1]);
        surgescript_var_destroy(tmp[0]);
    }
}