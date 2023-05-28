/*
 * Open Surge Engine
 * scripting.c - scripting system
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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

#include <stdarg.h>
#include "scripting.h"
#include "../core/global.h"
#include "../core/logfile.h"
#include "../core/asset.h"
#include "../core/v2d.h"
#include "../core/video.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../entities/camera.h"
#include "../scenes/level.h"

/* private area */
static surgescript_vm_t* vm = NULL;
static char** vm_argv = NULL;
static int vm_argc = 0;
static bool test_mode = false;
static int pause_counter = 0;
static void log_fun(const char* message);
static void err_fun(const char* message);
static void compile_scripts(surgescript_vm_t* vm);
static int compile_script(const char* filepath, void* param);
static bool found_test_script(const surgescript_vm_t* vm);
static void check_if_compatible();
static char* read_file(const char* filepath);
static void parse_surgescript_options(surgescript_vm_t* vm, int argc, char** argv);

/* SurgeEngine */
static void setup_surgeengine(surgescript_vm_t* vm);
extern void scripting_register_application(surgescript_vm_t* vm);
extern void scripting_register_surgeengine(surgescript_vm_t* vm);
extern void scripting_register_actor(surgescript_vm_t* vm);
extern void scripting_register_androidplatform(surgescript_vm_t* vm);
extern void scripting_register_animation(surgescript_vm_t* vm);
extern void scripting_register_brick(surgescript_vm_t* vm);
extern void scripting_register_camera(surgescript_vm_t* vm);
extern void scripting_register_collisions(surgescript_vm_t* vm);
extern void scripting_register_console(surgescript_vm_t* vm);
extern void scripting_register_entitycontainer(surgescript_vm_t* vm);
extern void scripting_register_entitymanager(surgescript_vm_t* vm);
extern void scripting_register_entitytree(surgescript_vm_t* vm);
extern void scripting_register_events(surgescript_vm_t* vm);
extern void scripting_register_input(surgescript_vm_t* vm);
extern void scripting_register_lang(surgescript_vm_t* vm);
extern void scripting_register_level(surgescript_vm_t* vm);
extern void scripting_register_levelmanager(surgescript_vm_t* vm);
extern void scripting_register_levelobjectcontainer(surgescript_vm_t* vm);
extern void scripting_register_mobilegamepad(surgescript_vm_t* vm);
extern void scripting_register_mouse(surgescript_vm_t* vm);
extern void scripting_register_music(surgescript_vm_t* vm);
extern void scripting_register_obstaclemap(surgescript_vm_t* vm);
extern void scripting_register_platform(surgescript_vm_t* vm);
extern void scripting_register_player(surgescript_vm_t* vm);
extern void scripting_register_playermanager(surgescript_vm_t* vm);
extern void scripting_register_prefs(surgescript_vm_t* vm);
extern void scripting_register_screen(surgescript_vm_t* vm);
extern void scripting_register_sensor(surgescript_vm_t* vm);
extern void scripting_register_sound(surgescript_vm_t* vm);
extern void scripting_register_text(surgescript_vm_t* vm);
extern void scripting_register_time(surgescript_vm_t* vm);
extern void scripting_register_transform(surgescript_vm_t* vm);
extern void scripting_register_vector2(surgescript_vm_t* vm);
extern void scripting_register_video(surgescript_vm_t* vm);
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

    /* parse special command-line options that affect the SurgeScript runtime */
    parse_surgescript_options(vm, vm_argc, vm_argv);

    /* register SurgeEngine builtins */
    setup_surgeengine(vm);

    /* compile scripts */
    compile_scripts(vm);

    /* launch VM */
    surgescript_vm_launch_ex(vm, vm_argc, vm_argv);
}

/*
 * scripting_release()
 * Releases the scripting system
 */
