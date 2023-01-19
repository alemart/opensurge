/*
 * Open Surge Engine
 * camera.c - scripting system: camera object
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
#include "scripting.h"
#include "../core/video.h"
#include "../entities/camera.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlocked(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_lock(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_unlock(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_screentoworld(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_worldtoscreen(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t POSITION_ADDR = 0;

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
    surgescript_vm_bind(vm, "Camera", "get_locked", fun_getlocked, 0);
    surgescript_vm_bind(vm, "Camera", "lock", fun_lock, 4);
    surgescript_vm_bind(vm, "Camera", "unlock", fun_unlock, 0);
    surgescript_vm_bind(vm, "Camera", "screenToWorld", fun_screentoworld, 1);
    surgescript_vm_bind(vm, "Camera", "worldToScreen", fun_worldtoscreen, 1);
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

    camera_set_position(scripting_vector2_to_v2d(v2));

    return NULL;
}

/* is the camera locked? */
surgescript_var_t* fun_getlocked(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_bool(surgescript_var_create(), camera_is_locked());
}

/* lock the camera to the boundaries (x1, y1, x2, y2). All coordinates are given in pixels; x1 < x2, y1 < y2 */
surgescript_var_t* fun_lock(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int x1 = surgescript_var_get_number(param[0]);
    int y1 = surgescript_var_get_number(param[1]);
    int x2 = surgescript_var_get_number(param[2]);
    int y2 = surgescript_var_get_number(param[3]);

    camera_lock(x1, y1, x2, y2);

    return NULL;
}

/* unlock the camera */
surgescript_var_t* fun_unlock(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    camera_unlock();
    return NULL;
}

/* converts a point from screen space to world space */
surgescript_var_t* fun_screentoworld(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_objecthandle_t new_handle = surgescript_objectmanager_spawn_temp(manager, "Vector2");
    v2d_t point = scripting_vector2_to_v2d(surgescript_objectmanager_get(manager, handle));
    v2d_t screen_center = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t camera_topleft = v2d_subtract(camera_get_position(), screen_center);

    /* convert to world space */
    scripting_vector2_update(
        surgescript_objectmanager_get(manager, new_handle),
        point.x + camera_topleft.x, point.y + camera_topleft.y
    );

    return surgescript_var_set_objecthandle(surgescript_var_create(), new_handle);
}

/* converts a point from world space to screen space */
surgescript_var_t* fun_worldtoscreen(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_objecthandle_t new_handle = surgescript_objectmanager_spawn_temp(manager, "Vector2");
    v2d_t point = scripting_vector2_to_v2d(surgescript_objectmanager_get(manager, handle));
    v2d_t screen_center = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t camera_topleft = v2d_subtract(camera_get_position(), screen_center);

    /* convert to screen space */
    scripting_vector2_update(
        surgescript_objectmanager_get(manager, new_handle),
        point.x - camera_topleft.x, point.y - camera_topleft.y
    );

    return surgescript_var_set_objecthandle(surgescript_var_create(), new_handle);
}