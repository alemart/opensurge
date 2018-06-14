/*
 * Open Surge Engine
 * surgescript.c - scripting system
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

#include "scripting.h"
#include "../core/logfile.h"
#include "../core/osspec.h"
#include "../core/util.h"
#include "../core/stringutil.h"

/* private area */
static surgescript_vm_t* vm = NULL;
static char** vm_argv = NULL;
static int vm_argc = 0;
static void log_fun(const char* message);
static void err_fun(const char* message);
static int compile_script(const char* filepath, void* param);

/* SurgeEngine */
extern void scripting_register_surgeengine(surgescript_vm_t* vm);
extern void scripting_register_actor(surgescript_vm_t* vm);

/*
 * scripting_init()
 * Initializes the scripting system
 */
void scripting_init(int argc, const char** argv)
{
    /* create VM */
    const char* path = "scripts/*.ss";
    surgescript_util_set_error_functions(log_fun, err_fun);
    vm = surgescript_vm_create();

    /* copy command line arguments */
    vm_argv = mallocx((vm_argc = argc) * sizeof(*vm_argv));
    while(argc-- > 0)
        vm_argv[argc] = str_dup(argv[argc]);

    /* register SurgeEngine builtins */
    scripting_register_surgeengine(vm);
    scripting_register_actor(vm);

    /* compile scripts */
    foreach_resource(path, compile_script, NULL, TRUE);

    /* launch VM */
    surgescript_vm_launch_ex(vm, vm_argc, vm_argv);
}

/*
 * scripting_release()
 * Releases the scripting system
 */
void scripting_release()
{
    /* release command line arguments */
    while(vm_argc-- > 0)
        free(vm_argv[vm_argc]);
    free(vm_argv);

    /* destroy VM */
    vm = surgescript_vm_destroy(vm);
}

/*
 * surgescript_vm()
 * Gets the SurgeScript VM
 */
surgescript_vm_t* surgescript_vm()
{
    return vm;
}


/* utilities */

/* get a component of the parent object */
surgescript_objecthandle_t require_component(const surgescript_object_t* object, const char* component_name)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    surgescript_objecthandle_t component = surgescript_object_child(parent, component_name);

    if(component == surgescript_objectmanager_null(manager))
        component = surgescript_objectmanager_spawn(manager, parent_handle, component_name, NULL);

    return component;
}



/* private stuff */

/* log function */
void log_fun(const char* message)
{
    logfile_message("%s", message);
}

/* scripting error */
void err_fun(const char* message)
{
    fatal_error("%s", message);
}

/* compiles a script */
int compile_script(const char* filepath, void* param)
{
    logfile_message("Compiling '%s'...", filepath);
    if(surgescript_vm_compile(vm, filepath))
        return 0;
    else
        return -1; /* error */
}