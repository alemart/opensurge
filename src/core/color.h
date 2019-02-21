/*
 * Open Surge Engine
 * color.h - color utility
 * Copyright (C) 2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _COLOR_H
#define _COLOR_H

#include <stdbool.h>
#include "global.h"

/* color type */
typedef struct color_t {
    int _value;
} color_t;

/* public API */
color_t color_rgb(uint8 r, uint8 g, uint8 b);
color_t color_rgba(uint8 r, uint8 g, uint8 b, uint8 a);
color_t color_hex(const char* hex_string);
void color_unmap(color_t color, uint8* r, uint8* g, uint8* b, uint8* a);
bool color_equals(color_t a, color_t b);
bool color_is_mask(color_t color);
color_t color_mask();

#endif