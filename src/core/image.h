/*
 * Open Surge Engine
 * image.h - image interface
 * Copyright (C) 2008-2010, 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#include "global.h"
#include "v2d.h"

/* opaque image type */
typedef struct image_t image_t;

/* image flags (bitwise OR) */
#define IF_NONE                 0
#define IF_HFLIP                1
#define IF_VFLIP                2

/* image management */
image_t *image_load(const char *path); /* will be unloaded automatically */
int image_unref(const char *path); /* use if you want to save memory... */
image_t *image_create(int width, int height); /* create a memory surface */
void image_destroy(image_t *img); /* call this after image_create() */
void image_save(const image_t *img, const char *path); /* saves the image to a file */
image_t *image_create_shared(const image_t *parent, int x, int y, int width, int height); /* creates a sub-image */

/* properties */
int image_width(const image_t *img);
int image_height(const image_t *img);
uint32 image_rgb(uint8 r, uint8 g, uint8 b);
void image_color2rgb(uint32 color, uint8 *r, uint8 *g, uint8 *b);
int image_pixelperfect_collision(const image_t *img1, const image_t *img2, int x1, int y1, int x2, int y2);
uint32 image_getpixel(const image_t *img, int x, int y);

/* drawing primitives */
void image_clear(image_t *img, uint32 color);
void image_putpixel(image_t *img, int x, int y, uint32 color);
void image_line(image_t *img, int x1, int y1, int x2, int y2, uint32 color);
void image_ellipse(image_t *img, int cx, int cy, int radius_x, int radius_y, uint32 color);
void image_rectfill(image_t *img, int x1, int y1, int x2, int y2, uint32 color);

/* rendering */
void image_blit(const image_t *src, image_t *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
void image_draw(const image_t *src, image_t *dest, int x, int y, uint32 flags);
void image_draw_scaled(const image_t *src, image_t *dest, int x, int y, v2d_t scale, uint32 flags);
void image_draw_rotated(const image_t *src, image_t *dest, int x, int y, int cx, int cy, float ang, uint32 flags);
void image_draw_trans(const image_t *src, image_t *dest, int x, int y, float alpha, uint32 flags);
void image_draw_lit(const image_t *src, image_t *dest, int x, int y, uint32 color, float alpha, uint32 flags);
void image_draw_waterfx(image_t *img, int y, uint32 color); /* pixels below y will have a water effect */

#endif
