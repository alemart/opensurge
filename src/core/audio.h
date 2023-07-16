/*
 * Open Surge Engine
 * audio.h - audio module
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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

/* forward declarations */
typedef struct music_t music_t;
typedef struct sound_t sound_t;

/* audio manager */
void audio_init();
void audio_update();
void audio_release();
void audio_preload();

/* music management */
music_t *music_load(const char *path); /* will be unloaded automatically */
void music_destroy(music_t *music); /* you don't usually need to bother with this. */
void music_play(music_t *music, bool loop); /* plays a music. Set loop to TRUE to make it loop continuously. */
void music_stop();
void music_pause();
void music_resume();
void music_set_volume(float volume); /* 0.0 <= volume <= 1.0 (default) */
float music_get_volume();
bool music_is_playing();
bool music_is_paused();
int music_unref(music_t *music); /* returns the number of active references */
float music_duration(); /* duration in seconds */
music_t *music_current(); /* currently playing music. May be NULL */
const char *music_path(const music_t *music); /* the filepath of the specified music */

/* sample management */
sound_t *sound_load(const char *path); /* will be unloaded automatically */
void sound_destroy(sound_t *sample);
void sound_play(sound_t *sample);
void sound_play_ex(sound_t *sample, float vol, float pan, float freq); /* 0.0<=volume<=1.0; (left) -1.0<=pan<=1.0 (right); 1.0 = default frequency */
void sound_stop(sound_t *sample);
bool sound_is_playing(sound_t *sample);
int sound_unref(sound_t *sample); /* returns the number of active references */
float sound_get_volume(sound_t *sample);
void sound_set_volume(sound_t *sample, float volume); /* volume is in the [0,1] range */

#endif
