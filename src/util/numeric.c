/*
 * Open Surge Engine
 * numeric.c - numeric utilities
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

#include "numeric.h"
#include "v2d.h"



/*
 * lerp()
 * Linear interpolation from a to b
 * 0 <= t <= 1
 */
float lerp(float a, float b, float t)
{
    if(t < 0.0f)
        t = 0.0f;
    else if(t > 1.0f)
        t = 1.0f;

    return a * (1.0f - t) + b * t;
}

/*
 * lerp_angle()
 * Linear interpolation from alpha to beta, both given in radians
 * 0 <= t <= 1
 */
float lerp_angle(float alpha, float beta, float t)
{
    v2d_t a = v2d_new(cosf(alpha), sinf(alpha));
    v2d_t b = v2d_new(cosf(beta), sinf(beta));

    float dot = v2d_dot(a, b);
    if(nearly_equal(dot, -1.0f)) {
        /* alpha == beta + k * pi, k odd */
        return alpha + PI * t; /* why not alpha - PI * t? */
    }

    v2d_t c = v2d_lerp(a, b, t);
    return atan2f(c.y, c.x);
}