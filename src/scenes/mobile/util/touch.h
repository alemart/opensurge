/*
 * Open Surge Engine
 * touch.h - touch input utilities
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

#ifndef _MOBILEUTILTOUCH_H
#define _MOBILEUTILTOUCH_H

#include "../../../util/v2d.h"

struct input_t;

void handle_touch_input(struct input_t* mouse_input, void (*on_touch_start)(v2d_t), void (*on_touch_end)(v2d_t,v2d_t), void (*on_touch_move)(v2d_t,v2d_t));
void handle_touch_input_ex(struct input_t* mouse_input, void* data, void (*on_touch_start)(v2d_t,void*), void (*on_touch_end)(v2d_t,v2d_t,void*), void (*on_touch_move)(v2d_t,v2d_t,void*));

#endif