/*
 * Open Surge Engine
 * v2d.c - 2D vectors
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
    return v2d_new(u.x + v.x , u.y + v.y);
}


/*
 * v2d_subtract()
 * Subtracts two vectors
 */
v2d_t v2d_subtract(v2d_t u, v2d_t v)
{
    return v2d_new(u.x - v.x , u.y - v.y);
}


/*
 * v2d_multiply()
 * Multiplies a vector by a scalar
 */
v2d_t v2d_multiply(v2d_t u, float h)
{
    return v2d_new(h * u.x , h * u.y);
}


/*
 * v2d_magnitude()
 * Returns the length of a vector
 */
float v2d_magnitude(v2d_t v)
{
    return sqrtf(v.x * v.x + v.y * v.y);
}


/*
 * v2d_dot()
 * Returns the dot product between u and v
 */
float v2d_dot(v2d_t u, v2d_t v)
{
    return (u.x * v.x + u.y * v.y);
}


/*
 * v2d_rotate()
 * Rotates a vector by an angle given in radians
 */
v2d_t v2d_rotate(v2d_t v, float radians)
{
    return v2d_new(
        v.x * cosf(radians) - v.y * sinf(radians),
        v.y * cosf(radians) + v.x * sinf(radians)
    );
}


/*
 * v2d_normalize()
 * Returns a normalized copy of the given vector
 */
v2d_t v2d_normalize(v2d_t v)
{
    float length = v2d_magnitude(v);

    if(!nearly_zero(length))
        return v2d_new(v.x / length, v.y / length);
    else
        return v2d_new(0.0f, 0.0f);
}


/*
 * v2d_lerp()
 * Linear interpolation between u and v
 * The same as: (1-t) * u + t * v, where 0 <= t <= 1
 */
v2d_t v2d_lerp(v2d_t u, v2d_t v, float t)
{
    t = clip01(t);
    return v2d_new(u.x + (v.x - u.x) * t, u.y + (v.y - u.y) * t);
}


/*
 * v2d_compmult()
 * Performs component-wise multiplication
 */
v2d_t v2d_compmult(v2d_t u, v2d_t v)
{
    return v2d_new(u.x * v.x, u.y * v.y);
}