/*
 * Open Surge Engine
 * transform.h - transforms
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

#ifndef _TRANSFORM_H
#define _TRANSFORM_H

#include "v2d.h"

/* transformation struct */
typedef struct transform_t transform_t;
struct transform_t {
    float m[16]; /* 4x4 matrix in column-major format */
};

/* forward declarations */
struct ALLEGRO_TRANSFORM;

/* basic API */
transform_t* transform_identity(transform_t* t); /* create an identity transform */
transform_t* transform_build(transform_t* t, v2d_t translation, float rotation, v2d_t scale, v2d_t anchor_point); /* build a standard transform */
transform_t* transform_copy(transform_t* dest, const transform_t* src); /* copy src to dest */
transform_t* transform_translate(transform_t* t, v2d_t offset); /* translation */
transform_t* transform_rotate(transform_t* t, float radians); /* rotation */
transform_t* transform_scale(transform_t* t, v2d_t scale); /* scale */

/* composition */
transform_t* transform_compose(transform_t* t, const transform_t* a); /* T := A * T */

/* decomposition */
void transform_decompose(const transform_t* t, v2d_t* translation, float* rotation, v2d_t* scale, v2d_t anchor_point); /* give an anchor point as input */

/* misc */
struct ALLEGRO_TRANSFORM* transform_to_allegro(struct ALLEGRO_TRANSFORM* al_transform, const transform_t* t);

#endif