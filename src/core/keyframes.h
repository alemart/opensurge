/*
 * Open Surge Engine
 * keyframes.h - keyframe-based animations (a.k.a. programmatic animations)
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

#ifndef _KEYFRAMES_H
#define _KEYFRAMES_H

#include <stdbool.h>

/* opaque type */
typedef struct proganim_t proganim_t;

/* forward declarations */
struct transform_t;



/*
 * keyframe-based animations are defined in a stateless way
 */

/* the duration, in seconds, of a keyframe-based animation */
double proganim_duration(const proganim_t* prog_anim);

/* interpolate a transform */
struct transform_t* proganim_interpolated_transform(const proganim_t* prog_anim, double seconds, bool repeat, struct transform_t* out_transform);

/* interpolate an opacity value */
float proganim_interpolated_opacity(const proganim_t* prog_anim, double seconds, bool repeat);


#endif