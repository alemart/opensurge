/*
 * Open Surge Engine
 * image.h - image interface
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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

#include <stdbool.h>
#include <stdint.h>
#include "color.h"
#include "../util/v2d.h"

/* opaque image type */
typedef struct image_t image_t;

/* opaque texture handle */
typedef uint32_t texturehandle_t; /* GLuint */

/* opaque cache of vertices for low-level routines */
typedef struct vertexcache_t vertexcache_t;

/* image flip flags */
enum {
    IF_NONE         = 0,        /* no flip */
    IF_HFLIP        = 1 << 0,   /* flip horizontally */
    IF_VFLIP        = 1 << 1    /* flip vertically */
};

/* image creation flags */
enum {
    IC_DEFAULT      = 0,        /* video bitmap */
    IC_BACKBUFFER   = 1 << 0,   /* the image is going to be a drawing target (optimize) */
    IC_DEPTH        = 1 << 1,   /* require a depth buffer */
    IC_WRAP_MIRROR  = 1 << 2    /* mirror wrapping (shaders) */
};

/* retrieve Allegro bitmap */
#define IMAGE2BITMAP(img)           (*((ALLEGRO_BITMAP**)(img)))

/* create & destroy */
image_t* image_create(int width, int height); /* create an image */
image_t* image_create_ex(int width, int height, int flags); /* create an image with extra options */
void image_destroy(image_t* img); /* call this after image_create() */
image_t* image_clone(const image_t* src); /* clone an image */

/* load & save */
image_t* image_load(const char* path); /* will be unloaded automatically */
void image_save(const image_t* img, const char *path); /* save the image to a file */
int image_unload(const image_t* img); /* use if you want to save memory... */

/* sub-images */
image_t* image_create_shared(const image_t* parent, int x, int y, int width, int height); /* create a sub-image */
const image_t* image_parent(const image_t* img);
const image_t* image_parent_ex(const image_t* img, int* x, int* y, int* width, int* height);

/* utilities */
int image_width(const image_t* img); /* the width of the image */
int image_height(const image_t* img); /* the height of the image */
void image_enable_linear_filtering(image_t* img); /* enable linear filtering */
void image_disable_linear_filtering(image_t* img); /* disable linear filtering */
const char* image_filepath(const image_t* img); /* relative path of the originating file, if defined */
texturehandle_t image_texture(const image_t* img); /* get texture handle */

/* pixel manipulation */
void image_lock(image_t* img, const char* mode);
void image_unlock(image_t* img);
bool image_is_locked(const image_t* img);
color_t image_getpixel(const image_t* img, int x, int y);
void image_putpixel(int x, int y, color_t color);

/* drawing target */
void image_set_drawing_target(image_t* new_target);
image_t* image_drawing_target();
void image_hold_drawing(bool hold);

/* drawing primitives */
void image_clear(color_t color);
void image_line(int x1, int y1, int x2, int y2, color_t color);
void image_ellipse(int cx, int cy, int radius_x, int radius_y, color_t color);
void image_ellipsefill(int cx, int cy, int radius_x, int radius_y, color_t color);
void image_rect(int x1, int y1, int x2, int y2, color_t color);
void image_rectfill(int x1, int y1, int x2, int y2, color_t color);
void image_quick_triangles(const vertexcache_t* cache);
void image_quick_triangles_ex(const vertexcache_t* cache, const image_t* src);

/* rendering */
void image_blit(const image_t* src, int src_x, int src_y, int dest_x, int dest_y, int width, int height);
void image_draw(const image_t* src, int x, int y, int flags);
void image_draw_scaled(const image_t* src, int x, int y, v2d_t scale, int flags);
void image_draw_scaled_trans(const image_t* src, int x, int y, v2d_t scale, float alpha, int flags);
void image_draw_rotated(const image_t* src, int x, int y, int cx, int cy, float radians, int flags);
void image_draw_rotated_trans(const image_t* src, int x, int y, int cx, int cy, float radians, float alpha, int flags);
void image_draw_scaled_rotated(const image_t* src, int x, int y, int cx, int cy, v2d_t scale, float radians, int flags);
void image_draw_scaled_rotated_trans(const image_t* src, int x, int y, int cx, int cy, v2d_t scale, float radians, float alpha, int flags);
void image_draw_trans(const image_t* src, int x, int y, float alpha, int flags);
void image_draw_lit(const image_t* src, int x, int y, color_t color, int flags);
void image_draw_tinted(const image_t* src, int x, int y, color_t color, int flags);

/* cache for low-level drawing */
vertexcache_t* vertexcache_create();
void vertexcache_destroy(vertexcache_t* cache);
void vertexcache_clear(vertexcache_t* cache);
void vertexcache_push(vertexcache_t* cache, int x, int y, color_t color);
void vertexcache_push_ex(vertexcache_t* cache, int x, int y, int u, int v, color_t color);

#endif