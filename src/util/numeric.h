/*
 * Open Surge Engine
 * numeric.h - numeric utilities
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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
int normalized_gaussian(float* g, float sigma, size_t n); /* normalized gaussian kernel */
double find_mean(const double* arr, int n); /* find the arithmetic mean of a dataset */
double find_mean_ex(const double* arr, int n, double* out_stddev, double* out_variance); /* find the arithmetic mean, the standard deviation and the variance of a dataset */
double find_median(const double* arr, int n); /* find the median of a dataset */
double find_median_ex(const double* arr, int n, double* out_min, double* out_max); /* find the median, the min and the max of a dataset */
int find_outliers(const double* arr, int n, bool* out_is_inlier, double* out_median, double* out_mad, double* out_min, double* out_max); /* find outliers in a normally distributed dataset */

#endif