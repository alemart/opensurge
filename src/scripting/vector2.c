/*
 * Open Surge Engine
 * vector2.c - scripting system: immutable 2D Vector
 * Copyright (C) 2018-2019  Alexandre Martins <alemartf@gmail.com>
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
#include <float.h>
#include <math.h>
#include "../core/util.h"
#include "../core/v2d.h"

/* Vector2 structure */
typedef struct surgescript_vector2_t surgescript_vector2_t;
struct surgescript_vector2_t {
    double x, y;
};

/*
*** Note: Vector2 must be immutable ***
*/

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getx(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gety(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlength(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_plus(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_minus(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_dot(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_translatedby(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_rotatedby(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_scaledby(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_normalized(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_directionto(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_distanceto(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_squareddistanceto(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_projectedon(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_tostring(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static inline surgescript_vector2_t* get_vector(const surgescript_object_t* object);
static inline const surgescript_vector2_t* safe_get_vector(const surgescript_object_t* object);
static inline surgescript_objecthandle_t spawn_vector(surgescript_objectmanager_t* manager, double x, double y);
static const surgescript_vector2_t ZERO = { 0.0, 0.0 };
static const double RAD2DEG = 57.2957795131;
static const double EPS = DBL_EPSILON;
static double y_axis = -1.0;

/*
 * scripting_register_vector2()
 * Register the Vector2 object
 */
void scripting_register_vector2(surgescript_vm_t* vm)
{
    /* make the y-axis compatible with SurgeScript's transforms */
    y_axis = surgescript_transform_is_using_inverted_y() ? -1.0 : 1.0;

    /* Vector2 is immutable: do not use verbs as method names */
    surgescript_vm_bind(vm, "Vector2", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Vector2", "__init", fun_init, 2);
    surgescript_vm_bind(vm, "Vector2", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Vector2", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Vector2", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Vector2", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Vector2", "toString", fun_tostring, 0);
    surgescript_vm_bind(vm, "Vector2", "get_x", fun_getx, 0);
    surgescript_vm_bind(vm, "Vector2", "get_y", fun_gety, 0);
    surgescript_vm_bind(vm, "Vector2", "get_length", fun_getlength, 0);
    surgescript_vm_bind(vm, "Vector2", "get_angle", fun_getangle, 0);
    surgescript_vm_bind(vm, "Vector2", "plus", fun_plus, 1);
    surgescript_vm_bind(vm, "Vector2", "minus", fun_minus, 1);
    surgescript_vm_bind(vm, "Vector2", "dot", fun_dot, 1);
    surgescript_vm_bind(vm, "Vector2", "translatedBy", fun_translatedby, 2);
    surgescript_vm_bind(vm, "Vector2", "rotatedBy", fun_rotatedby, 1);
    surgescript_vm_bind(vm, "Vector2", "scaledBy", fun_scaledby, 1);
    surgescript_vm_bind(vm, "Vector2", "normalized", fun_normalized, 0);
    surgescript_vm_bind(vm, "Vector2", "directionTo", fun_directionto, 1);
    surgescript_vm_bind(vm, "Vector2", "distanceTo", fun_distanceto, 1);
    surgescript_vm_bind(vm, "Vector2", "squaredDistanceTo", fun_squareddistanceto, 1);
    surgescript_vm_bind(vm, "Vector2", "projectedOn", fun_projectedon, 1);
}

/*
 * scripting_vector2_update()
 * Updates the contents of a SurgeScript Vector2 object (useful for engine functions / performance)
 * WARNING: Be sure that the referenced object is a Vector2. This function won't check it.
 */
void scripting_vector2_update(surgescript_object_t* object, double x, double y)
{
    surgescript_vector2_t* v = get_vector(object);
    v->x = x;
    v->y = y;
}

/*
 * scripting_vector2_read()
 * Reads the contents of a SurgeScript Vector2 object
 * If the given object is not a Vector2 object, then (0,0) is returned
 */
void scripting_vector2_read(const surgescript_object_t* object, double* x, double* y)
{
    const surgescript_vector2_t* v = safe_get_vector(object);
    *x = v->x;
    *y = v->y;
}

/*
 * scripting_vector2_to_v2d()
 * Converts a SurgeScript Vector2 object to a v2d_t
 * If the given object is not a Vector2 object, then (0,0) is returned
 */
v2d_t scripting_vector2_to_v2d(const surgescript_object_t* object)
{
    const surgescript_vector2_t* v = safe_get_vector(object);
    return v2d_new(v->x, v->y);
}



/* ---- Vector2 API ---- */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    //surgescript_object_set_active(object, false); /* FIXME: GC error (spawn on state) */
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_vector2_t* v = ssmalloc(sizeof *v);
    *v = ZERO;
    surgescript_object_set_userdata(object, v);
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_vector2_t* v = get_vector(object);
    ssfree(v);
    return NULL;
}

/* __init: pass the (x,y) components
   returns the Vector2 itself */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_vector2_t* v = get_vector(object);
    v->x = surgescript_var_get_number(param[0]);
    v->y = surgescript_var_get_number(param[1]);
    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(object));
}

/* spawn */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* can't do it */
    return NULL;
}

/* destroy */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* can't do it */
    return NULL;
}

/* convert to string */
surgescript_var_t* fun_tostring(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_vector2_t* me = get_vector(object);
    char x[24], y[24], buf[50];
    snprintf(x, sizeof(x), "%lf", me->x);
    snprintf(y, sizeof(y), "%lf", me->y);
    snprintf(buf, sizeof(buf), "(%s,%s)", x, y);
    return surgescript_var_set_string(surgescript_var_create(), buf);
}

/* get the x component */
surgescript_var_t* fun_getx(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), get_vector(object)->x);
}

/* get the y component */
surgescript_var_t* fun_gety(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), get_vector(object)->y);
}

/* get the length of the vector */
surgescript_var_t* fun_getlength(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_vector2_t* me = get_vector(object);
    double length = sqrt(me->x * me->x + me->y * me->y);
    return surgescript_var_set_number(surgescript_var_create(), length);
}

/* get the angle, in degrees, between the vector and the positive x-axis as in polar coordinates */
surgescript_var_t* fun_getangle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_vector2_t* me = get_vector(object);
    double degrees = 0.0;
    errno = 0;
    degrees = atan2(me->y * y_axis, me->x) * RAD2DEG;
    if(degrees < 0.0)
        degrees += 360.0;
    return surgescript_var_set_number(surgescript_var_create(), (errno == 0) ? degrees : 0.0);
}

/* '+' operator */
surgescript_var_t* fun_plus(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    const surgescript_vector2_t* other = safe_get_vector(surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(param[0])));
    surgescript_objecthandle_t result = spawn_vector(manager,
        me->x + other->x,
        me->y + other->y
    );
    return surgescript_var_set_objecthandle(surgescript_var_create(), result);
}

/* '-' operator */
surgescript_var_t* fun_minus(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    const surgescript_vector2_t* other = safe_get_vector(surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(param[0])));
    surgescript_objecthandle_t result = spawn_vector(manager,
        me->x - other->x,
        me->y - other->y
    );
    return surgescript_var_set_objecthandle(surgescript_var_create(), result);
}

/* dot product */
surgescript_var_t* fun_dot(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    const surgescript_vector2_t* other = safe_get_vector(surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(param[0])));
    double result = me->x * other->x + me->y * other->y;
    return surgescript_var_set_number(surgescript_var_create(), result);
}

/* returns the vector multiplied by a scalar */
surgescript_var_t* fun_scaledby(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    double scalar = surgescript_var_get_number(param[0]);
    surgescript_objecthandle_t result = spawn_vector(manager,
        scalar * me->x,
        scalar * me->y
    );
    return surgescript_var_set_objecthandle(surgescript_var_create(), result);
}

/* returns the vector translated by (dx,dy) */
surgescript_var_t* fun_translatedby(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    double dx = surgescript_var_get_number(param[0]);
    double dy = surgescript_var_get_number(param[1]);
    surgescript_objecthandle_t result = spawn_vector(manager,
        me->x + dx,
        me->y + dy
    );
    return surgescript_var_set_objecthandle(surgescript_var_create(), result);
}

/* returns the vector rotated counterclockwise by a number of degrees */
surgescript_var_t* fun_rotatedby(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    double angle = surgescript_var_get_number(param[0]) / RAD2DEG;
    double s = sin(angle) * y_axis, c = cos(angle);
    surgescript_objecthandle_t result = spawn_vector(manager,
        me->x * c - me->y * s,
        me->x * s + me->y * c
    );
    return surgescript_var_set_objecthandle(surgescript_var_create(), result);
}

/* returns the vector normalized to a unit vector */
surgescript_var_t* fun_normalized(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    double length = sqrt(me->x * me->x + me->y * me->y);
    length = max(length, EPS);
    surgescript_objecthandle_t result = spawn_vector(manager,
        me->x / length,
        me->y / length
    );
    return surgescript_var_set_objecthandle(surgescript_var_create(), result);
}

/* returns a unit vector pointing to the given vector (from this vector) */
surgescript_var_t* fun_directionto(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    const surgescript_vector2_t* other = safe_get_vector(surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(param[0])));
    double dx = other->x - me->x;
    double dy = other->y - me->y;
    double length = sqrt(dx * dx + dy * dy);
    length = max(length, EPS);
    surgescript_objecthandle_t result = spawn_vector(manager,
        dx / length,
        dy / length
    );
    return surgescript_var_set_objecthandle(surgescript_var_create(), result);
}

/* the distance between two points in space */
surgescript_var_t* fun_distanceto(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    const surgescript_vector2_t* other = safe_get_vector(surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(param[0])));
    double dx = me->x - other->x;
    double dy = me->y - other->y;
    return surgescript_var_set_number(surgescript_var_create(), sqrt(dx * dx + dy * dy));
}

/* the squared distance between two points in space */
surgescript_var_t* fun_squareddistanceto(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    const surgescript_vector2_t* other = safe_get_vector(surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(param[0])));
    double dx = me->x - other->x;
    double dy = me->y - other->y;
    return surgescript_var_set_number(surgescript_var_create(), dx * dx + dy * dy);
}

