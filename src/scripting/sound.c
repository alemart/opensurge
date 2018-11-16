/*
 * Open Surge Engine
 * sound.c - scripting system: sound
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
#include "../core/audio.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_play(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_stop(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getplaying(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static inline sound_t* get_sound(const surgescript_object_t* object);

/*
 * scripting_register_sound()
 * Register the Sound object
 */
void scripting_register_sound(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Sound", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Sound", "__init", fun_init, 1);
    surgescript_vm_bind(vm, "Sound", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Sound", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Sound", "play", fun_play, 0);
    surgescript_vm_bind(vm, "Sound", "stop", fun_stop, 0);
    surgescript_vm_bind(vm, "Sound", "get_playing", fun_getplaying, 0);
}

/*
 * scripting_sound_ptr()
 * Returns the built-in sound_t* associated to the given SurgeScript Sound object
 * May be NULL
 */
sound_t* scripting_sound_ptr(const surgescript_object_t* object)
{
    return get_sound(object);
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_set_userdata(object, NULL);
    return NULL;
}

/* __init: pass the relative path to a sound file */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* path = surgescript_var_get_string(param[0], manager);
    sound_t* sound = sound_load(path);

    surgescript_object_set_userdata(object, sound);

    ssfree(path);
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    sound_t* sound = get_sound(object);

    if(sound != NULL) {
        if(sound_is_playing(sound))
            sound_stop(sound);
        sound_unref(sound);
    }

    return NULL;
}

/* plays the sound */
surgescript_var_t* fun_play(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    sound_t* sound = get_sound(object);

    if(sound != NULL)
        sound_play(sound);

    return NULL;
}

/* stops the sound */
surgescript_var_t* fun_stop(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    sound_t* sound = get_sound(object);

    if(sound != NULL)
        sound_stop(sound);
    
    return NULL;
}

/* is the sound playing? */
surgescript_var_t* fun_getplaying(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    sound_t* sound = get_sound(object);
    return surgescript_var_set_bool(surgescript_var_create(),
        (sound != NULL && sound_is_playing(sound))
    );
}



/* --- utilities --- */

/* gets the sound_t* pointer: may be NULL */
sound_t* get_sound(const surgescript_object_t* object)
{
    return (sound_t*)surgescript_object_userdata(object);
}