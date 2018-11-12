/*
 * Open Surge Engine
 * mouse.c - scripting system: mouse input
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

#include <surgescript.h>
#include <stdint.h>
#include <math.h>
#include "../core/video.h"
#include "../core/input.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getxpos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getypos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttondown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttonpressed(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttonreleased(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static uint64_t hash(const char *str);

/* button hashes: "left", "right", "middle" */
#define BUTTON_LEFT         0x17C9A03B0
#define BUTTON_RIGHT        0x3110494163
#define BUTTON_MIDDLE       0x6530DC5EBD4

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
    surgescript_vm_bind(vm, "Mouse", "get_xpos", fun_getxpos, 0);
    surgescript_vm_bind(vm, "Mouse", "get_ypos", fun_getypos, 0);
    surgescript_vm_bind(vm, "Mouse", "buttonDown", fun_buttondown, 1);
    surgescript_vm_bind(vm, "Mouse", "buttonPressed", fun_buttonpressed, 1);
    surgescript_vm_bind(vm, "Mouse", "buttonReleased", fun_buttonreleased, 1);
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
    input_t* input = input_create_mouse();
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

/* get x-position (in screen coordinates) */
surgescript_var_t* fun_getxpos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    float mul = video_get_screen_size().x / video_get_window_size().x;
    return surgescript_var_set_number(surgescript_var_create(), floor(input_get_xy((inputmouse_t*)input).x * mul));
}

/* get y-position (in screen coordinates) */
surgescript_var_t* fun_getypos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    float mul = video_get_screen_size().y / video_get_window_size().y;
    return surgescript_var_set_number(surgescript_var_create(), floor(input_get_xy((inputmouse_t*)input).y * mul));
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



/* -- private -- */

/* djb2 hash function */
uint64_t hash(const char *str)
{
    int c; uint64_t hash = 5381;

    while((c = *((unsigned char*)(str++))))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}