/*
 * nanocalc addons
 * Mathematical built-in functions for nanocalc
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
 * http://opensurge2d.org
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this 
 * software and associated documentation files (the "Software"), to deal in the Software 
 * without restriction, including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons 
 * to whom the Software is furnished to do so, subject to the following conditions:
 *   
 * The above copyright notice and this permission notice shall be included in all copies or 
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "nanocalc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* constants */
static const float one = 1.0f;
#define EPS                 1e-5
#define INFI                (1.0f / (1.0f - one))
#define MAX_ARRAYS          2048
#define ARRAY_MAXLEN        1024
#define ARRAY_MAGIC         (0xDEAD + 0xBEEF)
#define ARRAY_PTR2HANDLE(x) (float)(ARRAY_MAGIC + (x))
#define ARRAY_HANDLE2PTR(x) ((int)(x) - ARRAY_MAGIC)



/* ============ available functions ============ */

/*
 * math
 */

/* if cond is true, returns t. Otherwise, returns f */
static float f_cond(float cond, float t, float f) { return fabs(cond)>EPS ? t : f; }

/* if val < lo, returns lo. if val > hi, returns hi. Otherwise, returns val */
static float f_clamp(float val, float lo, float hi) { return lo > hi ? f_clamp(val,hi,lo) : (val>lo ? (val<hi ? val : hi) : lo); }

/* linear interpolation */
static float f_lerp(float a, float b, float t) { return a + (b - a) * t; }

/* returns the maximum between a and b */
static float f_max(float a, float b) { return a>b?a:b; }

/* the minimum between a and b */
static float f_min(float a, float b) { return a<b?a:b; }

/* returns the arc tangent of y/x, in the interval [-pi/2,pi/2] radians */
static float f_atan2(float y, float x) { return (fabs(y)<EPS&&fabs(x)<EPS) ? 0.0f : atan2(y,x); }

/* returns 1 if x>=0 or -1 otherwise */
static float f_sign(float x) { return x >= 0.0f ? 1.0f : -1.0f; }

/* the absolute value of x */
static float f_abs(float x) { return fabs(x); }

/* given x integer, returns a random integer between 0 and x-1, inclusive */
static float f_random(float x) { return floor(((float)rand() / ((float)(RAND_MAX)+(float)(1)))*(floor(f_max(1,x)))); }

/* round down the value */
static float f_floor(float x) { return floor(x); }

/* round up the value */
static float f_ceil(float x) { return ceil(x); }

/* round to the closest integer */
static float f_round(float x) { return floor(x + 0.5f); }

/* compute the square root of x */
static float f_sqrt(float x) { return x >= 0.0f ? sqrt(x) : 0.0f; }

/* returns e raised to the power x */
static float f_exp(float x) { return exp(x); }

/* returns the natural logarithm of x */
static float f_log(float x) { return x > 0.0f ? log(x) : -INFI; }

/* returns the common (base-10) logarithm of x */
static float f_log10(float x) { return x > 0.0f ? log10(x) : -INFI; }

/* returns the cosine of x, x in radians */
static float f_cos(float x) { return cos(x); }

/* returns the sine of x, x in radians */
static float f_sin(float x) { return sin(x); }

/* returns the tangent of x, x in radians */
static float f_tan(float x) { return fabs(cos(x))>EPS ? tan(x) : f_sign(sin(x))*f_sign(cos(x))*INFI; }

/* returns the arc sine of x, in the interval [-pi/2,pi/2] radians */
static float f_asin(float x) { return asin(f_clamp(x,-1,1)); }

/* returns the arc cosine of x, in the interval [-pi/2,pi/2] radians */
static float f_acos(float x) { return acos(f_clamp(x,-1,1)); }

/* returns the arc tangent of x, in the interval [-pi/2,pi/2] radians */
/* Notice that because of the sign ambiguity, atan cannot determine with certainty in which quadrant the angle falls. Please use atan2 if you need to determine the quadrant. */
static float f_atan(float x) { return atan(x); }

/* hyperbolic sine of x */
static float f_sinh(float x) { return sinh(x); }

/* hyperbolic cosine of x */
static float f_cosh(float x) { return cosh(x); }

/* hyperbolic tangent of x */
static float f_tanh(float x) { return tanh(x); }

