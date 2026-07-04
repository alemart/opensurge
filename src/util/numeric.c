/*
 * Open Surge Engine
 * numeric.c - numeric utilities
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

#include <string.h>
#include "numeric.h"
#include "v2d.h"

static int compare_doubles(const void *a, const void *b);


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

/*
 * find_mean()
 * Find the arithmetic mean of a dataset arr of size n
 */
double find_mean(const double* arr, int n)
{
    return find_mean_ex(arr, n, NULL, NULL);
}

/*
 * find_mean_ex()
 * Find the arithmetic mean, the standard deviation and the variance of a dataset arr of size n
 * All output parameters are optional and may be set to NULL
 */
double find_mean_ex(const double* arr, int n, double* out_stddev, double* out_variance)
{
    if(n <= 0) {
        if(out_stddev) *out_stddev = 0.0;
        if(out_variance) *out_variance = 0.0;
        return 0.0;
    }

    double mean = 0.0;
    for(int i = 0; i < n; i++)
        mean += arr[i];
    mean /= (double)n;

    if(!out_variance && !out_stddev)
        return mean;

    double variance = 0.0;
    for(int i = 0; i < n; i++) {
        double deviation = arr[i] - mean;
        variance += deviation * deviation;
    }
    variance /= (double)n;

    if(out_variance)
        *out_variance = variance;

    if(out_stddev)
        *out_stddev = sqrt(variance);

    return mean;
}

/**
 * find_median()
 * Find the median of a dataset arr of size n
 */
double find_median(const double* arr, int n)
{
    return find_median_ex(arr, n, NULL, NULL);
}

/**
 * find_median_ex()
 * Find the median, the minimum, and the maximum value of a dataset arr of size n
 * All output parameters are optional and may be set to NULL
 */
double find_median_ex(const double* arr, int n, double* out_min, double* out_max)
{
    double median, *sorted_arr;

    if(n <= 0) {
        if(out_min) *out_min = 0.0;
        if(out_max) *out_max = 0.0;
        return 0.0;
    }

    sorted_arr = mallocx(n * sizeof(double));
    memcpy(sorted_arr, arr, n * sizeof(double));
    qsort(sorted_arr, n, sizeof(double), compare_doubles);

    if(n % 2 == 1)
        median = sorted_arr[n / 2];
    else
        median = (sorted_arr[(n / 2) - 1] + sorted_arr[n / 2]) * 0.5;

    if(out_min)
        *out_min = sorted_arr[0];

    if(out_max)
        *out_max = sorted_arr[n-1];

    free(sorted_arr);
    return median;
}

/**
 * find_outliers()
 * Find outliers in a normally distributed dataset arr of size n
 * All output parameters are optional and may be set to NULL. out_is_inlier is an output array of size n
 * Returns the number of outliers
 */
int find_outliers(const double* arr, int n, bool* out_is_inlier, double* out_median, double* out_mad, double* out_min, double* out_max)
{
    /* this implementation is based on the method described in
       "How to detect and handle outliers" by Iglewicz and Hoaglin (1993) */
    const double SENSITIVITY_THRESHOLD = 3.5; /* mimics a Z-score threshold for outlier detection */
    const double MAD_SCALE_FACTOR = 0.6745; /* suitable for normally distributed data */
    const double MAGIC_SCALE_FACTOR = SENSITIVITY_THRESHOLD / MAD_SCALE_FACTOR;

    /* validity check */
    if(n <= 0) {
        if(out_median) *out_median = 0.0;
        if(out_mad) *out_mad = 0.0;
        if(out_min) *out_min = 0.0;
        if(out_max) *out_max = 0.0;
        return false;
    }

    /* initialize the mask */
    if(out_is_inlier)
        memset(out_is_inlier, 1, n * sizeof(bool));

    /* find the median of the dataset */
    double median = find_median_ex(arr, n, out_min, out_max);
    if(out_median)
        *out_median = median;

    /* calculate absolute deviations */
    double* abs_deviations = mallocx(n * sizeof(double));
    for(int i = 0; i < n; i++)
        abs_deviations[i] = fabs(arr[i] - median);

    /* find the Median Absolute Deviation (MAD) */
    double mad = find_median_ex(abs_deviations, n, NULL, NULL); /* possibly zero */
    if(out_mad)
        *out_mad = mad;

    /* find outliers according to the Modified Z-Score method */
    int outlier_count = 0;
    double magic_threshold = mad * MAGIC_SCALE_FACTOR;
    for(int i = 0; i < n; i++) {
        if(abs_deviations[i] > magic_threshold) {
            ++outlier_count;
            if(out_is_inlier)
                out_is_inlier[i] = false;
        }
    }

    /* cleanup */
    free(abs_deviations);
    return outlier_count;
}





/* private stuff */


/**
 * compare_doubles()
 * Comparison function for qsort
 * Sort by increasing values
 */
int compare_doubles(const void* a, const void* b)
{
    double da = *(const double*)a;
    double db = *(const double*)b;
    return (da > db) - (da < db);
}
