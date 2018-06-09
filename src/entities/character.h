/*
 * Open Surge Engine
 * character.h - Character system: meta data about a playable character
 * Copyright (C) 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _CHARACTER_H
#define _CHARACTER_H

struct sound_t;

typedef struct {

    char *name;
    char *companion_object_name;

    struct {
        float acc; /* acceleration */
        float dec; /* deceleration */
        float topspeed; /* top speed */
        float jmp; /* initial jump velocity */
        float jmprel; /* release jump velocity */
        float grv; /* gravity */
        float rollthreshold; /* roll threshold */
        float brakingthreshold; /* braking animation treshold */
        float slp; /* slope factor */
        float rolluphillslp; /* slope factor (rolling uphill) */
        float rolldownhillslp; /* slope factor (rolling downhill) */
    } multiplier;

    struct {
        char *sprite_name;
        int stopped;
        int walking;
        int running;
        int jumping;
        int springing;
        int rolling;
        int pushing;
        int gettinghit;
        int dead;
        int braking;
        int ledge;
        int drowned;
        int breathing;
        int waiting;
        int ducking;
        int lookingup;
        int winning;
        int ceiling;
    } animation;

    struct {
        struct sound_t *jump;
        struct sound_t *roll;
        struct sound_t *brake;
        struct sound_t *death;
    } sample;

} character_t;

/* initializes the character system */
void charactersystem_init();

/* releases the particle system */
void charactersystem_release();

/* gets the meta data of a character */
const character_t* charactersystem_get(const char* character_name);

#endif
