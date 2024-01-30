/*
 * Open Surge Engine
 * camera.c - scripting system: web routines
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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
#include "../core/web.h"
#include "../util/util.h"

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
    surgescript_vm_bind(vm, "Web", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Web", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Web", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Web", "launchURL", fun_launchurl, 1);
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

/* launch URL: give a URL */
surgescript_var_t* fun_launchurl(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* url = surgescript_var_get_string(param[0], manager);

    if(!(strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0 || strncmp(url, "mailto:", 7) == 0)) {
        if(strstr(url, "://") != NULL)
            scripting_warning(object, "Can't launch URL. Unsupported protocol for %s", url);
        else
            scripting_warning(object, "Can't launch URL. Please specify a protocol (e.g., https://) to launch %s", url);
    }
    else
        launch_url(url);

    ssfree(url);
    return NULL;
}