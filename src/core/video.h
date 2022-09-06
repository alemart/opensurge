/*
 * Open Surge Engine
 * video.h - video manager
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _VIDEO_H
#define _VIDEO_H

#include <stdbool.h>
#include "v2d.h"
#include "color.h"

struct image_t;

/* video modes */
typedef enum videoresolution_t {
    VIDEORESOLUTION_EDT, /* level editor */
    VIDEORESOLUTION_1X,  /* original size */
    VIDEORESOLUTION_2X,  /* double size */
    VIDEORESOLUTION_3X,  /* triple size */
    VIDEORESOLUTION_4X,  /* quadruple size */
} videoresolution_t;

/* video manager */
void video_init(videoresolution_t resolution, bool smooth, bool fullscreen, int bpp);
void video_release();
void video_render();
int video_get_preferred_color_depth();
int video_get_color_depth();
void video_changemode(videoresolution_t resolution, bool smooth, bool fullscreen);
videoresolution_t video_get_resolution();
videoresolution_t video_initial_resolution();
bool video_is_smooth();
bool video_is_fullscreen();
v2d_t video_get_screen_size(); /* usually, 426x240 */
v2d_t video_get_window_size(); /* the real size of the window, in pixels */

/* backbuffer */
#define VIDEO_SCREEN_W            ((int)(video_get_screen_size().x))
#define VIDEO_SCREEN_H            ((int)(video_get_screen_size().y))
struct image_t *video_get_backbuffer();

/* FPS counter (frames per second) */
void video_show_fps(bool show);
bool video_is_fps_visible();
int video_fps(); /* get FPS rate */

/* video message */
void video_showmessage(const char *fmt, ...);

/* loading screen */
void video_display_loading_screen();

#endif
