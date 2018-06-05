/*
 * Open Surge Engine
 * inputmap.h - customized input mappings
 * Copyright (C) 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#ifndef _INPUTMAP_H
#define _INPUTMAP_H

#include "input.h"

/* public methods */
void inputmap_init();
void inputmap_release();

/* controllers: custom key mapping */
/* they're scripts located at the config/ folder */
typedef struct inputmap_t inputmap_t;
struct inputmap_t {
    char* name; /* controller name */

    struct inputmap_keyboard_t {
        int enabled;
        int scancode[IB_MAX]; /* scancode of button IB_* */
    } keyboard; /* keyboard mapping */

    struct inputmap_joystick_t {
        int enabled;
        int id;
        int button[IB_MAX];
    } joystick; /* joystick mapping */
};

/* gets an inputmap */
const inputmap_t* inputmap_get(const char* name);

#endif
