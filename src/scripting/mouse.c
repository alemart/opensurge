/*
 * Open Surge Engine
 * mouse.c - scripting system: mouse input
 * Copyright (C) 2018, 2019  Alexandre Martins <alemartf@gmail.com>
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
#include <stdint.h>
#include <math.h>
#include "scripting.h"
#include "../core/video.h"
#include "../core/input.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttondown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttonpressed(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttonreleased(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getscrollup(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getscrolldown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static uint64_t hash(const char *str);
static const surgescript_heapptr_t POSITION_ADDR = 0;

/* button hashes */
#define BUTTON_LEFT         UINT64_C(0x17C9A03B0)         /* hash("left") */
#define BUTTON_RIGHT        UINT64_C(0x3110494163)        /* hash("right") */
#define BUTTON_MIDDLE       UINT64_C(0x6530DC5EBD4)       /* hash("middle") */

/*
 * scripting_register_mouse()
 * Register the object
 */
void scripting_register_mouse(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Mouse", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Mouse", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Mouse", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Mouse", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Mouse", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Mouse", "buttonDown", fun_buttondown, 1);
    surgescript_vm_bind(vm, "Mouse", "buttonPressed", fun_buttonpressed, 1);
    surgescript_vm_bind(vm, "Mouse", "buttonReleased", fun_buttonreleased, 1);
    surgescript_vm_bind(vm, "Mouse", "get_position", fun_getposition, 0);
    surgescript_vm_bind(vm, "Mouse", "get_scrollUp", fun_getscrollup, 0);
    surgescript_vm_bind(vm, "Mouse", "get_scrollDown", fun_getscrolldown, 0);
}

/* Console routines */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_set_active(object, false);
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t position = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
    input_t* input = input_create_mouse();

    ssassert(POSITION_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, POSITION_ADDR), position);
    surgescript_object_set_userdata(object, input);

    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    input_destroy(input);
    return NULL;
}

/* spawn function */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* destroy function */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* get cursor position (in screen coordinates) */
surgescript_var_t* fun_getposition(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, POSITION_ADDR));
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, handle);
    v2d_t screen_size = video_get_screen_size();
    v2d_t window_size = video_get_window_size();
    v2d_t m = v2d_new(screen_size.x / window_size.x, screen_size.y / window_size.y);
    v2d_t pos = input_get_xy((inputmouse_t*)surgescript_object_userdata(object));

    scripting_vector2_update(v2, floor(pos.x * m.x), floor(pos.y * m.y));

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* buttonDown(button): is the given button being held down?
 * valid button values are: "left", "right", "middle"
 * for optimization reasons, it's mandatory: button must be of the string type */
surgescript_var_t* fun_buttondown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    const char* button = surgescript_var_fast_get_string(param[0]);
    switch(hash(button)) {
        case BUTTON_LEFT:   return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE1));
        case BUTTON_RIGHT:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE2));
        case BUTTON_MIDDLE: return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE3));
        default:            return surgescript_var_set_bool(surgescript_var_create(), false);
    }
}

/* buttonPressed(button): has the given button just been pressed?
 * valid button values are: "left", "right", "middle"
 * for optimization reasons, it's mandatory: button must be of the string type */
surgescript_var_t* fun_buttonpressed(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    const char* button = surgescript_var_fast_get_string(param[0]);
    switch(hash(button)) {
        case BUTTON_LEFT:   return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE1));
        case BUTTON_RIGHT:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE2));
        case BUTTON_MIDDLE: return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE3));
        default:            return surgescript_var_set_bool(surgescript_var_create(), false);
    }
}

/* buttonReleased(button): has the given button just been released?
 * valid button values are: "left", "right", "middle"
 * for optimization reasons, it's mandatory: button must be of the string type */
surgescript_var_t* fun_buttonreleased(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    const char* button = surgescript_var_fast_get_string(param[0]);
    switch(hash(button)) {
        case BUTTON_LEFT:   return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE1));
        case BUTTON_RIGHT:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE2));
        case BUTTON_MIDDLE: return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE3));
        default:            return surgescript_var_set_bool(surgescript_var_create(), false);
    }
}

/* Will be true when the user scrolls up with the mouse wheel */
surgescript_var_t* fun_getscrollup(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_UP));
}

/* Will be true when the user scrolls down with the mouse wheel */
surgescript_var_t* fun_getscrolldown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_DOWN));
}



/* -- private -- */

/* djb2 hash function */
uint64_t hash(const char *str)
{
    int c; uint64_t hash = 5381;

    while((c = *((unsigned char*)(str++))))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}