void scripting_release()
{
    surgescript_objectmanager_t* manager = surgescript_vm_objectmanager(vm);
    surgescript_objecthandle_t app_handle = surgescript_objectmanager_application(manager);
    surgescript_object_t* app = surgescript_objectmanager_get(manager, app_handle);
    const char* CALL_EXIT_FUNCTOR = "__callExitFunctor";

    /* call exit handler (similar to stdlib's atexit()) */
    if(surgescript_object_has_function(app, CALL_EXIT_FUNCTOR))
        surgescript_object_call_function(app, CALL_EXIT_FUNCTOR, NULL, 0, NULL);

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

/*
 * scripting_reload()
 * Reloads the entire scripting system,
 * clearing up all the scripts & objects
 */
void scripting_reload()
{
    logfile_message("Reloading scripts...");

    /* reset the SurgeScript VM */
    if(!surgescript_vm_reset(vm)) {
        logfile_message("Failed to reload the scripts");
        return;
    }

    /* parse special command-line options that affect the SurgeScript runtime */
    parse_surgescript_options(vm, vm_argc, vm_argv);

    /* register SurgeEngine builtins */
    setup_surgeengine(vm);

    /* compile scripts */
    compile_scripts(vm);

    /* launch VM */
    surgescript_vm_launch_ex(vm, vm_argc, vm_argv);

    /* done */
    logfile_message("The scripts have been reloaded!");
}

/*
 * scripting_pause_vm()
 * Pause the SurgeScript VM
 */
void scripting_pause_vm()
{
    if(pause_counter++ == 0) {
        logfile_message("Pausing the SurgeScript VM");
        surgescript_vm_pause(vm);
    }
}

/*
 * scripting_resume_vm()
 * Unpause the SurgeScript VM
 */
void scripting_resume_vm()
{
    if(--pause_counter == 0) {
        logfile_message("Resuming the SurgeScript VM");
        surgescript_vm_resume(vm);
    }

    pause_counter = max(pause_counter, 0); /* safeguard */
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
    v2d_t position;
    surgescript_transform_util_worldposition2d(object, &position.x, &position.y);
    return position;
}

/* compute the world angle of an object */
float scripting_util_world_angle(const surgescript_object_t* object)
{
    return surgescript_transform_util_worldangle2d(object);
}

/* set the world position of an object (teleport) */
void scripting_util_set_world_position(surgescript_object_t* object, v2d_t position)
{
    surgescript_transform_util_setworldposition2d(object, position.x, position.y);
}

/* set the world angle of an object (in degrees) */
void scripting_util_set_world_angle(surgescript_object_t* object, float angle)
{
    surgescript_transform_util_setworldangle2d(object, angle);
}

/* compute the proper camera position of an object (will check if it's detached or not) */
v2d_t scripting_util_object_camera(const surgescript_object_t* object)
{
    bool is_detached = surgescript_object_has_tag(object, "detached");
    return !is_detached ? camera_get_position() : v2d_new(VIDEO_SCREEN_W / 2, VIDEO_SCREEN_H / 2);
}

/* compute the camera position of the parent of an object */
v2d_t scripting_util_parent_camera(const surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    return scripting_util_object_camera(parent);
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
surgescript_object_t* scripting_util_get_component(surgescript_object_t* object, const char* component_name)
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

/* spawns an object as a child of System.__Temp */
surgescript_object_t* scripting_util_spawn_temp(surgescript_vm_t* vm, const char* object_name)
{
    surgescript_objectmanager_t* manager = surgescript_vm_objectmanager(vm);
    surgescript_objecthandle_t handle = surgescript_objectmanager_spawn_temp(manager, object_name);
    return surgescript_objectmanager_get(manager, handle);
}

/* display a scripting error and crash the application */
void scripting_error(const surgescript_object_t* object, const char* fmt, ...)
{
    const char* object_name = surgescript_object_name(object);
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    fatal_error("A scripting error was triggered in \"%s\".\n\n%s", object_name, buf);
}

/* display a scripting error without crashing the application */
void scripting_warning(const surgescript_object_t* object, const char* fmt, ...)
{
    const char* object_name = surgescript_object_name(object);
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    video_showmessage("%s: %s", object_name, buf);
    logfile_message("A scripting warning was triggered in \"%s\": %s", object_name, buf);
}


/* private stuff */

/* will check if the compiled SurgeScript version is compatible
   to this build of Open Surge */
void check_if_compatible()
{
    if(surgescript_util_versioncode(NULL) < surgescript_util_versioncode(SURGESCRIPT_MIN_VERSION))
        fatal_error("This build requires at least SurgeScript %s (using: %s)", SURGESCRIPT_MIN_VERSION, surgescript_util_version());
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

/* register SurgeEngine builtins */
void setup_surgeengine(surgescript_vm_t* vm)
{
    /* first, we setup the coordinate system for SurgeScript */
    surgescript_transform_use_inverted_y(true);

    /* next, we register the SurgeEngine builtins */
    scripting_register_surgeengine(vm);
    scripting_register_actor(vm);
    scripting_register_androidplatform(vm);
    scripting_register_animation(vm);
    scripting_register_brick(vm);
    scripting_register_camera(vm);
    scripting_register_collisions(vm);
    scripting_register_console(vm);
    scripting_register_entitycontainer(vm);
    scripting_register_entitymanager(vm);
    scripting_register_entitytree(vm);
    scripting_register_events(vm);
    scripting_register_input(vm);
    scripting_register_lang(vm);
    scripting_register_level(vm);
    scripting_register_levelmanager(vm);
    scripting_register_levelobjectcontainer(vm);
    scripting_register_mobilegamepad(vm);
    scripting_register_mouse(vm);
    scripting_register_music(vm);
    scripting_register_obstaclemap(vm);
    scripting_register_platform(vm);
    scripting_register_player(vm);
    scripting_register_playermanager(vm);
    scripting_register_prefs(vm);
    scripting_register_screen(vm);
    scripting_register_sensor(vm);
    scripting_register_sound(vm);
    scripting_register_text(vm);
    scripting_register_time(vm);
    scripting_register_transform(vm);
    scripting_register_vector2(vm);
    scripting_register_video(vm);
    scripting_register_web(vm);
}

/* compiles all .ss scripts from the scripts/ folder */
void compile_scripts(surgescript_vm_t* vm)
{
    /* compile scripts */
    asset_foreach_file("scripts", ".ss", compile_script, NULL, true);

    /* if no test script is present... */
    if(found_test_script(vm)) {
        logfile_message("Got a test script...");
        test_mode = true;
    }
    else {
        scripting_register_application(vm);
        test_mode = false;
    }
}

/* reads and compiles a script */
int compile_script(const char* filepath, void* param)
{
    const char* fullpath = asset_path(filepath);

    /* read script */
    char* script = read_file(fullpath);
    if(script == NULL)
        return -1;

    /* compile script */
    bool success = surgescript_vm_compile_virtual_file(vm, script, fullpath);

    /* done! */
    free(script);
    return success ? 0 : -1;
}

/* do we have a test script? (that is, did the user write his/her own "Application" object?) */
bool found_test_script(const surgescript_vm_t* vm)
{
    surgescript_programpool_t* pool = surgescript_vm_programpool(vm);
    return surgescript_programpool_exists(pool, "Application", "state:main");
}

/* reads a file using Allegro's File I/O interface */
char* read_file(const char* filepath)
{
    const size_t BUFSIZE = 4096;
    size_t read_chars = 0, data_size = 0;
    char* data = NULL;

    /* open the file in binary mode, so that offsets don't get messed up */
    ALLEGRO_FILE* fp = al_fopen(filepath, "rb");
    if(!fp) {
        fatal_error("Can't read file \"%s\". errno = %d", filepath, al_get_errno());
        return NULL;
    }

    /* read file to data[] */
    logfile_message("Reading script %s...", filepath);
    do {
        data_size += BUFSIZE;
        data = reallocx(data, data_size + 1);
        read_chars += al_fread(fp, data + read_chars, BUFSIZE);
        data[read_chars] = '\0';
    } while(read_chars == data_size);
    al_fclose(fp);

    /* success! */
    return data;
}

/* parse special command-line options that affect the SurgeScript runtime */
void parse_surgescript_options(surgescript_vm_t* vm, int argc, char** argv)
{
    for(int i = 0; i < argc; i++) {
        logfile_message("Found SurgeScript option %s", argv[i]);

        if(strcmp(argv[i], "--ss-allow-duplicates") == 0) {
            surgescript_parser_t* parser = surgescript_vm_parser(vm);
            surgescript_parser_set_flags(parser, SSPARSER_ALLOW_DUPLICATES);
        }
        else if(strcmp(argv[i], "--ss-skip-duplicates") == 0) {
            surgescript_parser_t* parser = surgescript_vm_parser(vm);
            surgescript_parser_set_flags(parser, SSPARSER_SKIP_DUPLICATES);
        }
    }
}