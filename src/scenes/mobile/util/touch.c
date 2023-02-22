/*
 * Open Surge Engine
 * touch.c - touch input utilities
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

#include "touch.h"
#include "../../../core/v2d.h"
#include "../../../core/video.h"
#include "../../../core/input.h"

static const inputbutton_t ACTION_BUTTON = IB_FIRE1;
static v2d_t read_mouse_position(input_t* mouse_input);



/*
 * handle_touch_input()
 * Typically handled in a update loop
 * The function pointers may be NULL
 */
void handle_touch_input(input_t* mouse_input, void (*on_touch_start)(v2d_t), void (*on_touch_end)(v2d_t,v2d_t), void (*on_touch_move)(v2d_t,v2d_t))
{
    /* only a single touch point is supported at the moment */
    static v2d_t touch_start, touch_end;

    if(input_button_released(mouse_input, ACTION_BUTTON)) {
        touch_end = read_mouse_position(mouse_input);
        if(on_touch_end != NULL)
            on_touch_end(touch_start, touch_end);
    }
    else if(input_button_pressed(mouse_input, ACTION_BUTTON)) {
        touch_start = read_mouse_position(mouse_input);
        if(on_touch_start != NULL)
            on_touch_start(touch_start);
    }
    else if(input_button_down(mouse_input, ACTION_BUTTON)) {
        v2d_t touch_current = read_mouse_position(mouse_input);
        if(on_touch_move != NULL)
            on_touch_move(touch_start, touch_current);
    }
}




/* private */

/* read the position of the cursor of the mouse in screen space */
v2d_t read_mouse_position(input_t* mouse_input)
{
    v2d_t window_size = video_get_window_size();
    v2d_t screen_size = video_get_screen_size();
    v2d_t window_mouse = input_get_xy((inputmouse_t*)mouse_input);
    v2d_t normalized_mouse = v2d_new(window_mouse.x / window_size.x, window_mouse.y / window_size.y);
    v2d_t mouse = v2d_compmult(normalized_mouse, screen_size);

    return mouse;
}