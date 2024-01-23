/*
 * Open Surge Engine
 * levelobjectcontainer.c - scripting system: a container of in-level objects
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
#include "../util/util.h"

/*

A LevelObjectContainer maintains a sort of "shadow tree". The children of Level
are registered in a container, and that container is responsible for updating
them instead of Level, which is what would happen by default. This technique
lets us pause the code execution of objects, efficiently partition the space,
and so on.

We store the references of Level's children in the heap of these cointainers,
so that they won't be Garbage Collected (i.e., they are not unreachable from
the root of the SurgeScript object tree).

*/

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_passive_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_pause(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_resume(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_addobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_removeobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_hasobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t IDX_ADDR = 0;
#define LAST_BUILTIN_ADDR IDX_ADDR /* must be an alias to the address of the last built-in variable of this object */

/*
* Heap memory layout:
* [ IDX | obj_1 | obj_2 | ... | obj_N ]
* only Level-spawned() objects come after IDX
*/
#define FIRST_STORED_OBJECT_ADDR (1 + LAST_BUILTIN_ADDR)

/* helpers */
static int token = 0;
static bool traverse_links(surgescript_var_t* handle_var, surgescript_heapptr_t ptr, void* data);
static bool find_and_remove_link(surgescript_var_t* handle_var, surgescript_heapptr_t ptr, void* data);
static bool check_if_link_exists(surgescript_var_t* handle_var, surgescript_heapptr_t ptr, void* data);
static bool cleanup_destroyed_objects(surgescript_var_t* handle_var, surgescript_heapptr_t ptr, void* data);
static void recycle_memory(surgescript_object_t* container);

/* iterator */
typedef struct containeriterator_state_t containeriterator_state_t;
struct containeriterator_state_t {

    /* we assume that the container will not be destroyed
       while iterating. That's a reasonable assumption. */
    const surgescript_objectmanager_t* manager;
    const surgescript_heap_t* heap;
    surgescript_heapptr_t next;

    surgescript_object_t* cached_object;
    surgescript_heapptr_t cached_next;

};
static iterator_state_t* containeriterator_state_ctor(void* container_object);
static void containeriterator_state_dtor(iterator_state_t* s);
static void* containeriterator_state_next(iterator_state_t* s);
static bool containeriterator_state_hasnext(iterator_state_t* s);




/*
 * Public functions
 */

/*
 * scripting_register_levelobjectcontainer()
 * Register the LevelObjectContainer object
 */
void scripting_register_levelobjectcontainer(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "LevelObjectContainer", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "LevelObjectContainer", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "LevelObjectContainer", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "LevelObjectContainer", "destroy", fun_destroy, 0);

    surgescript_vm_bind(vm, "LevelObjectContainer", "pause", fun_pause, 0);
    surgescript_vm_bind(vm, "LevelObjectContainer", "resume", fun_resume, 0);

    surgescript_vm_bind(vm, "LevelObjectContainer", "addObject", fun_addobject, 1);
    surgescript_vm_bind(vm, "LevelObjectContainer", "removeObject", fun_removeobject, 1);
    surgescript_vm_bind(vm, "LevelObjectContainer", "hasObject", fun_hasobject, 1);

    /* a passive container is only suitable for preventing garbage collection */
    surgescript_vm_bind(vm, "PassiveLevelObjectContainer", "state:main", fun_passive_main, 0);
    surgescript_vm_bind(vm, "PassiveLevelObjectContainer", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "PassiveLevelObjectContainer", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "PassiveLevelObjectContainer", "destroy", fun_constructor, 0);
    surgescript_vm_bind(vm, "PassiveLevelObjectContainer", "addObject", fun_addobject, 1);
}

/*
 * scripting_levelobjectcontainer_token()
 * Constructor token
 */
void* scripting_levelobjectcontainer_token()
{
    return &token;
}

/*
 * scripting_levelobjectcontainer_iterator()
 * Creates an iterator linked to a LevelContainerObject
 */
iterator_t* scripting_levelobjectcontainer_iterator(surgescript_object_t* container)
{
    return iterator_create(
        container,
        containeriterator_state_ctor,
        containeriterator_state_dtor,
        containeriterator_state_next,
        containeriterator_state_hasnext
    );
}





/*
 * SurgeScript API
 */

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* validate: use a token
       this object must not be spawned via SurgeScript,
       as it traverses the tree and updates the objects */
    const void* my_token = &token;
    const void* their_token = surgescript_object_userdata(object);
    ssassert(my_token == their_token);

    /* initialize */
    ssassert(IDX_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_rawbits(surgescript_heap_at(heap, IDX_ADDR), FIRST_STORED_OBJECT_ADDR);

    /* done */
    return NULL;
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);

    /* cleanup destroyed objects from the previous frame at the beginning of this main state loop */
    surgescript_heap_scan_all(heap, manager, cleanup_destroyed_objects);

    /* recycle memory */
    recycle_memory(object);

    /* traverse the sub-tree of each stored object */
    surgescript_heap_scan_all(heap, manager, traverse_links);

    /* done */
    return NULL;
}

