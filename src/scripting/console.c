/*
 * Open Surge Engine
 * console.c - scripting system: console object
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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
#include "../core/video.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_print(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_write(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_readline(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_console()
 * Register the engine-replacement for Console
 */
void scripting_register_console(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Console", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Console", "print", fun_print, 1);
    surgescript_vm_bind(vm, "Console", "write", fun_write, 1);
    surgescript_vm_bind(vm, "Console", "readline", fun_readline, 0);
}

/* Console routines */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_set_active(object, false);
    return NULL;
}

/* prints a message to the console */
surgescript_var_t* fun_print(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* message = surgescript_var_get_string(param[0], manager);
    video_showmessage("%s", message);
    ssfree(message);
    return NULL;
}

/* not implemented */
surgescript_var_t* fun_write(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return fun_print(object, param, num_params);
}

/* not implemented; returns null */
surgescript_var_t* fun_readline(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}