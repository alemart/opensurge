/*
 * Open Surge Engine
 * color.h - color utility
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

#ifndef _COLOR_H
#define _COLOR_H

#include <stdbool.h>
#include <stdint.h>

/* color type */
#include <allegro5/allegro.h>

typedef struct color_t {
    ALLEGRO_COLOR _color;
} color_t;

/* public API */
color_t color_rgb(uint8_t r, uint8_t g, uint8_t b);
color_t color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
color_t color_premul_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
color_t color_hex(const char* hex_string);
const char* color_to_hex(color_t color, char* dest, size_t dest_size);
void color_unmap(color_t color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a);
bool color_equals(color_t a, color_t b);
bool color_is_transparent(color_t color);

#endif