/* main state (passive container) */
surgescript_var_t* fun_passive_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* recycle memory */
    recycle_memory(object);

    /* done */
    return NULL;
}

/* spawn function */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* destroy function */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* pause this container */
surgescript_var_t* fun_pause(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_set_active(object, false);
    return NULL;
}

/* resume this container */
surgescript_var_t* fun_resume(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_set_active(object, true);
    return NULL;
}

/* add an object to this container */
surgescript_var_t* fun_addobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* the parameter must be an object */
    if(!surgescript_var_is_objecthandle(param[0])) {
        scripting_error(object, "%s.addObject() requires an object", surgescript_object_name(object));
        return NULL;
    }

    /* get the new object */
    surgescript_objecthandle_t new_object_handle = surgescript_var_get_objecthandle(param[0]);
    if(!surgescript_objectmanager_exists(manager, new_object_handle)) {
        scripting_error(object, "%s.addObject() received an invalid object (0x%x)", new_object_handle);
        return NULL;
    }
    surgescript_object_t* new_object = surgescript_objectmanager_get(manager, new_object_handle);

    /* the object must be a child of Level */
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(new_object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    const char* parent_name = surgescript_object_name(parent);
    if(0 != strcmp(parent_name, "Level")) {
        scripting_error(object, "%s.addObject() requires \"%s\" to be a child of Level, not of \"%s\"", surgescript_object_name(object), surgescript_object_name(new_object), parent_name);
        return NULL;
    }

    /* store a reference / link to the object, which will prevent garbage collection as well */
    surgescript_heapptr_t ptr = surgescript_heap_malloc(heap);
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, ptr), new_object_handle);

    /* reset the index */
    surgescript_var_set_rawbits(surgescript_heap_at(heap, IDX_ADDR), FIRST_STORED_OBJECT_ADDR);

    /* done! */
    return NULL;
}

/* remove the stored reference (link) to the input object */
surgescript_var_t* fun_removeobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t target_handle = surgescript_var_get_objecthandle(param[0]);

    /* we removed the object if we stopped the iteration at some point */
    bool not_found = surgescript_heap_scan_all(heap, &target_handle, find_and_remove_link);
    bool success = !not_found;

    /* done */
    return surgescript_var_set_bool(surgescript_var_create(), success);
}

/* checks if we have a stored reference (link) to the input object */
surgescript_var_t* fun_hasobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t target_handle = surgescript_var_get_objecthandle(param[0]);

    /* we found the object if we stopped the iteration at some point */
    bool not_found = surgescript_heap_scan_all(heap, &target_handle, check_if_link_exists);
    bool success = !not_found;

    /* done */
    return surgescript_var_set_bool(surgescript_var_create(), success);
}




/*
 * Helpers
 */

void recycle_memory(surgescript_object_t* container)
{
    surgescript_heap_t* heap = surgescript_object_heap(container);
    surgescript_heapptr_t idx = surgescript_var_get_rawbits(surgescript_heap_at(heap, IDX_ADDR));
    size_t heap_size = surgescript_heap_size(heap);

    /* continuously scan the memory for broken references */
    if(idx >= FIRST_STORED_OBJECT_ADDR && idx < heap_size) {
        surgescript_objectmanager_t* manager = surgescript_object_manager(container);

        /* an object stored in heap[idx] has been destroyed */
        if(surgescript_heap_validaddress(heap, idx) && (
            surgescript_var_is_null(surgescript_heap_at(heap, idx)) ||
            !surgescript_objectmanager_exists(manager, surgescript_var_get_objecthandle(surgescript_heap_at(heap, idx)))
        ))
            surgescript_heap_free(heap, idx); /* release the memory, so it can be reused */
    }

    /* update the scan index on the object memory */
    if(++idx >= heap_size)
        idx = FIRST_STORED_OBJECT_ADDR;
    surgescript_var_set_rawbits(surgescript_heap_at(heap, IDX_ADDR), idx);
}

bool cleanup_destroyed_objects(surgescript_var_t* handle_var, surgescript_heapptr_t ptr, void* data)
{
    /* skip initial entries */
    if(ptr < FIRST_STORED_OBJECT_ADDR)
        return true;

    /* skip if null */
    if(surgescript_var_is_null(handle_var))
        return true;

    /* get handle */
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(handle_var);
    surgescript_objectmanager_t* manager = (surgescript_objectmanager_t*)data;

    /* is it a valid object? */
    if(surgescript_objectmanager_exists(manager, handle)) {

        surgescript_object_t* stored_object = surgescript_objectmanager_get(manager, handle);

        /* is it a destroyed object? */
        if(surgescript_object_is_killed(stored_object)) {

            /* release immediately. If we just nullify the link, destructors may not be called
               (destructors are called in surgescript_object_update() the frame after they're destroyed) */
            surgescript_objectmanager_delete(manager, handle);

            /* nullify this link */
            surgescript_var_set_null(handle_var);

        }
    }
    else {

        /* nullify this link */
        surgescript_var_set_null(handle_var);

    }

    /* continue the iteration */
    return true;
}

