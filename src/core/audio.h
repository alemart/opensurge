/*
 * Open Surge Engine
 * audio.h - audio module
 * Copyright (C) 2008-2009  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _AUDIO_H
#define _AUDIO_H

/* forward declarations */
typedef struct music_t music_t;
typedef struct sound_t sound_t;


/* audio manager */
void audio_init();
void audio_update();
void audio_release();


/* music management */
music_t *music_load(const char *path); /* will be unloaded automatically */
void music_destroy(music_t *music); /* you don't usually need to bother with this. */
void music_play(music_t *music, int loop); /* plays and loops [loop] times the given music. Set loop to INFINITY to make it loop forever. */
void music_stop();
void music_pause();
void music_resume();
void music_set_volume(float volume); /* 0.0 <= volume <= 1.0 (default) */
float music_get_volume();
int music_is_playing();
int music_unref(const char *path); /* returns the number of active references */
float music_duration(); /* in seconds */


/* sample management */
sound_t *sound_load(const char *path); /* will be unloaded automatically */
void sound_destroy(sound_t *sample);
void sound_play(sound_t *sample);
void sound_play_ex(sound_t *sample, float vol, float pan, float freq, int loop); /* 0.0<=volume<=1.0; (left) -1.0<=pan<=1.0 (right); 1.0 = default frequency; 0 = no loops */
void sound_stop(sound_t *sample);
int sound_is_playing(sound_t *sample);
int sound_unref(const char *path); /* returns the number of active references */

#endif
