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
#include "../core/assetfs.h"
#include "../core/util.h"
#include "../core/stringutil.h"

/* utilities */
#include "../core/v2d.h"
#include "../core/video.h"
#include "../entities/camera.h"
#include "../scenes/level.h"

/* private area */
static surgescript_vm_t* vm = NULL;
static char** vm_argv = NULL;
static int vm_argc = 0;
static bool test_mode = false;
static void log_fun(const char* message);
static void err_fun(const char* message);
static int compile_script(const char* filepath, void* param);
static bool found_test_script(const surgescript_vm_t* vm);
static void check_if_compatible();

/* minimum required version */
static const char* min_surgescript_version = "0.5.3";

/* SurgeEngine */
extern void scripting_register_application(surgescript_vm_t* vm);
extern void scripting_register_surgeengine(surgescript_vm_t* vm);
extern void scripting_register_actor(surgescript_vm_t* vm);
extern void scripting_register_animation(surgescript_vm_t* vm);
extern void scripting_register_camera(surgescript_vm_t* vm);
extern void scripting_register_collisions(surgescript_vm_t* vm);
extern void scripting_register_console(surgescript_vm_t* vm);
extern void scripting_register_input(surgescript_vm_t* vm);
extern void scripting_register_level(surgescript_vm_t* vm);
extern void scripting_register_levelmanager(surgescript_vm_t* vm);
extern void scripting_register_mouse(surgescript_vm_t* vm);
extern void scripting_register_music(surgescript_vm_t* vm);
extern void scripting_register_obstaclemap(surgescript_vm_t* vm);
extern void scripting_register_player(surgescript_vm_t* vm);
extern void scripting_register_prefs(surgescript_vm_t* vm);
extern void scripting_register_sensor(surgescript_vm_t* vm);
extern void scripting_register_text(surgescript_vm_t* vm);
extern void scripting_register_time(surgescript_vm_t* vm);
extern void scripting_register_web(surgescript_vm_t* vm);

/*
 * scripting_init()
 * Initializes the scripting system
 */
void scripting_init(int argc, const char** argv)
{
    /* create VM */
    surgescript_util_set_error_functions(log_fun, err_fun);
    check_if_compatible();
    vm = surgescript_vm_create();

    /* copy command line arguments */
    vm_argv = mallocx((vm_argc = argc) * sizeof(*vm_argv));
    while(argc-- > 0)
        vm_argv[argc] = str_dup(argv[argc]);

    /* register SurgeEngine builtins */
    scripting_register_surgeengine(vm);
    scripting_register_actor(vm);
    scripting_register_animation(vm);
    scripting_register_camera(vm);
    scripting_register_collisions(vm);
    scripting_register_console(vm);
    scripting_register_input(vm);
    scripting_register_level(vm);
    scripting_register_levelmanager(vm);
    scripting_register_mouse(vm);
    scripting_register_music(vm);
    scripting_register_obstaclemap(vm);
    scripting_register_player(vm);
    scripting_register_prefs(vm);
    scripting_register_sensor(vm);
    scripting_register_text(vm);
    scripting_register_time(vm);
    scripting_register_web(vm);

    /* compile scripts */
    assetfs_foreach_file("scripts", ".ss", compile_script, NULL, true);

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
surgescript_objecthandle_t scripting_util_require_component(const surgescript_object_t* object, const char* component_name)
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
v2d_t scripting_util_world_position(const surgescript_object_t* object)
{
    /* this gotta be fast */
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t root = surgescript_objectmanager_root(manager);
    surgescript_objecthandle_t handle = surgescript_object_handle(object);
    surgescript_transform_t transform;
    v2d_t world_position;

    /* get local position */
    surgescript_object_peek_transform(object, &transform);
    world_position.x = transform.position.x;
    world_position.y = transform.position.y;

    /* compute world position */
    while(handle != root) {
        handle = surgescript_object_parent(object);
        object = surgescript_objectmanager_get(manager, handle);
        if(surgescript_object_transform_changed(object)) {
            surgescript_object_peek_transform(object, &transform);
            surgescript_transform_apply2d(&transform, &world_position.x, &world_position.y);
        }
    }

    /* done! */
    return world_position;
}

/* compute the world angle of an object */
float scripting_util_world_angle(const surgescript_object_t* object)
{
    surgescript_transform_t transform;
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, surgescript_object_parent(object));
    float parent_angle = (parent != object) ? scripting_util_world_angle(parent) : 0.0f;
    surgescript_object_peek_transform(object, &transform);
    return parent_angle + transform.rotation.z;
}

