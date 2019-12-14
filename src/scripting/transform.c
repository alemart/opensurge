/*
 * Open Surge Engine
 * transform.c - scripting system: Transform
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
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "scripting.h"
#include "../core/util.h"

/* private stuff */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_translateby(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_translate(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_rotate(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_lookat(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlocalposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setlocalposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlocalangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setlocalangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlocalscale(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setlocalscale(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* misc */
static inline surgescript_object_t* target(const surgescript_object_t* object);
static inline surgescript_object_t* checked_target(const surgescript_object_t* object);
static void world2localposition(surgescript_objectmanager_t* manager, surgescript_objecthandle_t handle, surgescript_objecthandle_t root, double *xpos, double *ypos);
static inline void worldposition2d(surgescript_object_t* object, double* world_x, double* world_y);
static inline void setworldposition2d(surgescript_object_t* object, double world_x, double world_y);
static inline double worldangle2d(surgescript_object_t* object);
static inline void setworldangle2d(surgescript_object_t* object, double angle);
static inline void notify_change(surgescript_object_t* object);
static inline surgescript_object_t* get_v2(surgescript_object_t* object, surgescript_heapptr_t addr);
static const double RAD2DEG = 57.2957795131;
static const char* ONCHANGE = "onTransformChange"; /* fun onTransformChange(transform): [optional] listener on the parent object */
static const surgescript_heapptr_t WORLDPOSITION_ADDR = 0;
static const surgescript_heapptr_t LOCALPOSITION_ADDR = 1;
static const surgescript_heapptr_t LOCALSCALE_ADDR = 2;
static const surgescript_heapptr_t RIGHT_ADDR = 3;
static const surgescript_heapptr_t UP_ADDR = 4;



/*
 * scripting_register_transform()
 * Register the Transform object
 */
void scripting_register_transform(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Transform", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Transform", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Transform", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Transform", "translate", fun_translate, 1);
    surgescript_vm_bind(vm, "Transform", "translateBy", fun_translateby, 2);
    surgescript_vm_bind(vm, "Transform", "move", fun_translateby, 2); /* deprecated */
    surgescript_vm_bind(vm, "Transform", "rotate", fun_rotate, 1);
    surgescript_vm_bind(vm, "Transform", "lookAt", fun_lookat, 1);
    surgescript_vm_bind(vm, "Transform", "get_position", fun_getposition, 0);
    surgescript_vm_bind(vm, "Transform", "set_position", fun_setposition, 1);
    surgescript_vm_bind(vm, "Transform", "get_angle", fun_getangle, 0);
    surgescript_vm_bind(vm, "Transform", "set_angle", fun_setangle, 1);
    surgescript_vm_bind(vm, "Transform", "get_localPosition", fun_getlocalposition, 0);
    surgescript_vm_bind(vm, "Transform", "set_localPosition", fun_setlocalposition, 1);
    surgescript_vm_bind(vm, "Transform", "get_localAngle", fun_getlocalangle, 0);
    surgescript_vm_bind(vm, "Transform", "set_localAngle", fun_setlocalangle, 1);
    surgescript_vm_bind(vm, "Transform", "get_localScale", fun_getlocalscale, 0);
    surgescript_vm_bind(vm, "Transform", "set_localScale", fun_setlocalscale, 1);
}



/* my functions */



/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* allocate space for the vectors */
    ssassert(WORLDPOSITION_ADDR == surgescript_heap_malloc(heap));
    ssassert(LOCALPOSITION_ADDR == surgescript_heap_malloc(heap));
    ssassert(LOCALSCALE_ADDR == surgescript_heap_malloc(heap));
    ssassert(RIGHT_ADDR == surgescript_heap_malloc(heap));
    ssassert(UP_ADDR == surgescript_heap_malloc(heap));

    /* lazy allocation for the actual vectors */
    surgescript_var_set_null(surgescript_heap_at(heap, WORLDPOSITION_ADDR));
    surgescript_var_set_null(surgescript_heap_at(heap, LOCALPOSITION_ADDR));
    surgescript_var_set_null(surgescript_heap_at(heap, LOCALSCALE_ADDR));
    surgescript_var_set_null(surgescript_heap_at(heap, RIGHT_ADDR));
    surgescript_var_set_null(surgescript_heap_at(heap, UP_ADDR));

    /* register target object for change notifications */
    if(surgescript_object_has_function(target(object), ONCHANGE))
        surgescript_object_set_userdata(object, target(object));
    else
        surgescript_object_set_userdata(object, NULL);

    /* done */
    return NULL;
}

/* spawn */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
}

/* translateBy: translate by (x,y) */
surgescript_var_t* fun_translateby(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    double x = surgescript_var_get_number(param[0]);
    double y = surgescript_var_get_number(param[1]);

    surgescript_transform_translate2d(transform, x, y);

    notify_change(object);
    return NULL;
}

/* translate by a Vector2 */
surgescript_var_t* fun_translate(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double x = 0.0, y = 0.0;

    scripting_vector2_read(surgescript_objectmanager_get(manager, v2h), &x, &y);
    surgescript_transform_translate2d(transform, x, y);

    notify_change(object);
    return NULL;
}

/* rotate (given an angle in degrees) */
surgescript_var_t* fun_rotate(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    double degrees = surgescript_var_get_number(param[0]);

    surgescript_transform_rotate2d(transform, degrees);

    notify_change(object);
    return NULL;
}

/* will look at a given position */
surgescript_var_t* fun_lookat(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t position_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* looked = surgescript_objectmanager_get(manager, position_handle);
    surgescript_object_t* looker = target(object);
    double looked_x, looked_y, looker_x, looker_y, angle;

    worldposition2d(looker, &looker_x, &looker_y);
    scripting_vector2_read(looked, &looked_x, &looked_y);

    errno = 0;
    angle = atan2(looker_y - looked_y, looked_x - looker_x);
    if(errno == 0) {
        setworldangle2d(looker, angle * RAD2DEG);
        notify_change(object);
    }

    return NULL;
}

/* get world position */
surgescript_var_t* fun_getposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* v2 = get_v2(object, WORLDPOSITION_ADDR);
    double world_x = 0.0, world_y = 0.0;

    worldposition2d(target(object), &world_x, &world_y);
    scripting_vector2_update(v2, world_x, world_y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(v2));
}

/* set world position: use move() or translate() to update, unless you're gonna set the world position directly */
surgescript_var_t* fun_setposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double x = 0.0, y = 0.0;

    scripting_vector2_read(surgescript_objectmanager_get(manager, v2h), &x, &y);
    setworldposition2d(target(object), x, y);

    notify_change(object);
    return NULL;
}

/* get world angle (in degrees) */
surgescript_var_t* fun_getangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    double world_angle = worldangle2d(target(object)); /* in degrees */
    return surgescript_var_set_number(surgescript_var_create(), world_angle);
}

