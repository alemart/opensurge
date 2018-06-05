/*
 * Open Surge Engine
 * v2d.h - 2D vectors
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _V2D_H
#define _V2D_H

/* 2D vector structure */
typedef struct {
    float x, y;
} v2d_t;

/* interface */
v2d_t v2d_new(float x, float y); /* constructor */
v2d_t v2d_add(v2d_t u, v2d_t v); /* returns u+v */
v2d_t v2d_subtract(v2d_t u, v2d_t v); /* returns u-v */
v2d_t v2d_multiply(v2d_t u, float h); /* returns h*u */
v2d_t v2d_rotate(v2d_t v, float ang); /* returns v rotated by ang radians */
v2d_t v2d_normalize(v2d_t v); /* returns a normalized copy of v */
float v2d_magnitude(v2d_t v); /* returns the length of v */
float v2d_dotproduct(v2d_t u, v2d_t v); /* returns <u,v> */
v2d_t v2d_lerp(v2d_t u, v2d_t v, float weight); /* linear interpolation; 0.0 <= weight <= 1.0 */

#endif