/* compute the proper camera position for an object (will check if it's detached or not) */
v2d_t scripting_util_object_camera(const surgescript_object_t* object)
{
    bool is_detached = surgescript_object_has_tag(object, "detached");
    return !is_detached ? camera_get_position() : v2d_new(VIDEO_SCREEN_W / 2, VIDEO_SCREEN_H / 2);
}

/* checks if the object is inside the visible part of the screen */
int scripting_util_is_object_inside_screen(const surgescript_object_t* object)
{
    v2d_t v = scripting_util_world_position(object);
    return level_inside_screen(v.x, v.y, 0, 0);
}

/* get the zindex of an object */
float scripting_util_object_zindex(surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_programpool_t* pool = surgescript_objectmanager_programpool(manager);
    const char* object_name = surgescript_object_name(object);
    float zindex = 0.5f;

    if(surgescript_programpool_exists(pool, object_name, "get_zindex")) {
        surgescript_var_t* tmp = surgescript_var_create();
        surgescript_object_call_function(object, "get_zindex", NULL, 0, tmp);
        zindex = surgescript_var_get_number(tmp);
        surgescript_var_destroy(tmp);
    }

    return zindex;
}

/* the name of the parent object */
const char* scripting_util_parent_name(const surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    return surgescript_object_name(parent);
}

/* get the SurgeEngine object */
surgescript_object_t* scripting_util_surgeengine_object(surgescript_vm_t* vm)
{
    surgescript_objectmanager_t* manager = surgescript_vm_objectmanager(vm);
    static surgescript_objecthandle_t cached_ref = 0;
    
    if(!cached_ref)
        cached_ref = surgescript_objectmanager_plugin_object(manager, "SurgeEngine");

    return surgescript_objectmanager_get(manager, cached_ref);
}

/* get a component of the SurgeEngine object */
surgescript_object_t* scripting_util_surgeengine_component(surgescript_vm_t* vm, const char* component_name)
{
    return scripting_util_get_component(scripting_util_surgeengine_object(vm), component_name);
}

/* get a component of an object (returns object.get_component()) */
surgescript_object_t* scripting_util_get_component(const surgescript_object_t* object, const char* component_name)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* accessor_fun = surgescript_util_accessorfun("get", component_name);
    surgescript_var_t* ret = surgescript_var_create();
    surgescript_objecthandle_t handle = 0;

    surgescript_object_call_function(object, accessor_fun, NULL, 0, ret);
    handle = surgescript_var_get_objecthandle(ret);

    surgescript_var_destroy(ret);
    ssfree(accessor_fun);
    return surgescript_objectmanager_get(manager, handle);
}


/* private stuff */

/* will check if the compiled SurgeScript version is compatible
   to this build of Open Surge */
void check_if_compatible()
{
    if(surgescript_util_versioncode(NULL) < surgescript_util_versioncode(min_surgescript_version))
        fatal_error("This build requires at least SurgeScript %s (using: %s)", min_surgescript_version, surgescript_util_version());
}

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
    const char* fullpath = assetfs_fullpath(filepath);

    /*logfile_message("Compiling '%s'...", filepath);*/
    if(surgescript_vm_compile(vm, fullpath))
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
