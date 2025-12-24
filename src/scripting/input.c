/*
 * Open Surge Engine
 * input.c - scripting system: input object
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
#include <stdint.h>
#include "../util/djb2.h"
#include "../core/video.h"
#include "../core/input.h"
#include "../core/inputmap.h"

/* input API */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttondown(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttonpressed(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_buttonreleased(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_simulatebutton(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_remap(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* button hashes */
#define BUTTON_HASH(x)  djb2(x)
#define BUTTON_UP       DJB2_CONST('u','p')
#define BUTTON_DOWN     DJB2_CONST('d','o','w','n')
#define BUTTON_LEFT     DJB2_CONST('l','e','f','t')
#define BUTTON_RIGHT    DJB2_CONST('r','i','g','h','t')
#define BUTTON_FIRE1    DJB2_CONST('f','i','r','e','1')
#define BUTTON_FIRE2    DJB2_CONST('f','i','r','e','2')
#define BUTTON_FIRE3    DJB2_CONST('f','i','r','e','3')
#define BUTTON_FIRE4    DJB2_CONST('f','i','r','e','4')
#define BUTTON_FIRE5    DJB2_CONST('f','i','r','e','5')
#define BUTTON_FIRE6    DJB2_CONST('f','i','r','e','6')
#define BUTTON_FIRE7    DJB2_CONST('f','i','r','e','7')
#define BUTTON_FIRE8    DJB2_CONST('f','i','r','e','8')

/* misc */
static const surgescript_heapptr_t IS_OWN_INPUT_POINTER = 0;


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
    surgescript_vm_bind(vm, "Input", "simulateButton", fun_simulatebutton, 2);
    surgescript_vm_bind(vm, "Input", "get_enabled", fun_getenabled, 0);
    surgescript_vm_bind(vm, "Input", "set_enabled", fun_setenabled, 1);
    surgescript_vm_bind(vm, "Input", "__init", fun_init, 1);
    surgescript_vm_bind(vm, "Input", "remap", fun_remap, 1);
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
    surgescript_heap_t* heap = surgescript_object_heap(object);

    ssassert(IS_OWN_INPUT_POINTER == surgescript_heap_malloc(heap));
    surgescript_var_set_bool(surgescript_heap_at(heap, IS_OWN_INPUT_POINTER), false);

    /* We may accept an external an input_t* as this object's userdata */
    if(surgescript_object_userdata(object) == NULL) {
        input_t* input = input_create_user(NULL);
        surgescript_object_set_userdata(object, input);
        surgescript_var_set_bool(surgescript_heap_at(heap, IS_OWN_INPUT_POINTER), true);
    }

    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* Will destroy the input_t* only if we created it ourselves */
    if(surgescript_var_get_bool(surgescript_heap_at(heap, IS_OWN_INPUT_POINTER))) {
        input_t* input = (input_t*)surgescript_object_userdata(object);
        input_destroy(input);
    }

    return NULL;
}

/* __init(inputMapName): set an input map on initialization */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);

    if(!surgescript_var_is_null(param[0])) {
        if(surgescript_var_get_bool(surgescript_heap_at(heap, IS_OWN_INPUT_POINTER))) {
            input_t* input = (input_t*)surgescript_object_userdata(object);
            const char* inputmap = surgescript_var_fast_get_string(param[0]);
            if(*inputmap != '\0')
                input_change_mapping((inputuserdefined_t*)input, inputmap);
        }
    }

    return NULL;
}

/* remap(inputMapName): change the input mapping. Returns true on success */
surgescript_var_t* fun_remap(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_var_t* ret = surgescript_var_create();

    if(surgescript_var_is_null(param[0]))
        return surgescript_var_set_bool(ret, false);

    const char* inputmap = surgescript_var_fast_get_string(param[0]);
    if(!inputmap_exists(inputmap)) {
        video_showmessage("Input map \"%s\" doesn't exist", inputmap);
        return surgescript_var_set_bool(ret, false);
    }

    input_t* input = (input_t*)surgescript_object_userdata(object);
    input_change_mapping((inputuserdefined_t*)input, inputmap);
    return surgescript_var_set_bool(ret, true);
}

/* buttonDown(button): is the given button being held down?
 * valid buttons are: "up", "down", "left", "right", "fire1", "fire2", ..., "fire8"
 * for optimization reasons, it's mandatory: button must be of the string type */
surgescript_var_t* fun_buttondown(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    const char* button = surgescript_var_fast_get_string(param[0]);

    switch(BUTTON_HASH(button)) {
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

    switch(BUTTON_HASH(button)) {
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

    switch(BUTTON_HASH(button)) {
        case BUTTON_UP:     return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_UP));
        case BUTTON_DOWN:   return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_DOWN));
        case BUTTON_LEFT:   return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_LEFT));
        case BUTTON_RIGHT:  return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_RIGHT));
        case BUTTON_FIRE1:  return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_FIRE1));
        case BUTTON_FIRE2:  return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_FIRE2));
        case BUTTON_FIRE3:  return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_FIRE3));
        case BUTTON_FIRE4:  return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_FIRE4));
        case BUTTON_FIRE5:  return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_FIRE5));
        case BUTTON_FIRE6:  return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_FIRE6));
        case BUTTON_FIRE7:  return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_FIRE7));
        case BUTTON_FIRE8:  return surgescript_var_set_bool(surgescript_var_create(), input_button_released(input, IB_FIRE8));
        default:            return surgescript_var_set_bool(surgescript_var_create(), false);
    }
}

/* simulateButton(button, down): simulates that a button is being held down/not down
 * valid buttons are: "up", "down", "left", "right", "fire1", "fire2", ..., "fire8"
 * for optimization reasons, it's mandatory: button must be of the string type */
surgescript_var_t* fun_simulatebutton(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    const char* button = surgescript_var_fast_get_string(param[0]);
    bool down = surgescript_var_get_bool(param[1]);
    void (*simulate_button)(input_t*,inputbutton_t) = down ? input_simulate_button_down : input_simulate_button_up;

    switch(BUTTON_HASH(button)) {
        case BUTTON_UP:     simulate_button(input, IB_UP); break;
        case BUTTON_DOWN:   simulate_button(input, IB_DOWN); break;
        case BUTTON_LEFT:   simulate_button(input, IB_LEFT); break;
        case BUTTON_RIGHT:  simulate_button(input, IB_RIGHT); break;
        case BUTTON_FIRE1:  simulate_button(input, IB_FIRE1); break;
        case BUTTON_FIRE2:  simulate_button(input, IB_FIRE2); break;
        case BUTTON_FIRE3:  simulate_button(input, IB_FIRE3); break;
        case BUTTON_FIRE4:  simulate_button(input, IB_FIRE4); break;
        case BUTTON_FIRE5:  simulate_button(input, IB_FIRE5); break;
        case BUTTON_FIRE6:  simulate_button(input, IB_FIRE6); break;
        case BUTTON_FIRE7:  simulate_button(input, IB_FIRE7); break;
        case BUTTON_FIRE8:  simulate_button(input, IB_FIRE8); break;
    }

    return NULL;
}

/* is the input object enabled? */
surgescript_var_t* fun_getenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    return surgescript_var_set_bool(surgescript_var_create(), input_is_enabled(input));
}

/* enable or disable the input object */
surgescript_var_t* fun_setenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    input_t* input = (input_t*)surgescript_object_userdata(object);
    bool enabled = surgescript_var_get_bool(param[0]);

    if(enabled)
        input_enable(input);
    else
        input_disable(input);

    return NULL;
}
