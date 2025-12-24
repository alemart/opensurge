/*
 * Open Surge Engine
 * platform.c - scripting system: platform data
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
#include <allegro5/allegro.h>

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

static surgescript_var_t* fun_getiswindows(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getisunix(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getismacos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getisandroid(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getisios(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getishtml(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

static surgescript_var_t* fun_getandroid(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* children's addresses */
static const surgescript_heapptr_t ANDROID_ADDR = 0;

/*
 * scripting_register_platform()
 * Register the Platform object
 */
void scripting_register_platform(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Platform", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Platform", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Platform", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Platform", "spawn", fun_spawn, 1);

    surgescript_vm_bind(vm, "Platform", "get_Android", fun_getandroid, 0);

    surgescript_vm_bind(vm, "Platform", "get_isWindows", fun_getiswindows, 0);
    surgescript_vm_bind(vm, "Platform", "get_isUnix", fun_getisunix, 0);
    surgescript_vm_bind(vm, "Platform", "get_isMacOS", fun_getismacos, 0);
    surgescript_vm_bind(vm, "Platform", "get_isAndroid", fun_getisandroid, 0);
    /*surgescript_vm_bind(vm, "Platform", "get_isIOS", fun_getisios, 0);
    surgescript_vm_bind(vm, "Platform", "get_isHTML", fun_getishtml, 0);*/

    (void)fun_getisios;
    (void)fun_getishtml;
}

/* Platform routines */

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);

    surgescript_objecthandle_t android = surgescript_objectmanager_spawn(manager, me, "AndroidPlatform", NULL);
    ssassert(ANDROID_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, ANDROID_ADDR), android);

    return NULL;
}

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
surgescript_var_t* fun_getiswindows(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
#if defined(_WIN32)
    bool result = true;
#else
    bool result = false;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on a Unix-style OS? e.g., Linux, macOS, BSD, Android, iOS, etc. */
surgescript_var_t* fun_getisunix(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
    bool result = true;
#else
    bool result = false;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on macOS or iOS? */
surgescript_var_t* fun_getismacos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
#if defined(__APPLE__) && defined(__MACH__)
    bool result = true;
#else
    bool result = false;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on Android? */
surgescript_var_t* fun_getisandroid(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
#if defined(__ANDROID__)
    bool result = true;
#else
    bool result = false;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on iOS? (currently unsupported) */
surgescript_var_t* fun_getisios(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
#if defined(ALLEGRO_IPHONE)
    bool result = true;
#else
    bool result = false;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}

/* is the engine currently running on HTML5? (currently unsupported) */
surgescript_var_t* fun_getishtml(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
#if defined(__EMSCRIPTEN__)
    bool result = true;
#else
    bool result = false;
#endif

    return surgescript_var_set_bool(surgescript_var_create(), result);
}




/* Android-specific routines */
surgescript_var_t* fun_getandroid(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ANDROID_ADDR));
}