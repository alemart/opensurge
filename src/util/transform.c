/*
 * Open Surge Engine
 * transform.c - transforms
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

#include <allegro5/allegro.h>
#include <math.h>
#include "transform.h"
#include "numeric.h"

/*
 * transform_identity()
 * Create an identity transform
 */
transform_t* transform_identity(transform_t* t)
{
    t->m[0] = 1.0f;
    t->m[1] = 0.0f;
    t->m[2] = 0.0f;
    t->m[3] = 0.0f;

    t->m[4] = 0.0f;
    t->m[5] = 1.0f;
    t->m[6] = 0.0f;
    t->m[7] = 0.0f;

    t->m[8] = 0.0f;
    t->m[9] = 0.0f;
    t->m[10] = 1.0f;
    t->m[11] = 0.0f;

    t->m[12] = 0.0f;
    t->m[13] = 0.0f;
    t->m[14] = 0.0f;
    t->m[15] = 1.0f;

    return t;
}

/*
 * transform_build()
 * Build a standard transform
 */
transform_t* transform_build(transform_t* t, v2d_t translation, float rotation, v2d_t scale, v2d_t anchor_point)
{
    /*

    Perform the following operations in order, starting from an identity matrix:

    1. Translate by (-anchor_point)
    2. Rotate
    3. Scale
    4. Translate

    */

    float c = cosf(rotation);
    float s = sinf(rotation);

    t->m[0] = scale.x * c;
    t->m[1] = scale.y * s;
    t->m[2] = 0.0f;
    t->m[3] = 0.0f;

    t->m[4] = scale.x * -s;
    t->m[5] = scale.y * c;
    t->m[6] = 0.0f;
    t->m[7] = 0.0f;

    t->m[8] = 0.0f;
    t->m[9] = 0.0f;
    t->m[10] = 1.0f;
    t->m[11] = 0.0f;

    t->m[12] = translation.x + scale.x * (c * -anchor_point.x - s * -anchor_point.y);
    t->m[13] = translation.y + scale.y * (s * -anchor_point.x + c * -anchor_point.y);
    t->m[14] = 0.0f;
    t->m[15] = 1.0f;

   return t;
}

/*
 * transform_copy()
 * Copy src to dest
 */
transform_t* transform_copy(transform_t* dest, const transform_t* src)
{
    *dest = *src;
    return dest;
}

/*
 * transform_translate()
 * Translation
 */
transform_t* transform_translate(transform_t* t, v2d_t offset)
{
    /*

    pre-multiply by

    [ 1  .  .  tx ]
    [ .  1  .  ty ]
    [ .  .  1  .  ]
    [ .  .  .  1  ]

    */

    /*float one = t->m[15];
    t->m[12] += offset.x * one;
    t->m[13] += offset.y * one;*/

    t->m[12] += offset.x;
    t->m[13] += offset.y;

    return t;
}

/*
 * transform_rotate()
 * Rotation
 */
transform_t* transform_rotate(transform_t* t, float radians)
{
    /*

    pre-multiply by

    [ cos x  -sin x   .   . ]
    [ sin x   cos x   .   . ]
    [   .       .     1   . ]
    [   .       .     .   1 ]

    */

    float c = cosf(radians);
    float s = sinf(radians);
    float p, q;

    #define UPDATE_COLUMN(col) do { \
        p = t->m[4*(col) + 0]; \
        q = t->m[4*(col) + 1]; \
        \
        t->m[4*(col) + 0] = c * p - s * q; \
        t->m[4*(col) + 1] = s * p + c * q; \
    } while(0)

    UPDATE_COLUMN(0);
    UPDATE_COLUMN(1);
    UPDATE_COLUMN(2);
    UPDATE_COLUMN(3);

    #undef UPDATE_COLUMN

    return t;
}

/*
 * transform_scale()
 * Scale
 */
transform_t* transform_scale(transform_t* t, v2d_t scale)
{
    /*

    pre-multiply by

    [ sx  .  .  . ]
    [ .  sy  .  . ]
    [ .   .  1  . ]
    [ .   .  .  1 ]

    */

    t->m[0] *= scale.x;
    t->m[1] *= scale.y;

    t->m[4] *= scale.x;
    t->m[5] *= scale.y;

    t->m[8] *= scale.x;
    t->m[9] *= scale.y;

    t->m[12] *= scale.x;
    t->m[13] *= scale.y;

    return t;
}

