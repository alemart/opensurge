/*
 * Open Surge Engine
 * sound.c - scripting system: sound
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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
#include "../util/util.h"

/* sound structure */
typedef struct surgescript_sound_t surgescript_sound_t;
struct surgescript_sound_t {
    const sound_t* sound;
    samplehandle_t handle; /* the handle of the last call to .play(); multiple SurgeScript objects may be created for fine-grained control of multiple instances of the same sound effect */
    float volume;
    float pan;
    float speed;
};

/* private */
#define NULL_HANDLE ((samplehandle_t)0) /* sound_play(NULL) */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_play(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_stop(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getplaying(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvolume(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvolume(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static inline surgescript_sound_t* get_sound_data(const surgescript_object_t* object);
static const surgescript_sound_t NO_SOUND = {
    .sound = NULL,
    .handle = NULL_HANDLE,
    .volume = 1.0f,
    .pan = 0.0f,
    .speed = 1.0f
};

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
    surgescript_vm_bind(vm, "Sound", "set_volume", fun_setvolume, 1);
    surgescript_vm_bind(vm, "Sound", "get_volume", fun_getvolume, 0);
    surgescript_vm_bind(vm, "Sound", "get_playing", fun_getplaying, 0);
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_sound_t* sound_data = ssmalloc(sizeof *sound_data);
    *sound_data = NO_SOUND;
    surgescript_object_set_userdata(object, sound_data);
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_sound_t* sound_data = get_sound_data(object);

#if 0
    if(sound_data->sound != NULL) {
        /* is this desirable? e.g., when you
           delete the parent object just after
           playing this sound */
        sound_stop(sound_data->handle); /* undesirable */
        sound_unref((sound_t*)sound_data->sound); /* this may be a bug */
    }
#endif

    ssfree(sound_data);
    return NULL;
}

/* __init: pass the relative path to a sound file */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_sound_t* sound_data = get_sound_data(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* path = surgescript_var_get_string(param[0], manager);

    *sound_data = NO_SOUND;
    sound_data->sound = sound_load(path);

    ssfree(path);
    return NULL;
}

/* plays the sound */
surgescript_var_t* fun_play(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_sound_t* sound_data = get_sound_data(object);

    if(sound_data->sound != NULL)
        sound_data->handle = sound_play_ex(sound_data->sound, sound_data->volume, sound_data->pan, sound_data->speed);

    return NULL;
}

/* stops the sound */
surgescript_var_t* fun_stop(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_sound_t* sound_data = get_sound_data(object);
    sound_stop(sound_data->handle);
    return NULL;
}

/* is the sound playing? */
surgescript_var_t* fun_getplaying(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_sound_t* sound_data = get_sound_data(object);
    return surgescript_var_set_bool(surgescript_var_create(), sound_is_playing(sound_data->handle));
}

/* get volume, a value in the [0, 1] range */
surgescript_var_t* fun_getvolume(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_sound_t* sound_data = get_sound_data(object);
    return surgescript_var_set_number(surgescript_var_create(), sound_data->volume);
}

/* set volume, a value in the [0, 1] range */
surgescript_var_t* fun_setvolume(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_sound_t* sound_data = get_sound_data(object);
    double volume = surgescript_var_get_number(param[0]);

    sound_data->volume = clip01(volume);
    sound_set_volume(sound_data->handle, sound_data->volume);

    return NULL;
}



/* --- utilities --- */

/* gets the sound_t* pointer: may be NULL */
surgescript_sound_t* get_sound_data(const surgescript_object_t* object)
{
    return (surgescript_sound_t*)surgescript_object_userdata(object);
}