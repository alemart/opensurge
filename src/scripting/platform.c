/*
 * Open Surge Engine
 * platform.c - scripting system: platform data
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

#include <surgescript.h>
#include <allegro5/allegro.h>

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getwindows(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getunix(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmacos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getandroid(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getios(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gethtml(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_platform()
 * Register the Platform object
 */
void scripting_register_platform(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Platform", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Platform", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Platform", "spawn", fun_spawn, 1);

    surgescript_vm_bind(vm, "Platform", "get_windows", fun_getwindows, 0);
    surgescript_vm_bind(vm, "Platform", "get_unix", fun_getunix, 0);
    surgescript_vm_bind(vm, "Platform", "get_macos", fun_getmacos, 0);
    surgescript_vm_bind(vm, "Platform", "get_android", fun_getandroid, 0);
    /*surgescript_vm_bind(vm, "Platform", "get_ios", fun_getios, 0);
    surgescript_vm_bind(vm, "Platform", "get_html", fun_gethtml, 0);*/

    (void)fun_getios;
    (void)fun_gethtml;
}

/* Platform routines */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_set_active(object, false);
    return NULL;
}

/* destroy */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* spawn */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* is the engine currently running on Microsoft Windows? */
surgescript_var_t* fun_getwindows(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bool result = false;

#if defined(_WIN32)
    result = true;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on a Unix-style OS? e.g., Linux, macOS, BSD, Android, iOS, etc. */
surgescript_var_t* fun_getunix(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bool result = false;

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
    result = true;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on macOS or iOS? */
surgescript_var_t* fun_getmacos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bool result = false;

#if defined(__APPLE__) && defined(__MACH__)
    result = true;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on Android? */
surgescript_var_t* fun_getandroid(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bool result = false;

#if defined(__ANDROID__)
    result = true;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on iOS? (currently unsupported) */
surgescript_var_t* fun_getios(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bool result = false;

#if defined(ALLEGRO_IPHONE)
    result = true;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on HTML5? (currently unsupported) */
surgescript_var_t* fun_gethtml(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bool result = false;

#if defined(__EMSCRIPTEN__)
    result = true;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}