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
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getxpos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getypos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setxpos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setypos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_camera()
 * Register the Camera object
 */
void scripting_register_camera(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Camera", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Camera", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Camera", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Camera", "get_xpos", fun_getxpos, 0);
    surgescript_vm_bind(vm, "Camera", "get_ypos", fun_getypos, 0);
    surgescript_vm_bind(vm, "Camera", "set_xpos", fun_setxpos, 1);
    surgescript_vm_bind(vm, "Camera", "set_ypos", fun_setypos, 1);
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

/* get camera xpos */
surgescript_var_t* fun_getxpos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), camera_get_position().x);
}

/* get camera ypos */
surgescript_var_t* fun_getypos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), camera_get_position().y);
}

/* set camera xpos */
surgescript_var_t* fun_setxpos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    float value = surgescript_var_get_number(param[0]);
    camera_set_position(v2d_new(value, camera_get_position().y));
    return NULL;
}

/* set camera ypos */
surgescript_var_t* fun_setypos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    float value = surgescript_var_get_number(param[0]);
    camera_set_position(v2d_new(camera_get_position().x, value));
    return NULL;
}