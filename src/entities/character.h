/*
 * Open Surge Engine
 * character.h - Character system: meta data about a playable character
 * Copyright (C) 2008-2023 Alexandre Martins <alemartf@gmail.com>
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

#include <stdbool.h>
#include "../util/darray.h"

struct sound_t;

typedef struct character_t character_t;
struct character_t {
    char *name;

    struct {
        float acc; /* acceleration */
        float dec; /* deceleration */
        float topspeed; /* top speed */
        float jmp; /* jump velocity */
        float grv; /* gravity */
        float slp; /* slope factor */
        float frc; /* friction */
        float chrg; /* charge speed */
        float airacc; /* air acceleration */
        float airdrag; /* air drag (friction) */
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
        int charging;
    } animation;

    struct {
        struct sound_t *jump;
        struct sound_t *roll;
        struct sound_t *brake;
        struct sound_t *death;
        struct sound_t* charge;
        struct sound_t* release;
        float charge_pitch;
    } sample;

    struct {
        bool roll;
        bool charge;
        bool brake;
    } ability;

    DARRAY(char*, companion_name);
};

/* initializes the character system */
void charactersystem_init();

/* releases the character system */
void charactersystem_release();

/* gets the meta data of a character */
const character_t* charactersystem_get(const char* character_name);

#endif