/*
 * transform_compose()
 * Pre-multiplies T by A, i.e., T := A * T
 */
transform_t* transform_compose(transform_t* t, const transform_t* a)
{
    #define DOT(row, col) ( \
        a->m[(row) + 0] * t->m[(col) * 4 + 0] + \
        a->m[(row) + 4] * t->m[(col) * 4 + 1] + \
        a->m[(row) + 8] * t->m[(col) * 4 + 2] + \
        a->m[(row) + 12] * t->m[(col) * 4 + 3] \
    )

    *t = (const transform_t){ .m = {
        DOT(0, 0),
        DOT(1, 0),
        DOT(2, 0),
        DOT(3, 0),
                    DOT(0, 1),
                    DOT(1, 1),
                    DOT(2, 1),
                    DOT(3, 1),
                                DOT(0, 2),
                                DOT(1, 2),
                                DOT(2, 2),
                                DOT(3, 2),
                                            DOT(0, 3),
                                            DOT(1, 3),
                                            DOT(2, 3),
                                            DOT(3, 3)
    }};

    return t;

    #undef DOT
}

/*
 * transform_decompose()
 * Decomposition of a 2D transform, given an anchor point
 * You may set the output parameters to NULL
 */
void transform_decompose(const transform_t* t, v2d_t* translation, float* rotation, v2d_t* scale, v2d_t anchor_point)
{
    /* extract the 2x2 rotation-scale block M */
    float a = t->m[0];
    float b = t->m[1];
    float c = t->m[4];
    float d = t->m[5];

    /* find auxiliary values related to the scale factor */
    float sx_squared = a*a + c*c;
    float sy_squared = b*b + d*d;
    float sx_times_sy = a*d - b*c;
    float s2 = sx_squared + sy_squared;

    /* find a rotation matrix Q assuming that the scale factor (sx,sy) is such that sy > 0 */
    float cos_squared = (a*a + d*d) / s2;
    float sin_squared = (b*b + c*c) / s2;
    float cos = sqrtf(cos_squared) * copysignf(1.0f, a * sx_times_sy); /* sy > 0 => sign(cos) == sign(a) * sign(sx*sy) == sign(a*sx) */
    float sin = sqrtf(sin_squared) * copysignf(1.0f, b); /* sy > 0 => sign(sin) == sign(b) */
    v2d_t q1 = v2d_new(cos, sin);
    v2d_t q2 = v2d_new(-sin, cos);

    /* find a 2x2 matrix L such that M = L * Q. L should be diagonal (i.e., l1 = l2 = 0), but may not be */
    float l0 = a * q1.x + c * q2.x;
    float l1 = b * q1.x + d * q2.x;
    float l2 = a * q1.y + c * q2.y;
    float l3 = b * q1.y + d * q2.y;
    /*printf("L=%f,%f,%f,%f\n",l0,l1,l2,l3);*/

    /* set the scale */
    if(scale != NULL) {
        scale->x = l0;
        scale->y = l3;
    }
    (void)l1;
    (void)l2;

    /* set the rotation */
    if(rotation != NULL) {
        float radians = acosf(cos) * copysignf(1.0f, sin);
        *rotation = radians;
    }

    /* set the translation */
    if(translation != NULL) {
        float tx = t->m[12];
        float ty = t->m[13];
        translation->x = tx - l0 * (cos * -anchor_point.x - sin * -anchor_point.y);
        translation->y = ty - l3 * (sin * -anchor_point.x + cos * -anchor_point.y);
    }
}

/*
 * transform_to_allegro()
 * Convert a transform_t to an ALLEGRO_TRANSFORM
 */
ALLEGRO_TRANSFORM* transform_to_allegro(ALLEGRO_TRANSFORM* al_transform, const transform_t* t)
{
    /* force a 2D transform */
    transform_t t2 = *t;
    t2.m[2] = t2.m[3] = t2.m[6] = t2.m[7] = t2.m[8] = t2.m[9] = t2.m[11] = t2.m[14] = 0.0;
    t2.m[10] = t2.m[15] = 1.0;

    /* convert */
    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 4; col++)
            al_transform->m[col][row] = t2.m[row + col * 4];
    }

    /* done */
    return al_transform;
}