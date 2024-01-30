/*
 * Open Surge Engine
 * waterfx.h - water special effect
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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

#ifndef _WATERFX_H
#define _WATERFX_H

#include "../core/color.h"
#include "../util/v2d.h"

void waterfx_init();
void waterfx_release();
void waterfx_update();
void waterfx_render_fg(v2d_t camera_position);
void waterfx_render_bg(v2d_t camera_position);

void waterfx_set_ypos(int ypos);
int waterfx_ypos();
int waterfx_default_ypos();

void waterfx_set_color(color_t color);
color_t waterfx_color();
color_t waterfx_default_color();

#endif