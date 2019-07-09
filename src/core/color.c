/*
 * Open Surge Engine
 * color.c - color utility
 * Copyright (C) 2019  Alexandre Martins <alemartf@gmail.com>
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

#if !defined(A5BUILD)
#include <allegro.h>
#endif

#include <string.h>
#include "color.h"

/*
 * color_rgb()
 * Generates a color from its RGB components
 * 0 <= r, g, b <= 255
 */
color_t color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
#if defined(A5BUILD)
    return (color_t){ al_map_rgb(r, g, b) };
#else
    return (color_t){ makeacol(r, g, b, 255) };
#endif
}

/*
 * color_rgba()
 * Generates a color from its RGBA components
 * 0 <= r, g, b, a <= 255
 */
color_t color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
#if defined(A5BUILD)
    return (color_t){ al_map_rgba(r, g, b, a) };
#else
    return (color_t){ makeacol(r, g, b, a) };
#endif
}

/*
 * color_hex()
 * Converts a 3, 6 or 8-character RGB[A] hex string to a color
 * Example: "fff" becomes white; "ff8800" becomes orange
 */
color_t color_hex(const char* hex_string)
{
    static const uint8_t t[128] = { /* accepts any 0-127 entry */
        ['0'] = 0, ['1'] = 1, ['2'] = 2, ['3'] = 3, ['4'] = 4,
        ['5'] = 5, ['6'] = 6, ['7'] = 7, ['8'] = 8, ['9'] = 9,
        ['a'] = 10, ['b'] = 11, ['c'] = 12, ['d'] = 13, ['e'] = 14, ['f'] = 15,
        ['A'] = 10, ['B'] = 11, ['C'] = 12, ['D'] = 13, ['E'] = 14, ['F'] = 15
    };
    uint8_t buf[8] = { 0, 0, 0, 0, 0, 0, 15, 15 }, *p = buf;
    uint8_t r, g, b, a;

    /* process hex color */
    while(p < buf + sizeof(buf) && *hex_string)
        *p++ = t[*hex_string++ & 127];

    /* obtain colors */
    if(p > buf + 3) {
        r = (buf[0] << 4) + buf[1];
        g = (buf[2] << 4) + buf[3];
        b = (buf[4] << 4) + buf[5];
        a = (buf[6] << 4) + buf[7];
    }
    else {
        r = (buf[0] << 4) + buf[0];
        g = (buf[1] << 4) + buf[1];
        b = (buf[2] << 4) + buf[2];
        a = 255;
    }

    /* done! */
    return color_rgba(r, g, b, a);
}

/*
 * color_unmap()
 * Gets the RGBA components of a color
 * 0 <= r, g, b, a <= 255
 */
void color_unmap(color_t color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a)
{
#if defined(A5BUILD)
    unsigned char tmp = 0;
    al_unmap_rgba(color._color, r ? (unsigned char*)r : &tmp, g ? (unsigned char*)g : &tmp, b ? (unsigned char*)b : &tmp, a ? (unsigned char*)a : &tmp);
#else
    if(r) *r = getr(color._value);
    if(g) *g = getg(color._value);
    if(b) *b = getb(color._value);
    if(a) *a = geta(color._value);
#endif
}

/*
 * color_equals()
 * Compares two colors (equality)
 */
bool color_equals(color_t a, color_t b)
{
    return memcmp(&a, &b, sizeof(color_t)) == 0;
}

/*
 * color_is_transparent()
 * Is the given color transparent?
 */
bool color_is_transparent(color_t color)
{
#if defined(A5BUILD)
    unsigned char r, g, b, a;
    al_unmap_rgba(color._color, &r, &g, &b, &a);
    return (a == 0) || (r == 255 && g == 0 && b == 255); /* bright pink is the mask color */
#else
    /* mask color (bright pink); ignore alpha */
    return (255 == getr(color._value) && 0 == getg(color._value) && 255 == getb(color._value));
#endif
}