/*
 * Open Surge Engine
 * audio.h - audio module
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

#ifndef _AUDIO_H
#define _AUDIO_H

#include <stdbool.h>
#include <stdint.h>

/* forward declarations */
typedef struct music_t music_t; /* music */
typedef struct sound_t sound_t; /* sound effect */
typedef uint64_t samplehandle_t; /* a handle to a sample that is played at some point in time - past or present */

/* audio manager */
void audio_init();
void audio_update();
void audio_release();
void audio_preload();

/* audio settings */
bool audio_is_muted();
void audio_set_muted(bool muted); /* global mute / unmute */
float audio_get_master_volume();
void audio_set_master_volume(float volume); /* 0.0 <= volume <= 1.0 (default) */
float audio_get_mixer_percentage();
void audio_set_mixer_percentage(float percentage); /* no music 0% ... 50% equal music & sfx ... 100% no sfx */

/* music API */
music_t *music_load(const char *path); /* will be unloaded automatically */
void music_destroy(music_t *music); /* you don't usually need to bother with this. */
int music_unref(music_t *music); /* returns the number of active references */
void music_play(music_t *music, bool loop); /* plays a music. Set loop to TRUE to make it loop continuously. */
void music_stop();
void music_pause();
void music_resume();
void music_set_volume(float volume); /* 0.0 <= volume <= 1.0 (default) */
float music_get_volume();
bool music_is_playing();
bool music_is_paused();
float music_duration(); /* duration in seconds */
music_t *music_current(); /* currently playing music. May be NULL */
const char *music_path(const music_t *music); /* the filepath of the specified music */

/* sound API */
sound_t* sound_load(const char* path); /* will be unloaded automatically */
void sound_destroy(sound_t* sound);
int sound_unref(sound_t* sound); /* returns the number of active references */
samplehandle_t sound_play(const sound_t* sound);
samplehandle_t sound_play_ex(const sound_t* sound, float volume, float pan, float speed); /* 0.0 <= volume; (left) -1.0 <= pan <= 1.0 (right); 1.0 = default speed */
void sound_stop(samplehandle_t handle);
bool sound_is_playing(samplehandle_t handle);
float sound_get_volume(samplehandle_t handle);
void sound_set_volume(samplehandle_t handle, float volume);
void sound_stop_all();
void sound_swap_mixers();

/* underwater muffler */
typedef enum mufflerprofile_t mufflerprofile_t;
enum mufflerprofile_t
{
    MUFFLER_OFF = 0,    /* disabled */
    MUFFLER_LOW,        /* light */
    MUFFLER_MEDIUM,     /* moderate */
    MUFFLER_HIGH        /* intense */
};

void audio_muffler_set_profile(mufflerprofile_t profile);
mufflerprofile_t audio_muffler_profile();
void audio_muffler_activate(int flags);
bool audio_muffler_is_activated();

enum /* muffler flags */
{
    MUFFLE_NOTHING = 0,
    MUFFLE_SOUNDS = 1,
    MUFFLE_MUSICS = 2,
    MUFFLE_EVERYTHING = MUFFLE_SOUNDS | MUFFLE_MUSICS
};

#endif