/* the vector projected onto another */
surgescript_var_t* fun_projectedon(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const surgescript_vector2_t* me = get_vector(object);
    const surgescript_vector2_t* other = safe_get_vector(surgescript_objectmanager_get(manager, surgescript_var_get_objecthandle(param[0])));
    double dot = me->x * other->x + me->y * other->y;
    double length2 = other->x * other->x + other->y * other->y;
    length2 = max(length2, EPS);
    surgescript_objecthandle_t result = spawn_vector(manager,
        (dot / length2) * other->x,
        (dot / length2) * other->y
    );
    return surgescript_var_set_objecthandle(surgescript_var_create(), result);
}


/* --- utilities --- */

/* gets the Vector2 structure (without checking the validity of the object) */
surgescript_vector2_t* get_vector(const surgescript_object_t* object)
{
    return (surgescript_vector2_t*)(surgescript_object_userdata(object));
}

/* returns the Vector2 structure if the object is a Vector2, or ZERO otherwise */
const surgescript_vector2_t* safe_get_vector(const surgescript_object_t* object)
{
    if(strcmp(surgescript_object_name(object), "Vector2") == 0)
        return get_vector(object);
    else
        return &ZERO;
}

/* spawn a new Vector2 */
surgescript_objecthandle_t spawn_vector(surgescript_objectmanager_t* manager, double x, double y)
{
    surgescript_objecthandle_t handle = surgescript_objectmanager_spawn_temp(manager, "Vector2");
    surgescript_vector2_t* v = get_vector(surgescript_objectmanager_get(manager, handle));
    v->x = x;
    v->y = y;
    return handle;
}