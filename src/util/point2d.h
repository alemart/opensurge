/*
 * Open Surge Engine
 * point2d.h - 2D points with integer coordinates
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

#ifndef _POINT2D_H
#define _POINT2D_H

/* Point struct */
typedef struct point2d_t {
    int x;
    int y;
} point2d_t;

/* Constructor */
#define point2d_new(x, y) ((point2d_t){ (x), (y) })

/* Convert from v2d_t */
#define point2d_from_v2d(v) point2d_new((v).x, (v).y) /* truncate */

/* Equality test */
#define point2d_equals(p1, p2) ( \
    (p1).x == (p2).x && \
    (p1).y == (p2).y \
)

/* Addition p1 + p2 */
#define point2d_add(p1, p2) point2d_new( \
    (p1).x + (p2).x, \
    (p1).y + (p2).y \
)

/* Subtraction p1 - p2 */
#define point2d_subtract(p1, p2) point2d_new( \
    (p1).x - (p2).x, \
    (p1).y - (p2).y \
)

/* Dot product */
#define point2d_dot(p1, p2) ( \
    (p1).x * (p2).x + (p1).y * (p2).y \
)

#endif