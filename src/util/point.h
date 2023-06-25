/*
 * Open Surge Engine
 * point.h - 2D points with integer coordinates
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

#ifndef _POINT_H
#define _POINT_H

/* Point struct */
typedef struct point_t {
    int x;
    int y;
} point_t;

/* Constructor */
#define point_new(x, y) ((point_t){ (x), (y) })

/* Equality test */
#define point_equals(p1, p2) ( \
    (p1).x == (p2).x && \
    (p1).y == (p2).y \
)

/* Addition p1 + p2 */
#define point_add(p1, p2) point_new( \
    (p1).x + (p2).x, \
    (p1).y + (p2).y \
)

/* Subtraction p1 - p2 */
#define point_subtract(p1, p2) point_new( \
    (p1).x - (p2).x, \
    (p1).y - (p2).y \
)

/* Dot product */
#define point_dot(p1, p2) ( \
    (p1).x * (p2).x + (p1).y * (p2).y \
)

#endif