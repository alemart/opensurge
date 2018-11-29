/*
 * Open Surge Engine
 * camera.c - scripting system: camera object
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
#include "../entities/camera.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t POSITION_ADDR = 0;
extern void scripting_vector2_read(const surgescript_object_t* object, double* x, double* y);
extern void scripting_vector2_update(surgescript_object_t* object, double x, double y);

/*
 * scripting_register_camera()
 * Register the Camera object
 */
void scripting_register_camera(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Camera", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Camera", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Camera", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Camera", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Camera", "get_position", fun_getposition, 0);
    surgescript_vm_bind(vm, "Camera", "set_position", fun_setposition, 1);
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t position = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);

    ssassert(POSITION_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, POSITION_ADDR), position);

    return NULL;
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
}

/* destroy */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* not allowed */
    return NULL;
}

/* spawn */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* not allowed */
    return NULL;
}

/* get camera position, in world coordinates */
surgescript_var_t* fun_getposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, POSITION_ADDR));
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, handle);
    v2d_t cam = camera_get_position();

    scripting_vector2_update(v2, cam.x, cam.y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* set camera position, in world coordinates */
surgescript_var_t* fun_setposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, handle);
    double x = 0.0, y = 0.0;

    scripting_vector2_read(v2, &x, &y);
    camera_set_position(v2d_new(x, y));

    return NULL;
}