/* set world angle (in degrees) */
surgescript_var_t* fun_setangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    setworldangle2d(target(object), surgescript_var_get_number(param[0]));
    notify_change(object);
    return NULL;
}

/* get local position */
surgescript_var_t* fun_getlocalposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    surgescript_object_t* v2 = get_v2(object, LOCALPOSITION_ADDR);

    scripting_vector2_update(v2, transform->position.x, transform->position.y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(v2));
}

/* set local position: use move() or translate() to update, unless you're gonna set the position directly */
surgescript_var_t* fun_setlocalposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double x = 0.0, y = 0.0;

    scripting_vector2_read(surgescript_objectmanager_get(manager, v2h), &x, &y);
    transform->position.x = x;
    transform->position.y = y;

    notify_change(object);
    return NULL;
}

/* get local angle (in degrees) */
surgescript_var_t* fun_getlocalangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    return surgescript_var_set_number(surgescript_var_create(), transform->rotation.z);
}

/* set local angle (in degrees) */
surgescript_var_t* fun_setlocalangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    surgescript_transform_setrotation2d(transform, surgescript_var_get_number(param[0]));
    notify_change(object);
    return NULL;
}

/* get local scale */
surgescript_var_t* fun_getlocalscale(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    surgescript_object_t* v2 = get_v2(object, LOCALSCALE_ADDR);

    scripting_vector2_update(v2, transform->scale.x, transform->scale.y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(v2));
}

/* set local scale */
surgescript_var_t* fun_setlocalscale(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double x = 0.0, y = 0.0;

    scripting_vector2_read(surgescript_objectmanager_get(manager, v2h), &x, &y);
    transform->scale.x = x;
    transform->scale.y = y;

    notify_change(object);
    return NULL;
}



/* misc */

/* will return the target object of the given transform object */
surgescript_object_t* target(const surgescript_object_t* object)
{
    /* the target object is the parent of the transform */
    return surgescript_objectmanager_get(
        surgescript_object_manager(object),
        surgescript_object_parent(object)
    );
}

