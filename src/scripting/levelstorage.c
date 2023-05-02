/*
 * Open Surge Engine
 * levelstorage.c - scripting system: a storage of references of in-level objects
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

/*

We store the references of Level's children in the heap of LevelStorage,
so that they won't be Garbage Collected (i.e., they are not unreachable
from the root of the SurgeScript object tree).

*/

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_storereference(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t IDX_ADDR = 0;
#define LAST_BUILTIN_ADDR IDX_ADDR /* must be an alias to the address of the last built-in variable of this object */

/*
* Heap memory layout:
* [ IDX | obj_1 | obj_2 | ... | obj_N ]
* only Level-spawned() objects come after IDX
*/
#define FIRST_STORED_OBJECT_ADDR (1 + LAST_BUILTIN_ADDR)



/*
 * scripting_register_levelstorage()
 * Register the LevelStorage object
 */
void scripting_register_levelstorage(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "LevelStorage", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "LevelStorage", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "LevelStorage", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "LevelStorage", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "LevelStorage", "storeReference", fun_storereference, 1);
}



/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* validate: Level must be the parent object */
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    const char* parent_name = surgescript_object_name(parent);
    if(0 != strcmp(parent_name, "Level")) {
        scripting_error(object, "Not a child of Level");
        return NULL;
    }

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
    surgescript_heapptr_t idx = surgescript_var_get_rawbits(surgescript_heap_at(heap, IDX_ADDR));
    size_t heap_size = surgescript_heap_size(heap);

    /* continuously scan the memory for broken references */
    if(idx >= FIRST_STORED_OBJECT_ADDR && idx < heap_size) {
        surgescript_objectmanager_t* manager = surgescript_object_manager(object);

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

/* store the reference of the object in order to prevent garbage collection */
surgescript_var_t* fun_storereference(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* the parameter must be an object */
    if(!surgescript_var_is_objecthandle(param[0])) {
        scripting_error(object, "%s.storeReference() requires an object", surgescript_object_name(object));
        return NULL;
    }

    /* get my parent, which is Level */
    surgescript_objecthandle_t my_parent_handle = surgescript_object_parent(object);
    surgescript_object_t* my_parent = surgescript_objectmanager_get(manager, my_parent_handle);

    /* the object must be a descendant of Level */
    surgescript_objecthandle_t child_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* child = surgescript_objectmanager_get(manager, child_handle);
    surgescript_objecthandle_t childs_parent = surgescript_object_parent(child);
    if(!surgescript_object_is_ascendant(child, my_parent)) {
        scripting_error(object, "%s.storeReference() requires object \"%s\" to be a descendant of %s", surgescript_object_name(object), surgescript_object_name(child), surgescript_object_name(my_parent));
        return NULL;
    }

    /* store a reference to the object, so it won't be Garbage Collected */
    surgescript_heapptr_t ptr = surgescript_heap_malloc(heap);
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, ptr), child_handle);

    /* reset the index */
    surgescript_var_set_rawbits(surgescript_heap_at(heap, IDX_ADDR), FIRST_STORED_OBJECT_ADDR);

    /* done! */
    return NULL;
}