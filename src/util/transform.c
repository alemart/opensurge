/*
 * Open Surge Engine
 * transform.c - transforms
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

#include <allegro5/allegro.h>
#include <math.h>
#include "transform.h"

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
 * transform_copy()
 * Copy src to t
 */
transform_t* transform_copy(transform_t* t, const transform_t* src)
{
    *t = *src;

    return t;
}

/*
 * transform_translate()
 * Translation
 */
transform_t* transform_translate(transform_t* t, v2d_t offset)
{
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
    float c = cosf(radians);
    float s = sinf(radians);

    for(int i = 0; i < 4; i++) {
        float p = t->m[4*i + 0];
        float q = t->m[4*i + 1];

        t->m[4*i + 0] = c * p - s * q;
        t->m[4*i + 1] = s * p + c * q;
    }

    return t;
}

/*
 * transform_scale()
 * Scale
 */
transform_t* transform_scale(transform_t* t, v2d_t scale)
{
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
 * Compose transforms A and B such that the resulting transform
 * T is set to A * B
 */
transform_t* transform_compose(transform_t* t, const transform_t* a, const transform_t* b)
{
    #define DOT(row, col) ( \
        a->m[(row) + 0] * b->m[(col) * 4 + 0] + \
        a->m[(row) + 4] * b->m[(col) * 4 + 1] + \
        a->m[(row) + 8] * b->m[(col) * 4 + 2] + \
        a->m[(row) + 12] * b->m[(col) * 4 + 3] \
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