/*
 * Open Surge Engine
 * color.c - color utility
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

#include <string.h>
#include "color.h"
#include "../util/stringutil.h"

/*
 * color_rgb()
 * Generates a color from its RGB components
 * 0 <= r, g, b <= 255
 */
color_t color_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return (color_t){ al_map_rgb(r, g, b) };
}

/*
 * color_rgba()
 * Generates a color from its RGBA components
 * 0 <= r, g, b, a <= 255
 * Note: color_premul_rgba() may be preferable over this
 */
color_t color_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return (color_t){ al_map_rgba(r, g, b, a) };
}

/*
 * color_premul_rgba()
 * Generates a color from its RGBA components
 * The RGB components will be premultiplied by the alpha value
 * 0 <= r, g, b, a <= 255
 */
color_t color_premul_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    float rf = (float)r / 255.0f;
    float gf = (float)g / 255.0f;
    float bf = (float)b / 255.0f;
    float af = (float)a / 255.0f;

    return (color_t){ al_map_rgba_f(rf * af, gf * af, bf * af, af) };
}

/*
 * color_hex()
 * Converts a 3, 6 or 8-character RGB[A] hex string to a color
 * Example: "fff" becomes white; "ff8800" becomes orange
 * Note: this will return a color with premultiplied alpha
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
        r = (buf[0] << 4) | buf[1];
        g = (buf[2] << 4) | buf[3];
        b = (buf[4] << 4) | buf[5];
        a = (buf[6] << 4) | buf[7];
    }
    else {
        r = (buf[0] << 4) | buf[0];
        g = (buf[1] << 4) | buf[1];
        b = (buf[2] << 4) | buf[2];
        a = 255;
    }

    /* done! */
    return color_premul_rgba(r, g, b, a);
}

/*
 * color_to_hex()
 * Converts a color to an equivalent hex string, e.g.,
 * color_rgba(255, 255, 0, 128) becomes "ffff0080"
 * color_rgb(255, 255, 255) becomes "ffffff"
 * If dest is set to NULL, a static buffer is returned.
 * Otherwise, dest is returned with the hex string.
 * Note: dest_size should be >= 9 (or 0 if dest is NULL)
 */
const char* color_to_hex(color_t color, char* dest, size_t dest_size)
{
    static const char table[] = "0123456789abcdef";
    static char buf[16];
    uint8_t r, g, b, a;
    char *p = buf;

    /* get the RGBA components of the input color */
    color_unmap(color, &r, &g, &b, &a);

    /* write the hex string to the internal buffer */
    *(p++) = table[(r >> 4) & 15];
    *(p++) = table[r & 15];
    *(p++) = table[(g >> 4) & 15];
    *(p++) = table[g & 15];
    *(p++) = table[(b >> 4) & 15];
    *(p++) = table[b & 15];
    if(a < 255) {
        *(p++) = table[(a >> 4) & 15];
        *(p++) = table[a & 15];
    }
    *p = '\0';

    /* return the hex string */
    if(dest != NULL)
        return str_cpy(dest, buf, dest_size);
    else
        return buf;
}

/*
 * color_unmap()
 * Gets the RGBA components of a color
 * 0 <= r, g, b, a <= 255
 */
void color_unmap(color_t color, uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* a)
{
    unsigned char tmp = 0;
    al_unmap_rgba(color._color, r ? (unsigned char*)r : &tmp, g ? (unsigned char*)g : &tmp, b ? (unsigned char*)b : &tmp, a ? (unsigned char*)a : &tmp);
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
    unsigned char r, g, b, a;
    al_unmap_rgba(color._color, &r, &g, &b, &a);
    return (a == 0) || (r == 255 && g == 0 && b == 255); /* bright pink is the mask color */
}