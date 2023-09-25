/*
 * Open Surge Engine
 * mobilegamepad.h - virtual gamepad for mobile devices
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

#ifndef _MOBILEGAMEPAD_H
#define _MOBILEGAMEPAD_H

#include <stdint.h>

/* directionals */
enum {
    MOBILEGAMEPAD_DPAD_CENTER       = 0,
    MOBILEGAMEPAD_DPAD_RIGHT        = 1,
    MOBILEGAMEPAD_DPAD_UP           = 1 << 1,
    MOBILEGAMEPAD_DPAD_LEFT         = 1 << 2,
    MOBILEGAMEPAD_DPAD_DOWN         = 1 << 3
};

/* buttons */
enum {
    MOBILEGAMEPAD_BUTTON_NONE       = 0,
    MOBILEGAMEPAD_BUTTON_ACTION     = 1,
    MOBILEGAMEPAD_BUTTON_BACK       = 1 << 1
};

/* initialization flags */
enum {
    MOBILEGAMEPAD_DISABLED          = 0,
    MOBILEGAMEPAD_WANT_TOUCH_INPUT  = 1,
    MOBILEGAMEPAD_WANT_MOUSE_INPUT  = 1 << 1
};

#if defined(__ANDROID__)
#define MOBILEGAMEPAD_DEFAULT_FLAGS (MOBILEGAMEPAD_WANT_TOUCH_INPUT)
#else
#define MOBILEGAMEPAD_DEFAULT_FLAGS (MOBILEGAMEPAD_WANT_TOUCH_INPUT | MOBILEGAMEPAD_WANT_MOUSE_INPUT)
#endif

/* state of the mobile gamepad */
typedef struct mobilegamepad_state_t mobilegamepad_state_t;
struct mobilegamepad_state_t {

    /* D-Pad flags */
    uint8_t dpad;

    /* button flags */
    uint8_t buttons;

};

/* public API */
void mobilegamepad_init(int flags);
void mobilegamepad_release();
void mobilegamepad_update();
void mobilegamepad_render();

bool mobilegamepad_is_available();
bool mobilegamepad_is_visible();
void mobilegamepad_get_state(mobilegamepad_state_t* state);

int mobilegamepad_opacity();
void mobilegamepad_set_opacity(int value);

void mobilegamepad_fadein();
void mobilegamepad_fadeout();

#endif