/* convert radians to degrees */
static float f_rad2deg(float x) { return x * 57.2957795131f; }

/* convert degrees to radians */
static float f_deg2rad(float x) { return x / 57.2957795131f; }

/* easter egg ;) */
static float f_leet() { return 1337; }

/* pi */
static float f_pi() { return 3.1415926535f; }

/* infinity */
static float f_infinity() { return INFI; }



/*
 * arrays
 */
struct ncarray_t {
    int length;
    float *value;
} ncarray[MAX_ARRAYS];

static void init_arrays()
{
    int i = MAX_ARRAYS;
    while(i--) {
        ncarray[i].length = 0;
        ncarray[i].value = NULL;
    }
}

static void release_arrays()
{
    int i = MAX_ARRAYS;
    while(i--) {
        ncarray[i].length = 0;
        if(ncarray[i].value) {
            free(ncarray[i].value);
            ncarray[i].value = NULL;
        }
    }
}




/* creates a new array */
static float f_new_array(float length)
{
    int i, j;

    /* must use a valid length */
    if(!((int)length > 0 && (int)length < ARRAY_MAXLEN))
        nanocalc_error("Can't create a new array with length %d. The length must be between 1 and %d, inclusive.", (int)length, ARRAY_MAXLEN-1);

    /* finds the first unused array */
    for(i=0; i<MAX_ARRAYS; i++) {
        if(ncarray[i].value == NULL) {
            ncarray[i].length = (int)length;
            if(!(ncarray[i].value = malloc((int)length * sizeof(*(ncarray[i].value)))))
                nanocalc_error("Can't create a new array with length %d: out of memory.", (int)length);
            for(j=0; j<(int)length; j++)
                ncarray[i].value[j] = 0.0f;
            return ARRAY_PTR2HANDLE(i);
        }
    }

    /* error! */
    nanocalc_error("Can't create more than %d arrays.", MAX_ARRAYS);
    return -1.0f;
}

/* destroys an existing array */
static float f_delete_array(float handle)
{
    int ptr = ARRAY_HANDLE2PTR(handle);

    if(ptr < 0 || ptr >= MAX_ARRAYS || ncarray[ptr].value == NULL)
        nanocalc_error("Invalid array handle: %f", handle);

    ncarray[ptr].length = 0;
    free(ncarray[ptr].value);
    ncarray[ptr].value = NULL;
    return -1.0f;
}

/* sets the elements of an array */
static float f_set_array_element(float handle, float index, float value)
{
    int ptr = ARRAY_HANDLE2PTR(handle);

    if(ptr < 0 || ptr >= MAX_ARRAYS || ncarray[ptr].value == NULL)
        nanocalc_error("Invalid array handle: %f", handle);

    if(index < 0 || index >= ncarray[ptr].length)
        nanocalc_error("Invalid array index: %d (handle %f). It should be a value between 0 and %d, inclusive.", (int)index, handle, -1+ncarray[ptr].length);

    return (ncarray[ptr].value[(int)index] = value);
}

/* gets the element of an array */
static float f_array_element(float handle, float index)
{
    int ptr = ARRAY_HANDLE2PTR(handle);

    if(ptr < 0 || ptr >= MAX_ARRAYS || ncarray[ptr].value == NULL)
        nanocalc_error("Invalid array handle: %f", handle);

    if(index < 0 || index >= ncarray[ptr].length)
        nanocalc_error("Invalid array index: %d (handle %f). It should be a value between 0 and %d, inclusive.", (int)index, handle, -1+ncarray[ptr].length);

    return ncarray[ptr].value[(int)index];
}

/* the length of an array */
static float f_array_length(float handle)
{
    int ptr = ARRAY_HANDLE2PTR(handle);

    if(ptr < 0 || ptr >= MAX_ARRAYS || ncarray[ptr].value == NULL)
        nanocalc_error("Invalid array handle: %f", handle);

    return ncarray[ptr].length;
}

/* is the given array valid? */
static float f_is_array(float handle)
{
    int ptr = ARRAY_HANDLE2PTR(handle);
    return (ptr < 0 || ptr >= MAX_ARRAYS || ncarray[ptr].value == NULL) ? 0.0f : 1.0f;
}

