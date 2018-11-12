/*
 * Open Surge Engine
 * input.c - scripting system: input object
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
#include "../core/input.h"

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttondown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttonpressed(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttonreleased(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static uint64_t hash(const char *str);

/* button hashes: "up", "down", "left", "right", "fire1", "fire2", ..., "fire8" */
#define BUTTON_UP             0x5979CA        /* hash("up") */
#define BUTTON_DOWN           0x17C95CD5D     /* hash("down") */
#define BUTTON_LEFT           0x17C9A03B0     /* and so on... */
#define BUTTON_RIGHT          0x3110494163
#define BUTTON_FIRE1          0x310F70497C
#define BUTTON_FIRE2          0x310F70497D
#define BUTTON_FIRE3          0x310F70497E
#define BUTTON_FIRE4          0x310F70497F
#define BUTTON_FIRE5          0x310F704980
#define BUTTON_FIRE6          0x310F704981
#define BUTTON_FIRE7          0x310F704982
#define BUTTON_FIRE8          0x310F704983


/*
 * scripting_register_input()
 * Register the object
 */
void scripting_register_input(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Input", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Input", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Input", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Input", "buttonDown", fun_buttondown, 1);
    surgescript_vm_bind(vm, "Input", "buttonPressed", fun_buttonpressed, 1);
    surgescript_vm_bind(vm, "Input", "buttonReleased", fun_buttonreleased, 1);
    surgescript_vm_bind(vm, "Input", "__init", fun_init, 1);
}

/* Console routines */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = input_create_user(NULL);
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

/* __init(inputMap): set an input map */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    if(!surgescript_var_is_null(param[0])) {
        input_t* input = (input_t*)surgescript_object_userdata(object);
        const char* inputmap = surgescript_var_fast_get_string(param[0]);
        if(*inputmap != 0)
            input_change_mapping((inputuserdefined_t*)input, inputmap);
    }
    return NULL;
}

/* buttonDown(button): is the given button being held down?
 * valid buttons are: "up", "down", "left", "right", "fire1", "fire2", ..., "fire8"
 * for optimization reasons, it's mandatory: button must be of the string type */
surgescript_var_t* fun_buttondown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    const char* button = surgescript_var_fast_get_string(param[0]);
    switch(hash(button)) {
        case BUTTON_UP:     return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_UP));
        case BUTTON_DOWN:   return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_DOWN));
        case BUTTON_LEFT:   return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_LEFT));
        case BUTTON_RIGHT:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_RIGHT));
        case BUTTON_FIRE1:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE1));
        case BUTTON_FIRE2:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE2));
        case BUTTON_FIRE3:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE3));
        case BUTTON_FIRE4:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE4));
        case BUTTON_FIRE5:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE5));
        case BUTTON_FIRE6:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE6));
        case BUTTON_FIRE7:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE7));
        case BUTTON_FIRE8:  return surgescript_var_set_bool(surgescript_var_create(), input_button_down(input, IB_FIRE8));
        default:            return surgescript_var_set_bool(surgescript_var_create(), false);
    }
}

/* buttonPressed(button): has the given button just been pressed?
 * valid buttons are: "up", "down", "left", "right", "fire1", "fire2", ..., "fire8"
 * for optimization reasons, it's mandatory: button must be of the string type */
surgescript_var_t* fun_buttonpressed(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    const char* button = surgescript_var_fast_get_string(param[0]);
    switch(hash(button)) {
        case BUTTON_UP:     return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_UP));
        case BUTTON_DOWN:   return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_DOWN));
        case BUTTON_LEFT:   return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_LEFT));
        case BUTTON_RIGHT:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_RIGHT));
        case BUTTON_FIRE1:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE1));
        case BUTTON_FIRE2:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE2));
        case BUTTON_FIRE3:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE3));
        case BUTTON_FIRE4:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE4));
        case BUTTON_FIRE5:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE5));
        case BUTTON_FIRE6:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE6));
        case BUTTON_FIRE7:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE7));
        case BUTTON_FIRE8:  return surgescript_var_set_bool(surgescript_var_create(), input_button_pressed(input, IB_FIRE8));
        default:            return surgescript_var_set_bool(surgescript_var_create(), false);
    }
}

/* buttonReleased(button): has the given button just been released?
 * valid buttons are: "up", "down", "left", "right", "fire1", "fire2", ..., "fire8"
 * for optimization reasons, it's mandatory: button must be of the string type */
surgescript_var_t* fun_buttonreleased(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    const char* button = surgescript_var_fast_get_string(param[0]);
    switch(hash(button)) {
        case BUTTON_UP:     return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_UP));
        case BUTTON_DOWN:   return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_DOWN));
        case BUTTON_LEFT:   return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_LEFT));
        case BUTTON_RIGHT:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_RIGHT));
        case BUTTON_FIRE1:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE1));
        case BUTTON_FIRE2:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE2));
        case BUTTON_FIRE3:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE3));
        case BUTTON_FIRE4:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE4));
        case BUTTON_FIRE5:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE5));
        case BUTTON_FIRE6:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE6));
        case BUTTON_FIRE7:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE7));
        case BUTTON_FIRE8:  return surgescript_var_set_bool(surgescript_var_create(), input_button_up(input, IB_FIRE8));
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