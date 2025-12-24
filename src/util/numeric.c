/*
 * Open Surge Engine
 * numeric.c - numeric utilities
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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
        return alpha + PI * clip01(t); /* why not alpha - PI * t? */
    }

    v2d_t c = v2d_lerp(a, b, t);
    return atan2f(c.y, c.x);
}

/*
 * normalized_gaussian()
 * Generate a Gaussian g[0..n-1] with standard deviation sigma around (n-1)/2
 * with the sum of all g[i] = 1. n should be at least 1 + 2 * ceil(sigma * 3).
 * Returns half the window size on success or a negative value on error
 */
int normalized_gaussian(float* g, float sigma, size_t n)
{
    double int_g = 0.0;

    if(n == 0 || sigma <= 0.0f)
        return -1; /* invalid input */

    int c = (n-1) / 2; /* 0 <= 2*c < n doesn't go out of bounds; also, unsigned n > 0 */
    int window_size = 1 + 2 * (int)ceilf(sigma * 3); /* [-3 sigma, +3 sigma] */
    int w = min(window_size / 2, c); /* 0 <= 2*w + 1 <= 2*c + 1 = n */

    memset(g, 0, sizeof(*g) * n);
    g += c;

    /*

    note: (w >= 0)

    i)  w <= c => 0 <= c-w
    ii) w <= c and 2*c < n => c+w <= c+c = 2*c < n

    therefore, 0 <= c+x < n for all x in [-w,+w]

    */

    for(int x = -w; x <= w; x++) {
        g[x] = expf(-0.5f * (x / sigma) * (x / sigma));
        int_g += g[x];
    }

    for(int x = -w; x <= w; x++)
        g[x] /= int_g; /* normalize */

    return w;
}