/* resizes an existing array */
static float f_resize_array(float handle, float new_length)
{
    int j, ptr = ARRAY_HANDLE2PTR(handle);

    if(ptr < 0 || ptr >= MAX_ARRAYS || ncarray[ptr].value == NULL)
        nanocalc_error("Invalid array handle: %f", handle);

    if(!((int)new_length > 0 && (int)new_length < ARRAY_MAXLEN))
        nanocalc_error("Can't create resize an array to have a length of %d. The length must be between 1 and %d, inclusive.", (int)new_length, ARRAY_MAXLEN-1);

    j = ncarray[ptr].length;
    ncarray[ptr].length = (int)new_length;
    ncarray[ptr].value = realloc(ncarray[ptr].value, (int)new_length * sizeof(*(ncarray[ptr].value)));
    for(; j<(int)new_length; j++)
        ncarray[ptr].value[j] = 0.0f;

    return handle;
}

/* clones an existing array */
static float f_clone_array(float handle)
{
    int len;
    float arr;

    arr = f_new_array(len = f_array_length(handle)); /* already checks if the handle is valid */
    while(len--)
        ncarray[ ARRAY_HANDLE2PTR(arr) ].value[len] = ncarray[ ARRAY_HANDLE2PTR(handle) ].value[len];

    return arr;
}


/* ============ nanocalc addons ================ */

/* binds the mathematical functions: call this AFTER nanocalc_init() */
void nanocalc_addons_init()
{
    /* array system */
    init_arrays();
    nanocalc_register_bif_arity3("set_array_element", f_set_array_element);
    nanocalc_register_bif_arity2("array_element", f_array_element);
    nanocalc_register_bif_arity2("resize_array", f_resize_array);
    nanocalc_register_bif_arity1("new_array", f_new_array);
    nanocalc_register_bif_arity1("delete_array", f_delete_array);
    nanocalc_register_bif_arity1("array_length", f_array_length);
    nanocalc_register_bif_arity1("clone_array", f_clone_array);
    nanocalc_register_bif_arity1("is_array", f_is_array);



    /* registering the math BIFs */
    nanocalc_register_bif_arity3("cond", f_cond);
    nanocalc_register_bif_arity3("clamp", f_clamp);
    nanocalc_register_bif_arity3("lerp", f_lerp);

    nanocalc_register_bif_arity2("max", f_max);
    nanocalc_register_bif_arity2("min", f_min);
    nanocalc_register_bif_arity2("atan2", f_atan2);

    nanocalc_register_bif_arity1("sign", f_sign);
    nanocalc_register_bif_arity1("abs", f_abs);
    nanocalc_register_bif_arity1("random", f_random);
    nanocalc_register_bif_arity1("floor", f_floor);
    nanocalc_register_bif_arity1("ceil", f_ceil);
    nanocalc_register_bif_arity1("round", f_round);
    nanocalc_register_bif_arity1("sqrt", f_sqrt);
    nanocalc_register_bif_arity1("exp", f_exp);
    nanocalc_register_bif_arity1("log", f_log);
    nanocalc_register_bif_arity1("log10", f_log10);
    nanocalc_register_bif_arity1("cos", f_cos);
    nanocalc_register_bif_arity1("sin", f_sin);
    nanocalc_register_bif_arity1("tan", f_tan);
    nanocalc_register_bif_arity1("asin", f_asin);
    nanocalc_register_bif_arity1("acos", f_acos);
    nanocalc_register_bif_arity1("atan", f_atan);
    nanocalc_register_bif_arity1("cosh", f_cosh);
    nanocalc_register_bif_arity1("sinh", f_sinh);
    nanocalc_register_bif_arity1("tanh", f_tanh);
    nanocalc_register_bif_arity1("deg2rad", f_deg2rad);
    nanocalc_register_bif_arity1("rad2deg", f_rad2deg);

    nanocalc_register_bif_arity0("leet", f_leet);
    nanocalc_register_bif_arity0("pi", f_pi);
    nanocalc_register_bif_arity0("infinity", f_infinity);
}

/* call this when you're done, but before nanocalc_release() */
void nanocalc_addons_release()
{
    /* clear any existing arrays */
    release_arrays();

    /* done! ;) */
}

/* reset all arrays */
void nanocalc_addons_resetarrays()
{
    release_arrays();
    init_arrays();
}

#ifdef __cplusplus
}
#endif
