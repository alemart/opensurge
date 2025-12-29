/*
 * Open Surge Engine
 * input.h - input management
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

#ifndef _INPUT_H
#define _INPUT_H

#include <stdbool.h>
#include "../util/v2d.h"

/* forward declarations */
typedef struct input_t input_t; /* input_t is the base class */
typedef struct inputmouse_t inputmouse_t;
typedef struct inputcomputer_t inputcomputer_t;
typedef struct inputuserdefined_t inputuserdefined_t;

/* constants */
#define MAX_JOYSTICK_BUTTONS 32 /* bit vector uint32_t */

/* buttons */
typedef enum inputbutton_t {

    /* directionals */
    IB_UP,      /* up */
    IB_RIGHT,   /* right */
    IB_DOWN,    /* down */
    IB_LEFT,    /* left */

    /* basic controller */
    IB_FIRE1,   /* primary action button */
    IB_FIRE2,   /* secondary action button */
    IB_FIRE3,   /* start button */
    IB_FIRE4,   /* back / select button */

    /* extended controller */
    IB_FIRE5,
    IB_FIRE6,
    IB_FIRE7,
    IB_FIRE8,

    /* number of buttons */
    IB_MAX

} inputbutton_t;

/* public methods */
void input_init();
void input_update();
void input_release();

bool input_is_joystick_available(); /* a joystick is available, but not necessarily enabled */
bool input_is_joystick_enabled(); /* a joystick is available and enabled (i.e., not ignored) */
bool input_is_joystick_ignored();
void input_ignore_joystick(bool ignore); /* ignores the input received from joysticks (if they're available) */
int input_number_of_joysticks();
void input_reconfigure_joysticks();
void input_print_joysticks();

input_t *input_create_user(const char* inputmap_name); /* user's custom input device (set inputmap_name to NULL to use a default mapping) */
input_t *input_create_computer(); /* computer-controlled "input": will return an inputcomputer_t*, which is also an input_t* */
input_t *input_create_mouse(); /* mouse */
void input_destroy(input_t *in);

bool input_button_down(const input_t *in, inputbutton_t button);
bool input_button_pressed(const input_t *in, inputbutton_t button);
bool input_button_released(const input_t *in, inputbutton_t button);

void input_simulate_button_down(input_t *in, inputbutton_t button);
void input_simulate_button_up(input_t *in, inputbutton_t button);
void input_simulate_button_press(input_t *in, inputbutton_t button);

void input_reset(input_t *in);
void input_copy(input_t *dest, const input_t *src);

bool input_is_enabled(const input_t *in);
void input_enable(input_t *in);
void input_disable(input_t *in);

bool input_is_blocked(const input_t *in);
void input_block(input_t *in);
void input_unblock(input_t *in);

v2d_t input_get_xy(const inputmouse_t *in);

void input_change_mapping(inputuserdefined_t *in, const char* inputmap_name); /* set inputmap_name to NULL to use a default mapping */
const char* input_get_mapping_name(const inputuserdefined_t *in);

#endif