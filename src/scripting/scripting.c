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

/* utilities */
#include "../core/v2d.h"
#include "../core/video.h"
#include "../entities/camera.h"

/* private area */
static surgescript_vm_t* vm = NULL;
static char** vm_argv = NULL;
static int vm_argc = 0;
static bool test_mode = false;
static void log_fun(const char* message);
static void err_fun(const char* message);
static int compile_script(const char* filepath, void* param);
static bool found_test_script(const surgescript_vm_t* vm);

/* SurgeEngine */
extern void scripting_register_application(surgescript_vm_t* vm);
extern void scripting_register_surgeengine(surgescript_vm_t* vm);
extern void scripting_register_actor(surgescript_vm_t* vm);
extern void scripting_register_camera(surgescript_vm_t* vm);
extern void scripting_register_console(surgescript_vm_t* vm);
extern void scripting_register_obstaclemap(surgescript_vm_t* vm);
extern void scripting_register_sensor(surgescript_vm_t* vm);
extern void scripting_register_time(surgescript_vm_t* vm);
extern void scripting_register_mouse(surgescript_vm_t* vm);

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
    scripting_register_camera(vm);
    scripting_register_console(vm);
    scripting_register_obstaclemap(vm);
    scripting_register_sensor(vm);
    scripting_register_time(vm);
    scripting_register_mouse(vm);

    /* compile scripts */
    foreach_resource(path, compile_script, NULL, TRUE);

    /* if no test script is present... */
    if(found_test_script(vm)) {
        logfile_message("Got a test script...");
        test_mode = true;
    }
    else {
        scripting_register_application(vm);
        test_mode = false;
    }

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


/*
 * scripting_testmode()
 * Are we in test mode?
 */
bool scripting_testmode()
{
    return test_mode;
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

/* compute the world position of an object */
v2d_t world_position(const surgescript_object_t* object)
{
    surgescript_transform_t transform;
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, surgescript_object_parent(object));
    v2d_t parent_origin = (parent != object) ? world_position(parent) : v2d_new(0, 0);
    surgescript_object_peek_transform(object, &transform);
    surgescript_transform_apply2d(&transform, &parent_origin.x, &parent_origin.y);
    return parent_origin;
}

/* compute the proper camera position for an object (will check if it's detached or not) */
v2d_t object_camera(const surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    bool is_detached = surgescript_object_has_tag(parent, "detached");
    return !is_detached ? camera_get_position() : v2d_new(VIDEO_SCREEN_W / 2, VIDEO_SCREEN_H / 2);
}

/* the name of the parent object */
const char* parent_name(const surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    return surgescript_object_name(parent);
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

/* do we have a test script? (that is, did the user write his/her own "Application" object?) */
bool found_test_script(const surgescript_vm_t* vm)
{
    surgescript_programpool_t* pool = surgescript_vm_programpool(vm);
    return surgescript_programpool_exists(pool, "Application", "state:main");
}