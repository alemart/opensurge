/*
 * Open Surge Engine
 * application.c - scripting system: application object
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

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_callexitfunctor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getonexit(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setonexit(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t EXITFUNCTOR_ADDR = 0;

/*
 * scripting_register_application()
 * Register the default Application object
 */
void scripting_register_application(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Application", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Application", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Application", "__callExitFunctor", fun_callexitfunctor, 0);
    surgescript_vm_bind(vm, "Application", "set_onExit", fun_setonexit, 1);
    surgescript_vm_bind(vm, "Application", "get_onExit", fun_getonexit, 0);
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* allocate variable */
    ssassert(EXITFUNCTOR_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, EXITFUNCTOR_ADDR));

    return NULL;
}

/* this function is called when the engine is closed */
surgescript_var_t* fun_callexitfunctor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* onexit = surgescript_heap_at(heap, EXITFUNCTOR_ADDR);

    /* we require Application.onExit to be an existing function object;
       if it's not, do nothing */
    if(surgescript_var_is_objecthandle(onexit)) {
        surgescript_objectmanager_t* manager = surgescript_object_manager(object);
        surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(onexit);
        if(surgescript_objectmanager_exists(manager, handle)) {
            surgescript_object_t* functor = surgescript_objectmanager_get(manager, handle);
            if(surgescript_object_has_function(functor, "call"))
                surgescript_object_call_function(functor, "call", NULL, 0, NULL);
        }
    }

    return NULL;
}

/* gets onExit, a functor called when unloading the level */
surgescript_var_t* fun_getonexit(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, EXITFUNCTOR_ADDR));
}

/* sets onExit, a functor called when unloading the level */
surgescript_var_t* fun_setonexit(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* onexit = surgescript_heap_at(heap, EXITFUNCTOR_ADDR);
    surgescript_var_copy(onexit, param[0]);
    return NULL;
}