/*
 * Open Surge Engine
 * transform.c - scripting system: Transform
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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
#include <math.h>
#include "scripting.h"
#include "../util/util.h"

/* private stuff */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_translateby(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_translate(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_rotate(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_scale(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_scaleby(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
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
static surgescript_var_t* fun_getlossyscale(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getright(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getup(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* misc */
static inline surgescript_object_t* target(const surgescript_object_t* object);
static inline surgescript_object_t* checked_target(const surgescript_object_t* object);
static inline void notify_change(surgescript_object_t* object);
static inline surgescript_object_t* get_v2(surgescript_object_t* object, surgescript_heapptr_t addr);
static const char* ONCHANGE = "onTransformChange"; /* fun onTransformChange(transform) is an optional listener on the parent object */
static const surgescript_heapptr_t WORLDPOSITION_ADDR = 0;
static const surgescript_heapptr_t LOCALPOSITION_ADDR = 1;
static const surgescript_heapptr_t LOCALSCALE_ADDR = 2;
static const surgescript_heapptr_t LOSSYSCALE_ADDR = 3;
static const surgescript_heapptr_t RIGHT_ADDR = 4;
static const surgescript_heapptr_t UP_ADDR = 5;



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
    surgescript_vm_bind(vm, "Transform", "scale", fun_scale, 1);
    surgescript_vm_bind(vm, "Transform", "scaleBy", fun_scaleby, 2);
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
    surgescript_vm_bind(vm, "Transform", "get_lossyScale", fun_getlossyscale, 0);
    surgescript_vm_bind(vm, "Transform", "get_right", fun_getright, 0);
    surgescript_vm_bind(vm, "Transform", "get_up", fun_getup, 0);
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
    ssassert(LOSSYSCALE_ADDR == surgescript_heap_malloc(heap));
    ssassert(RIGHT_ADDR == surgescript_heap_malloc(heap));
    ssassert(UP_ADDR == surgescript_heap_malloc(heap));

    /* lazy allocation for the actual vectors */
    surgescript_var_set_null(surgescript_heap_at(heap, WORLDPOSITION_ADDR));
    surgescript_var_set_null(surgescript_heap_at(heap, LOCALPOSITION_ADDR));
    surgescript_var_set_null(surgescript_heap_at(heap, LOCALSCALE_ADDR));
    surgescript_var_set_null(surgescript_heap_at(heap, LOSSYSCALE_ADDR));
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

/* scaleBy: scale by (x,y) */
surgescript_var_t* fun_scaleby(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    double x = surgescript_var_get_number(param[0]);
    double y = surgescript_var_get_number(param[1]);

    surgescript_transform_scale2d(transform, x, y);

    notify_change(object);
    return NULL;
}

/* scale by a Vector2 */
surgescript_var_t* fun_scale(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double x = 0.0, y = 0.0;

    scripting_vector2_read(surgescript_objectmanager_get(manager, v2h), &x, &y);
    surgescript_transform_scale2d(transform, x, y);

    notify_change(object);
    return NULL;
}

/* will look at a given position */
surgescript_var_t* fun_lookat(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* position = surgescript_objectmanager_get(manager, handle);
    double position_x = 0.0, position_y = 0.0;

    scripting_vector2_read(position, &position_x, &position_y);
    surgescript_transform_util_lookat2d(target(object), position_x, position_y);

    notify_change(object);
    return NULL;
}

/* get world position */
surgescript_var_t* fun_getposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* v2 = get_v2(object, WORLDPOSITION_ADDR);
    float world_x = 0.0f, world_y = 0.0f;

    surgescript_transform_util_worldposition2d(target(object), &world_x, &world_y);
    scripting_vector2_update(v2, world_x, world_y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(v2));
}

/* set world position: use translateBy() or translate() to update, unless you're gonna set the world position directly */
surgescript_var_t* fun_setposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double world_x = 0.0, world_y = 0.0;

    scripting_vector2_read(surgescript_objectmanager_get(manager, v2h), &world_x, &world_y);
    surgescript_transform_util_setworldposition2d(target(object), world_x, world_y);

    notify_change(object);
    return NULL;
}

/* get world angle (in degrees) */
surgescript_var_t* fun_getangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    double world_angle = surgescript_transform_util_worldangle2d(target(object)); /* in degrees */
    return surgescript_var_set_number(surgescript_var_create(), world_angle);
}

/* set world angle (in degrees) */
surgescript_var_t* fun_setangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    double world_angle = surgescript_var_get_number(param[0]);

    surgescript_transform_util_setworldangle2d(target(object), world_angle);

    notify_change(object);
    return NULL;
}

/* get local position */
surgescript_var_t* fun_getlocalposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    surgescript_object_t* v2 = get_v2(object, LOCALPOSITION_ADDR);
    float x, y;

    surgescript_transform_getposition2d(transform, &x, &y);
    scripting_vector2_update(v2, x, y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(v2));
}

/* set local position: use translateBy() or translate() to update, unless you're gonna set the position directly */
surgescript_var_t* fun_setlocalposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double x = 0.0, y = 0.0;

    scripting_vector2_read(surgescript_objectmanager_get(manager, v2h), &x, &y);
    surgescript_transform_setposition2d(transform, x, y);

    notify_change(object);
    return NULL;
}

/* get local angle (in degrees) */
surgescript_var_t* fun_getlocalangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(target(object));
    float degrees = surgescript_transform_getrotation2d(transform);
    return surgescript_var_set_number(surgescript_var_create(), degrees);
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
    float sx, sy;

    surgescript_transform_getscale2d(transform, &sx, &sy);
    scripting_vector2_update(v2, sx, sy);

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
    surgescript_transform_setscale2d(transform, x, y);

    notify_change(object);
    return NULL;
}

/* get lossy scale: an approximation of the world scale (not very accurate,
   does not take into account if a parent transform is rotated and scaled) */
surgescript_var_t* fun_getlossyscale(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* v2 = get_v2(object, LOSSYSCALE_ADDR);
    float x = 1.0f, y = 1.0f;

    surgescript_transform_util_lossyscale2d(target(object), &x, &y);
    scripting_vector2_update(v2, x, y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(v2));
}

/* get the right vector of the Transform in world space */
surgescript_var_t* fun_getright(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* v2 = get_v2(object, RIGHT_ADDR);
    float x = 1.0f, y = 0.0f;

    surgescript_transform_util_right2d(target(object), &x, &y);
    scripting_vector2_update(v2, x, y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(v2));
}

/* get the up vector of the Transform in world space */
surgescript_var_t* fun_getup(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* v2 = get_v2(object, UP_ADDR);
    float x = 0.0f, y = -1.0f;

    surgescript_transform_util_up2d(target(object), &x, &y);
    scripting_vector2_update(v2, x, y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(v2));
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