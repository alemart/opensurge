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
#include "scripting.h"
#include "../core/util.h"
#include "../core/audio.h"
#include "../scenes/level.h"
#include "../entities/player.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmusic(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
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
    surgescript_vm_bind(vm, "Level", "get_music", fun_getmusic, 0);
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
    if(surgescript_tagsystem_has_tag(tag_system, child_name, "entity")) {
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