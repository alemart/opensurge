/*
 * Open Surge Engine
 * sensor.c - scripting system: sensor component
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
#include "../core/v2d.h"
#include "../core/util.h"
#include "../core/image.h"
#include "../entities/camera.h"
#include "../physics/obstacle.h"
#include "../physics/obstaclemap.h"
#include "../physics/physicsactor.h"
#include "../physics/sensor.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_update(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getstatus(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static inline const obstaclemap_t* get_obstaclemap(const surgescript_object_t* object);
static inline sensor_t* get_sensor(const surgescript_object_t* object);
static inline void update(surgescript_object_t* object);
static v2d_t parent_camera(const surgescript_object_t* object);
static const surgescript_heapptr_t OBSTACLEMAP_ADDR = 0;
static const surgescript_heapptr_t VISIBLE_ADDR = 1;
static const surgescript_heapptr_t STATUS_ADDR = 2;
#define SENSOR_COLOR() (color_hex("ffff00"))
static const int STATUS_NONE = 0;
static const int STATUS_SOLID = 1;
static const int STATUS_ONEWAY = 2;

/*
 * scripting_register_sensor()
 * Register this component
 */
void scripting_register_sensor(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Sensor", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Sensor", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Sensor", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Sensor", "__init", fun_init, 5);
    surgescript_vm_bind(vm, "Sensor", "get_zindex", fun_getzindex, 0);
    surgescript_vm_bind(vm, "Sensor", "render", fun_render, 0);
    surgescript_vm_bind(vm, "Sensor", "update", fun_update, 0);
    surgescript_vm_bind(vm, "Sensor", "set_visible", fun_setvisible, 1);
    surgescript_vm_bind(vm, "Sensor", "get_visible", fun_getvisible, 0);
    surgescript_vm_bind(vm, "Sensor", "get_status", fun_getstatus, 0);
}


/* private */

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);

    /* will create the sensor later */
    surgescript_object_set_userdata(object, NULL);

    /* the parent object can't be detached */
    if(surgescript_object_has_tag(parent, "detached"))
        fatal_error("Scripting Error: an object (\"%s\") that spawns a Sensor cannot be \"detached\".", scripting_util_parent_name(object));

    /* done! */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    sensor_t* sensor = get_sensor(object);
    if(sensor != NULL)
        sensor_destroy(sensor);
    return NULL;
}

/* main state; will check for collisions automatically once per frame */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    update(object);
    return NULL;
}

/* init function: x1, y1, x2, y2 are the sensor coordinates relative to the parent object */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    int x1 = (int)surgescript_var_get_number(param[0]);
    int y1 = (int)surgescript_var_get_number(param[1]);
    int x2 = (int)surgescript_var_get_number(param[2]);
    int y2 = (int)surgescript_var_get_number(param[3]);
    surgescript_objecthandle_t obstaclemap = surgescript_var_get_objecthandle(param[4]);
    sensor_t* sensor = get_sensor(object);

    /* can't call init twice */
    if(sensor != NULL)
        return NULL;

    /* store reference to the obstacle map */
    ssassert(OBSTACLEMAP_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, OBSTACLEMAP_ADDR), obstaclemap);
    ssassert(0 == strcmp("ObstacleMap", surgescript_object_name(surgescript_objectmanager_get(manager, obstaclemap))));

    /* create a new sensor */
    if(x1 == x2) {
        sensor = sensor_create_vertical(x1, y1, y2, SENSOR_COLOR());
        surgescript_object_set_userdata(object, sensor);
    }
    else if(y1 == y2) {
        sensor = sensor_create_horizontal(y1, x1, x2, SENSOR_COLOR());
        surgescript_object_set_userdata(object, sensor);
    }
    else
        fatal_error("Scripting Error: object \"%s\" spawns a Sensor with invalid coordinates.", scripting_util_parent_name(object));

    /* initial configuration */
    ssassert(VISIBLE_ADDR == surgescript_heap_malloc(heap));
    ssassert(STATUS_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_bool(surgescript_heap_at(heap, VISIBLE_ADDR), false);
    surgescript_var_set_number(surgescript_heap_at(heap, STATUS_ADDR), 0);

    /* done! */
    return NULL;
}

/* update the collision status AT THIS MOMENT IN TIME;
   useful if you move the object and, in the same frame, need to revalidate the collision status */
surgescript_var_t* fun_update(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    update(object);
    return NULL;
}

/* render */
surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    bool visible = surgescript_var_get_bool(surgescript_heap_at(heap, VISIBLE_ADDR));

    if(visible) {
        sensor_t* sensor = get_sensor(object);
        sensor_render(sensor, scripting_util_world_position(object), MM_FLOOR, parent_camera(object));
    }

    return NULL;
}

/* get zindex */
surgescript_var_t* fun_getzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), INFINITY);
}

/* set visibility */
surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    bool visible = surgescript_var_get_bool(param[0]);
    surgescript_var_set_bool(surgescript_heap_at(heap, VISIBLE_ADDR), visible);
    return NULL;
}

/* get visibility */
surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, VISIBLE_ADDR));
}

/* get status */
surgescript_var_t* fun_getstatus(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, STATUS_ADDR));
}




/* --- helpers --- */

/* get the obstacle map */
const obstaclemap_t* get_obstaclemap(const surgescript_object_t* object)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, OBSTACLEMAP_ADDR));
    surgescript_object_t* target = surgescript_objectmanager_get(manager, handle);
    obstaclemap_t* obstaclemap = (obstaclemap_t*)(surgescript_object_userdata(target));
    ssassert(NULL != obstaclemap);
    return obstaclemap;
}

/* get the sensor */
sensor_t* get_sensor(const surgescript_object_t* object)
{
    return (sensor_t*)(surgescript_object_userdata(object));
}

/* update the sensor */
void update(surgescript_object_t* object)
{
    sensor_t* sensor = get_sensor(object);
    const obstaclemap_t* obstaclemap = get_obstaclemap(object);
    const obstacle_t* obstacle = sensor_check(sensor, scripting_util_world_position(object), MM_FLOOR, obstaclemap);
    surgescript_heap_t* heap = surgescript_object_heap(object);

    if(obstacle != NULL) {
        if(obstacle_is_solid(obstacle))
            surgescript_var_set_number(surgescript_heap_at(heap, STATUS_ADDR), STATUS_SOLID);
        else
            surgescript_var_set_number(surgescript_heap_at(heap, STATUS_ADDR), STATUS_ONEWAY);
    }
    else
        surgescript_var_set_number(surgescript_heap_at(heap, STATUS_ADDR), STATUS_NONE);
}

/* get the camera of the parent object (is it detached?) */
v2d_t parent_camera(const surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    return scripting_util_object_camera(parent);
}