bool traverse_links(surgescript_var_t* handle_var, surgescript_heapptr_t ptr, void* data)
{
    /* skip initial entries */
    if(ptr < FIRST_STORED_OBJECT_ADDR)
        return true;

    /* skip if null */
    if(surgescript_var_is_null(handle_var))
        return true;

    /* get handle */
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(handle_var);
    surgescript_objectmanager_t* manager = (surgescript_objectmanager_t*)data;

    /* is it a valid object? */
    if(surgescript_objectmanager_exists(manager, handle)) {

        /* traverse the sub-tree */
        surgescript_object_t* stored_object = surgescript_objectmanager_get(manager, handle);
        surgescript_object_traverse_tree(stored_object, surgescript_object_update);

    }

    /* continue the iteration */
    return true;
}

bool find_and_remove_link(surgescript_var_t* handle_var, surgescript_heapptr_t ptr, void* data)
{
    /* skip initial entries */
    if(ptr < FIRST_STORED_OBJECT_ADDR)
        return true;

    /* skip if null */
    if(surgescript_var_is_null(handle_var))
        return true;
    
    /* get handle */
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(handle_var);
    const surgescript_objecthandle_t* target_handle = (const surgescript_objecthandle_t*)data;

    /* nullify the link and stop the iteration
       if we find the target handle */
    if(handle == *target_handle) {
        surgescript_var_set_null(handle_var);
        return false;
    }

    /* otherwise continue the iteration */
    return true;
}

bool check_if_link_exists(surgescript_var_t* handle_var, surgescript_heapptr_t ptr, void* data)
{
    /* skip initial entries */
    if(ptr < FIRST_STORED_OBJECT_ADDR)
        return true;

    /* skip if null */
    if(surgescript_var_is_null(handle_var))
        return true;
    
    /* get handle */
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(handle_var);
    const surgescript_objecthandle_t* target_handle = (const surgescript_objecthandle_t*)data;

    /* continue the iteration while we don't find the target handle */
    return handle != *target_handle;
}



/*
 * Iterator
 */

/* state constructor */
iterator_state_t* containeriterator_state_ctor(void* container_object)
{
    const surgescript_object_t* container = (const surgescript_object_t*)container_object;
    const surgescript_objectmanager_t* manager = surgescript_object_manager(container);
    const surgescript_heap_t* heap = surgescript_object_heap(container);

    containeriterator_state_t* state = mallocx(sizeof *state);
    state->manager = manager;
    state->heap = heap;
    state->next = FIRST_STORED_OBJECT_ADDR;
    state->cached_object = NULL;
    state->cached_next = 0;

    return state;
}

/* state destructor */
void containeriterator_state_dtor(iterator_state_t* s)
{
    containeriterator_state_t* state = (containeriterator_state_t*)s;
    free(state);
}

/* advance the iteration cursor */
void* containeriterator_state_next(iterator_state_t* s)
{
    containeriterator_state_t* state = (containeriterator_state_t*)s;

    /* do we have a cached object? don't recompute things */
    if(state->cached_object != NULL) {
        surgescript_object_t* stored_object = state->cached_object;
        state->cached_object = NULL;
        state->next = state->cached_next; /* advance the cursor */
        return stored_object;
    }

    /* find the next object */
    size_t heap_size = surgescript_heap_size(state->heap);
    while(state->next < heap_size) {
        surgescript_heapptr_t current = state->next++; /* advance the cursor */

        if(surgescript_heap_validaddress(state->heap, current)) {
            const surgescript_var_t* data = surgescript_heap_at(state->heap, current);

            if(!surgescript_var_is_null(data)) { /* data is either null or objecthandle */
                surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(data);

                if(surgescript_objectmanager_exists(state->manager, handle)) {
                    /* success */
                    surgescript_object_t* stored_object = surgescript_objectmanager_get(state->manager, handle);
                    return stored_object;
                }
            }
        }
    }

    /* end of iteration */
    return NULL;
}

/* is the iteration over? */
bool containeriterator_state_hasnext(iterator_state_t* s)
{
    containeriterator_state_t* state = (containeriterator_state_t*)s;
    size_t heap_size = surgescript_heap_size(state->heap);

    /* is there a next object? */
    surgescript_heapptr_t test_next = state->next;
    while(test_next < heap_size) {
        surgescript_heapptr_t current = test_next++;

        if(surgescript_heap_validaddress(state->heap, current)) {
            const surgescript_var_t* data = surgescript_heap_at(state->heap, current);

            if(!surgescript_var_is_null(data)) { /* data is either null or objecthandle */
                surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(data);

                if(surgescript_objectmanager_exists(state->manager, handle)) {
                    /* success */
                    state->cached_object = surgescript_objectmanager_get(state->manager, handle);
                    state->cached_next = test_next;
                    return true;
                }
            }
        }
    }

    /* no more valid objects */
    return false;
}