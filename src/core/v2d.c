/*
 * Open Surge Engine
 * v2d.c - 2D vectors
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#include <math.h>
#include "v2d.h"
#include "util.h"
#include "global.h"

/*
 * v2d_add()
 * Adds two vectors
 */
v2d_t v2d_add(v2d_t u, v2d_t v)
{
    v2d_t w = { u.x + v.x , u.y + v.y };
    return w;
}


/*
 * v2d_subtract()
 * Subtracts two vectors
 */
v2d_t v2d_subtract(v2d_t u, v2d_t v)
{
    v2d_t w = { u.x - v.x , u.y - v.y };
    return w;
}


/*
 * v2d_multiply()
 * Multiplies a vector by a scalar
 */
v2d_t v2d_multiply(v2d_t u, float h)
{
    v2d_t v = { h * u.x , h * u.y };
    return v;
}


/*
 * v2d_magnitude()
 * Returns the magnitude of a given vector
 */
float v2d_magnitude(v2d_t v)
{
    return sqrt((v.x * v.x) + (v.y * v.y));
}


/*
 * v2d_dotproduct()
 * Dot product: u.v
 */
float v2d_dotproduct(v2d_t u, v2d_t v)
{
    return (u.x * v.x + u.y * v.y);
}


/*
 * v2d_rotate()
 * Rotates a vector. Angle in radians.
 */
v2d_t v2d_rotate(v2d_t v, float ang)
{
    v2d_t w = {
        v.x * cos(ang) - v.y * sin(ang),
        v.y * cos(ang) + v.x * sin(ang)
    };
    return w;
}


/*
 * v2d_normalize()
 * The same thing as v = v / |v|,
 * where |v| is the magnitude of v.
 */
v2d_t v2d_normalize(v2d_t v)
{
    float length = v2d_magnitude(v);

    if(!nearly_zero(length))
        return v2d_new(v.x / length, v.y / length);
    else
        return v2d_new(0, 0);
}



/*
 * v2d_lerp()
 * Performs a linear interpolation
 * between u and v (0.0 <= t <= 1.0)
 * The same as: (1-t) * u + t * v
 */
v2d_t v2d_lerp(v2d_t u, v2d_t v, float t)
{
    t = clip01(t);
    return v2d_new(u.x + (v.x - u.x) * t, u.y + (v.y - u.y) * t);
}

