/*
 * Open Surge Engine
 * video.c - scripting system: video component
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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
#include "../core/video.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getscreen(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfullscreen(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setfullscreen(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t SCREEN_ADDR = 0;

/*
 * scripting_register_video()
 * Register this component
 */
void scripting_register_video(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Video", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Video", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Video", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Video", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Video", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Video", "get_Screen", fun_getscreen, 0);
    surgescript_vm_bind(vm, "Video", "get_fullscreen", fun_getfullscreen, 0);
    surgescript_vm_bind(vm, "Video", "set_fullscreen", fun_setfullscreen, 1);
    surgescript_vm_bind(vm, "Video", "get_mode", fun_getmode, 0);
    surgescript_vm_bind(vm, "Video", "set_mode", fun_setmode, 1);
}


/* private */

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);

    /* allocate variables */
    ssassert(SCREEN_ADDR == surgescript_heap_malloc(heap));

    /* internal data */
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, SCREEN_ADDR),
        surgescript_objectmanager_spawn(manager, me, "Screen", NULL)
    );

    /* done! */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
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

/* get the Screen object */
surgescript_var_t* fun_getscreen(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, SCREEN_ADDR));
}

/* is the engine running on fullscreen mode? */
surgescript_var_t* fun_getfullscreen(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bool fullscreen = video_is_fullscreen();
    return surgescript_var_set_bool(surgescript_var_create(), fullscreen);
}

/* enable/disable the fullscreen mode */
surgescript_var_t* fun_setfullscreen(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bool fullscreen = surgescript_var_get_bool(param[0]);
    video_set_fullscreen(fullscreen);
    return NULL;
}

/* get the current Video mode */
surgescript_var_t* fun_getmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* mode = "";

    switch(video_get_mode()) {
        case VIDEOMODE_DEFAULT:
            mode = "default";
            break;

        case VIDEOMODE_FILL:
            mode = "fill";
            break;

        case VIDEOMODE_BESTFIT:
            mode = "best-fit";
            break;
    }

    return surgescript_var_set_string(surgescript_var_create(), mode);
}

/* set the current Video mode */
surgescript_var_t* fun_setmode(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* mode = surgescript_var_fast_get_string(param[0]);

    if(strcmp(mode, "default") == 0)
        video_set_mode(VIDEOMODE_DEFAULT);
    else if(strcmp(mode, "fill") == 0)
        video_set_mode(VIDEOMODE_FILL);
    else if(strcmp(mode, "best-fit") == 0)
        video_set_mode(VIDEOMODE_BESTFIT);

    return NULL;
}
