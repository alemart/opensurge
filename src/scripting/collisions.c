/*
 * Open Surge Engine
 * collisions.c - scripting system: collision system
 * Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include "scripting.h"
#include "../core/image.h"
#include "../core/video.h"

/* private */
static surgescript_var_t* fun_collisionbox_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_collisions()
 * Register built-in functions for the collision system
 */
void scripting_register_collisions(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "CollisionBox", "render", fun_collisionbox_render, 0);
    surgescript_vm_bind(vm, "CollisionBox", "zindex", fun_collisionbox_zindex, 0);
}

/* CollisionBox routines */

/* render */
surgescript_var_t* fun_collisionbox_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_var_t* debug = surgescript_var_create();
    surgescript_object_call_function(object, "get_visible", NULL, 0, debug);

    if(surgescript_var_get_bool(debug) && scripting_util_is_object_inside_screen(object)) {
        surgescript_var_t* left = surgescript_var_create();
        surgescript_var_t* right = surgescript_var_create();
        surgescript_var_t* top = surgescript_var_create();
        surgescript_var_t* bottom = surgescript_var_create();
        uint32_t color = image_rgb(255, 255, 0);
        v2d_t camera = scripting_util_object_camera(object);
        int l, r, t, b;

        surgescript_object_call_function(object, "get_left", NULL, 0, left);
        surgescript_object_call_function(object, "get_right", NULL, 0, right);
        surgescript_object_call_function(object, "get_top", NULL, 0, top);
        surgescript_object_call_function(object, "get_bottom", NULL, 0, bottom);

        l = surgescript_var_get_number(left) - (camera.x - VIDEO_SCREEN_W/2);
        r = surgescript_var_get_number(right) - 1 - (camera.x - VIDEO_SCREEN_W/2);
        t = surgescript_var_get_number(top) - (camera.y - VIDEO_SCREEN_H/2);
        b = surgescript_var_get_number(bottom) - 1 - (camera.y - VIDEO_SCREEN_H/2);

        image_line(video_get_backbuffer(), l, t, r, t, color);
        image_line(video_get_backbuffer(), r, t, r, b, color);
        image_line(video_get_backbuffer(), r, b, l, b, color);
        image_line(video_get_backbuffer(), l, b, l, t, color);

        surgescript_var_destroy(bottom);
        surgescript_var_destroy(top);
        surgescript_var_destroy(right);
        surgescript_var_destroy(left);
    }

    surgescript_var_destroy(debug);
    return NULL;
}

/* zindex */
surgescript_var_t* fun_collisionbox_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), 1.0f);
}