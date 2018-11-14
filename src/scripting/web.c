/*
 * Open Surge Engine
 * camera.c - scripting system: web routines
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
#include "../core/web.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_launchurl(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_web()
 * Register the Web object
 */
void scripting_register_web(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Camera", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Camera", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Camera", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Camera", "launchURL", fun_launchurl, 1);
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
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

/* launch URL: give a URL */
surgescript_var_t* fun_launchurl(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* url = surgescript_var_get_string(param[0], manager);
    launch_url(url);
    ssfree(url);
    return NULL;
}