/* will check if the given object is a Transform and return its target object */
surgescript_object_t* checked_target(const surgescript_object_t* object)
{
    const char* name = surgescript_object_name(object);

    if(strcmp(name, "Transform") != 0)
        return NULL;
        
    return target(object);
}

/* the Vector2 object at the specified address */
surgescript_object_t* get_v2(surgescript_object_t* object, surgescript_heapptr_t addr)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* v2ptr = surgescript_heap_at(heap, addr);

    /* lazy allocation */
    if(surgescript_var_is_null(v2ptr)) {
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        surgescript_objecthandle_t v2 = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
        surgescript_var_set_objecthandle(v2ptr, v2);
    }

    /* return the vector */
    return surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(v2ptr));
}

/* this will help compute the local position, recursively, of an object given its world position */
void world2localposition(surgescript_objectmanager_t* manager, surgescript_objecthandle_t handle, surgescript_objecthandle_t root, double* xpos, double* ypos)
{
    surgescript_object_t* object = surgescript_objectmanager_get(manager, handle);
    surgescript_transform_t transform;
    float x, y;

    if(handle != root)
        world2localposition(manager, surgescript_object_parent(object), root, xpos, ypos);

    x = *xpos;
    y = *ypos;
    surgescript_object_peek_transform(object, &transform);
    surgescript_transform_apply2dinverse(&transform, &x, &y);
    *xpos = x;
    *ypos = y;

    /* alternatively, one could compute the world position of the parent and return the difference */
}

/* Computes the 2D world position of the object */
void worldposition2d(surgescript_object_t* object, double* world_x, double* world_y)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t root = surgescript_objectmanager_root(manager);
    surgescript_objecthandle_t handle = surgescript_object_handle(object);
    surgescript_transform_t transform;
    float x, y;

    /* get local position */
    surgescript_object_peek_transform(object, &transform);
    x = transform.position.x;
    y = transform.position.y;

    /* compute world position */
    while(handle != root) {
        handle = surgescript_object_parent(object);
        object = surgescript_objectmanager_get(manager, handle); /* object receives its parent */
        surgescript_object_peek_transform(object, &transform);
        surgescript_transform_apply2d(&transform, &x, &y);
    }

    /* done! */
    *world_x = x;
    *world_y = y;
}

/* Sets the 2D world position of the object (recomputes the local transform accordingly) */
void setworldposition2d(surgescript_object_t* object, double world_x, double world_y)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t root = surgescript_objectmanager_root(manager);
    surgescript_objecthandle_t handle = surgescript_object_handle(object);
    surgescript_transform_t* transform = surgescript_object_transform(object);

    /* compute local transform */
    if(handle != root)
        world2localposition(manager, surgescript_object_parent(object), root, &world_x, &world_y);

    /* set local transform */
    transform->position.x = world_x;
    transform->position.y = world_y;
}

/* computes the world angle of the object, given its local angle */
double worldangle2d(surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t root = surgescript_objectmanager_root(manager);
    surgescript_objecthandle_t handle = surgescript_object_handle(object);
    surgescript_transform_t transform;
    double world_angle;

    /* get local angle */
    surgescript_object_peek_transform(object, &transform);
    world_angle = transform.rotation.z;

    /* compute world angle */
    while(handle != root) {
        handle = surgescript_object_parent(object);
        object = surgescript_objectmanager_get(manager, handle); /* object receives its parent */
        surgescript_object_peek_transform(object, &transform);
        world_angle += transform.rotation.z;
    }

    /* done! */
    return fmod(world_angle, 360.0);
}

/* sets the world angle of the object by computing its corresponding local angle */
void setworldangle2d(surgescript_object_t* object, double angle)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent = surgescript_object_parent(object);
    surgescript_transform_t* transform = surgescript_object_transform(object);

    /* compute corresponding local angle */
    angle -= worldangle2d(surgescript_objectmanager_get(manager, parent));

    /* update local angle */
    surgescript_transform_setrotation2d(transform, angle);
}

/* notify the target object of a transform change */
void notify_change(surgescript_object_t* object)
{
    surgescript_object_t* notifiable_target = (surgescript_object_t*)surgescript_object_userdata(object);
    if(notifiable_target != NULL && notifiable_target == target(object)) { /* safety check */
        surgescript_var_t* transform_handle = surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(object));
        const surgescript_var_t* p[] = { transform_handle };
        surgescript_object_call_function(notifiable_target, ONCHANGE, p, 1, NULL);
        surgescript_var_destroy(transform_handle);
    }
}
