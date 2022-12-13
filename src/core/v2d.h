/*
 * Open Surge Engine
 * v2d.h - 2D vectors
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

#ifndef _V2D_H
#define _V2D_H

/* 2D vector structure */
typedef struct {
    float x, y;
} v2d_t;

/* constructor */
#define v2d_new(x, y)            (v2d_t){ (x), (y) }

/* interface */
v2d_t v2d_add(v2d_t u, v2d_t v); /* returns u+v */
v2d_t v2d_subtract(v2d_t u, v2d_t v); /* returns u-v */
v2d_t v2d_multiply(v2d_t u, float h); /* returns h*u */
v2d_t v2d_rotate(v2d_t v, float ang); /* returns v rotated by ang radians */
v2d_t v2d_normalize(v2d_t v); /* returns a normalized copy of v */
float v2d_magnitude(v2d_t v); /* returns the length of v */
float v2d_dotproduct(v2d_t u, v2d_t v); /* returns <u,v> */
v2d_t v2d_lerp(v2d_t u, v2d_t v, float t); /* linear interpolation; 0.0 <= t <= 1.0 */
v2d_t v2d_compmult(v2d_t u, v2d_t v); /* component-wise multiplication */

#endif
