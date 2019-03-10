/*
 * Open Surge Engine
 * image.h - image interface
 * Copyright (C) 2008-2010, 2012, 2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _IMAGE_H
#define _IMAGE_H

#include "color.h"
#include "v2d.h"

/* opaque image type */
typedef struct image_t image_t;

/* image flags (allows bitwise OR) */
typedef enum imageflags_t {
    IF_NONE  = 0,
    IF_HFLIP = 1, /* powers of 2 */
    IF_VFLIP = 2
} imageflags_t;

/* image management */
image_t* image_load(const char* path); /* will be unloaded automatically */
image_t* image_create(int width, int height); /* create a memory surface */
image_t* image_create_shared(const image_t* parent, int x, int y, int width, int height); /* creates a shared sub-image */
void image_destroy(image_t* img); /* call this after image_create() */
int image_unload(image_t* img); /* use if you want to save memory... */

/* utilities */
int image_width(const image_t* img); /* the width of the image */
int image_height(const image_t* img); /* the height of the image */
void image_save(const image_t* img, const char *path); /* saves the image to a file */
image_t* image_clone(const image_t* src); /* clones an image */
image_t* image_clone_region(const image_t* src, int x, int y, int width, int height); /* clones a region */
image_t* image_snapshot(); /* take a snapshot */

/* pixel manipulation */
void image_lock(image_t* img);
void image_unlock(image_t* img);
color_t image_getpixel(const image_t* img, int x, int y);

/* drawing target */
void image_set_drawing_target(image_t* new_target);
image_t* image_drawing_target();

/* drawing primitives */
void image_clear(color_t color);
void image_line(int x1, int y1, int x2, int y2, color_t color);
void image_ellipse(int cx, int cy, int radius_x, int radius_y, color_t color);
void image_rectfill(int x1, int y1, int x2, int y2, color_t color);
void image_rect(int x1, int y1, int x2, int y2, color_t color);
void image_waterfx(int y, color_t color);

/* rendering */
void image_blit(const image_t* src, int src_x, int src_y, int dest_x, int dest_y, int width, int height);
void image_draw(const image_t* src, int x, int y, imageflags_t flags);
void image_draw_scaled(const image_t* src, int x, int y, v2d_t scale, imageflags_t flags);
void image_draw_rotated(const image_t* src, int x, int y, int cx, int cy, float radians, imageflags_t flags);
void image_draw_scaled_rotated(const image_t* src, int x, int y, int cx, int cy, v2d_t scale, float radians, imageflags_t flags);
void image_draw_trans(const image_t* src, int x, int y, float alpha, imageflags_t flags);
void image_draw_tinted(const image_t* src, int x, int y, color_t color, imageflags_t flags);
void image_draw_multiply(const image_t* src, int x, int y, color_t color, imageflags_t flags);

#endif
