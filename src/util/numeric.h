/*
 * Open Surge Engine
 * numeric.h - numeric utilities
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

#ifndef _NUMERIC_H
#define _NUMERIC_H

#include <math.h>
#include "util.h"

#ifdef PI
#undef PI
#endif

#define PI                      3.14159265358979323846
#define TWO_PI                  6.283185307179586
#define PI_OVER_TWO             1.5707963267948966
#define RAD2DEG                 57.29577951308232
#define DEG2RAD                 0.01745329251994329576

#define sign(x)                 copysign(1.0, (x)) /* -1.0 or 1.0 */
#define nearly_zero(x)          (fabs(x)<=0.00001)
#define nearly_equal(a,b)       (fabs((a)-(b))<=0.00001*max(fabs(a),fabs(b)))

float lerp(float a, float b, float t); /* linear interpolation */
float lerp_angle(float alpha, float beta, float t); /* alpha, beta in radians */

#endif