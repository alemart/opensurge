/*
 * Open Surge Engine
 * video.h - video manager
 * Copyright (C) 2008-2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "v2d.h"
#include "color.h"

struct image_t;

/* video modes */
#define VIDEORESOLUTION_1X        0 /* original size */
#define VIDEORESOLUTION_2X        1 /* double size */
#define VIDEORESOLUTION_3X        2 /* triple size */
#define VIDEORESOLUTION_4X        3 /* quadruple size */
#define VIDEORESOLUTION_EDT       4 /* level editor (the window size varies) */

/* video manager */
void video_init(const char *window_title, int resolution, int smooth, int fullscreen, int bpp);
void video_release();
void video_render();
int video_get_desktop_color_depth();
int video_get_color_depth();
int video_is_window_active();
void video_changemode(int resolution, int smooth, int fullscreen);
int video_get_resolution();
int video_is_smooth();
int video_is_fullscreen();
v2d_t video_get_screen_size(); /* usually, 426x240 */
v2d_t video_get_window_size(); /* the real size of the window, in pixels */
const struct image_t* video_get_window_surface();

/* backbuffer */
#define VIDEO_SCREEN_W            ((int)(video_get_screen_size().x))
#define VIDEO_SCREEN_H            ((int)(video_get_screen_size().y))
struct image_t *video_get_backbuffer();

/* FPS counter (frames per second) */
void video_show_fps(int show);
int video_is_fps_visible();
int video_fps(); /* get FPS rate */

/* video message */
void video_showmessage(const char *fmt, ...);

/* loading screen */
void video_display_loading_screen();

#endif
