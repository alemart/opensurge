/*
 * Open Surge Engine
 * v2d.c - 2D vectors
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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
 * v2d_new()
 * Creates a new 2D vector
 */
v2d_t v2d_new(float x, float y)
{
    v2d_t v = { x , y };
    return v;
}


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
    return sqrt( (v.x*v.x) + (v.y*v.y) );
}


/*
 * v2d_dotproduct()
 * Dot product: u.v
 */
float v2d_dotproduct(v2d_t u, v2d_t v)
{
    return (u.x*v.x + u.y*v.y);
}


/*
 * v2d_rotate()
 * Rotates a vector. Angle in radians.
 */
v2d_t v2d_rotate(v2d_t v, float ang)
{
    float x = v.x, y = v.y;
    v2d_t w;

    w.x = x*cos(ang) - y*sin(ang);
    w.y = y*cos(ang) + x*sin(ang);

    return w;
}


/*
 * v2d_normalize()
 * The same thing as v = v / |v|,
 * where |v| is the magnitude of v.
 */
v2d_t v2d_normalize(v2d_t v)
{
    float m = v2d_magnitude(v);
    v2d_t w = (m > EPSILON) ? v2d_new(v.x/m,v.y/m) : v2d_new(0,0);
    return w;
}



/*
 * v2d_lerp()
 * Performs a linear interpolation
 * between u and v.
 * 0.0 <= weight <= 1.0
 * The same as: (1-weight)*u + weight*v
 */
v2d_t v2d_lerp(v2d_t u, v2d_t v, float weight)
{
    float w = clip(weight, 0.0, 1.0);
    float c = 1.0 - w;
    return v2d_new(u.x*c+v.x*w, u.y*c+v.y*w);
}

