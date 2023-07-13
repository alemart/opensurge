/*
 * Open Surge Engine
 * rect.h - rectangles with integer coordinates
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

#ifndef _RECT_H
#define _RECT_H

/* Rectangle struct */
typedef struct rect_t {
    int x;
    int y;
    int width;
    int height;
} rect_t;

/* Constructor */
#define rect_new(x, y, width, height) ((rect_t){ (x), (y), (width), (height) })

/* Area of the rectangle */
#define rect_area(r) ( \
    (r).width * (r).height \
)

/* Equality test */
#define rect_equals(r1, r2) ( \
    (r1).x == (r2).x && \
    (r1).y == (r2).y && \
    (r1).width == (r2).width && \
    (r1).height == (r2).height \
)

/* Does rectangle r contains point p?
   p is a point2d_t or a v2d_t */
#define rect_contains(r, p) ( \
    (p).x >= (r).x && (p).x < (r).x + (r).width && \
    (p).y >= (r).y && (p).y < (r).y + (r).height \
)

/* Do rectangles r1 and r2 overlap? */
#define rect_overlaps(r1, r2) (!( \
    (r1).x >= (r2).x + (r2).width || \
    (r2).x >= (r1).x + (r1).width || \
    (r1).y >= (r2).y + (r2).height || \
    (r2).y >= (r1).y + (r1).height \
))

/* Rectangle intersection */
#define rect_intersect(r1, r2) ( \
    !rect_overlaps((r1), (r2)) ? rect_new(0, 0, 0, 0) : \
    rect_new( \
        (r1).x > (r2).x ? (r1).x : (r2).x, \
        (r1).y > (r2).y ? (r1).y : (r2).y, \
        ((r1).x + (r1).width < (r2).x + (r2).width ? (r1).x + (r1).width : (r2).x + (r2).width) - ((r1).x > (r2).x ? (r1).x : (r2).x), \
        ((r1).y + (r1).height < (r2).y + (r2).height ? (r1).y + (r1).height : (r2).y + (r2).height) - ((r1).y > (r2).y ? (r1).y : (r2).y) \
    ) \
)

#endif