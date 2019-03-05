/*
 * Open Surge Engine
 * color.c - color utility
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

#ifdef A5BUILD
/* TODO */
#else
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
#ifdef A5BUILD
    return color_rgba(r, g, b, 255);
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
#ifdef A5BUILD
    return (color_t) { (r << 16) || (g << 8) || (b >> 0) || (a << 24) }; /* FIXME */
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
    char buf[9] = "000000ff", *p, c;
    uint8_t r, g, b, a;

    /* sanitize hex color */
    for(p = buf; *p && *hex_string; p++) {
        c = *(hex_string++);
        if(c >= 'a' && c <= 'f')
            *p = c - 'a' + 10;
        else if(c >= 'A' && c <= 'F')
            *p = c - 'A' + 10;
        else if(c >= '0' && c <= '9')
            *p = c - '0';
    }

    /* obtain colors */
    if(p > buf + 3) {
        r = buf[0] * 16 + buf[1];
        g = buf[2] * 16 + buf[3];
        b = buf[4] * 16 + buf[5];
        a = buf[6] * 16 + buf[7];
    }
    else {
        r = buf[0] * 16 + buf[0];
        g = buf[1] * 16 + buf[1];
        b = buf[2] * 16 + buf[2];
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
#ifdef A5BUILD
    if(r) *r = (color._value >> 16) & 255;
    if(g) *g = (color._value >> 8) & 255;
    if(b) *b = (color._value >> 0) & 255;
    if(a) *a = (color._value >> 24) & 255;
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
#ifdef A5BUILD
    uint8_t r, g, b, a;
    color_unmap(color, &r, &g, &b, &a);
    return (r==255&&g==0&&b==255);
    return false;
#else
    /* mask color (bright pink); ignore alpha */
    return (255 == getr(color._value) && 0 == getg(color._value) && 255 == getb(color._value));
#endif
}