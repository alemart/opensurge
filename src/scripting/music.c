/*
 * Open Surge Engine
 * music.c - scripting system: music
 * Copyright (C) 2018  Alexandre Martins <alemartf@gmail.com>
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
#include "../core/util.h"
#include "../core/audio.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_play(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_stop(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_pause(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvolume(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvolume(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getplaying(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t VOLUME_ADDR = 0;
static const double DEFAULT_VOLUME = 1.0;
static inline music_t* get_music(const surgescript_object_t* object);
static inline double get_volume(const surgescript_object_t* object);

/*
 * scripting_register_music()
 * Register the Music object
 */
void scripting_register_music(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Music", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Music", "__init", fun_init, 1);
    surgescript_vm_bind(vm, "Music", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Music", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Music", "play", fun_play, 0);
    surgescript_vm_bind(vm, "Music", "stop", fun_stop, 0);
    surgescript_vm_bind(vm, "Music", "pause", fun_pause, 0);
    surgescript_vm_bind(vm, "Music", "set_volume", fun_setvolume, 1);
    surgescript_vm_bind(vm, "Music", "get_volume", fun_getvolume, 0);
    surgescript_vm_bind(vm, "Music", "get_playing", fun_getplaying, 0);
}

/*
 * scripting_music_ptr()
 * Returns the built-in music_t* associated to the given SurgeScript Music object
 * May be NULL
 */
music_t* scripting_music_ptr(const surgescript_object_t* object)
{
    return get_music(object);
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    music_t* music = get_music(object);

    /* synchronize volume property with the built-in volume of the engine */
    if(music != NULL && music_current() == music) {
        surgescript_heap_t* heap = surgescript_object_heap(object);

        /* NOTE: unsure about the performance of music_get_volume(),
                 as it is implementation-defined */
        surgescript_var_set_number(surgescript_heap_at(heap, VOLUME_ADDR), music_get_volume());
    }

    /* done */
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    ssassert(VOLUME_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_number(surgescript_heap_at(heap, VOLUME_ADDR), DEFAULT_VOLUME);
    surgescript_object_set_userdata(object, NULL);
    return NULL;
}

/* __init: pass the relative path to a music file */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    char* path = surgescript_var_get_string(param[0], manager);
    music_t* music = music_load(path);

    surgescript_object_set_userdata(object, music);
    if(music != NULL && music_current() == music)
        surgescript_var_set_number(surgescript_heap_at(heap, VOLUME_ADDR), music_get_volume());

    ssfree(path);
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    music_t* music = get_music(object);

    if(music != NULL) {
        /*
        // is this desirable? e.g., when you
        // delete the parent object just after
        // playing this sound
        if(music_current() == music && music_is_playing())
            music_stop();
        music_unref(music);
        */
        surgescript_object_set_userdata(object, NULL);
    }

    return NULL;
}

/* plays the music (once) */
surgescript_var_t* fun_play(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    music_t* music = get_music(object);
    double volume = get_volume(object);

    if(music != NULL) {
        if(music_current() == music && music_is_paused())
            music_resume(music);
        else
            music_play(music, false);
        music_set_volume(volume);
    }

    return NULL;
}

/* stops the music */
surgescript_var_t* fun_stop(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    music_t* music = get_music(object);

    if(music != NULL && music_current() == music)
        music_stop();
    
    return NULL;
}

/* pauses the music */
surgescript_var_t* fun_pause(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    music_t* music = get_music(object);

    if(music != NULL && music_current() == music)
        music_pause();

    return NULL;
}

/* is this music playing? */
surgescript_var_t* fun_getplaying(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    music_t* music = get_music(object);
    return surgescript_var_set_bool(surgescript_var_create(),
        (music != NULL && music_current() == music && music_is_playing())
    );
}

/* get volume, a value in the [0, 1] range */
surgescript_var_t* fun_getvolume(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    double volume = get_volume(object);
    return surgescript_var_set_number(surgescript_var_create(), volume);
}

/* set volume, a value in the [0, 1] range */
surgescript_var_t* fun_setvolume(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    double volume = surgescript_var_get_number(param[0]);
    music_t* music = get_music(object);

    volume = clip(volume, 0.0, 1.0);
    surgescript_var_set_number(surgescript_heap_at(heap, VOLUME_ADDR), volume);
    if(music != NULL && music_current() == music)
        music_set_volume(volume);

    return NULL;
}


/* --- utilities --- */

/* gets the music_t* pointer: may be NULL */
music_t* get_music(const surgescript_object_t* object)
{
    return (music_t*)surgescript_object_userdata(object);
}

/* the volume of the music, a value in [0, 1] */
double get_volume(const surgescript_object_t* object)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_get_number(surgescript_heap_at(heap, VOLUME_ADDR));
}