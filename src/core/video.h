/*
 * Open Surge Engine
 * video.h - video manager
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

#ifndef _VIDEO_H
#define _VIDEO_H

#include <stdbool.h>
#include "../util/v2d.h"
#include "color.h"

struct image_t;

/* video manager */
void video_init();
void video_release();
void video_render(void (*render_overlay)());

/* backbuffer */
#define VIDEO_SCREEN_W ((int)(video_get_screen_size().x))
#define VIDEO_SCREEN_H ((int)(video_get_screen_size().y))
struct image_t *video_get_backbuffer();
v2d_t video_get_screen_size(); /* usually 426x240 pixels */

/* resolution: controls the size of the window */
typedef enum videoresolution_t {
    VIDEORESOLUTION_1X = 1, /* original size */
    VIDEORESOLUTION_2X,     /* double size */
    VIDEORESOLUTION_3X,     /* triple size */
    VIDEORESOLUTION_4X      /* quadruple size */
} videoresolution_t;

void video_set_resolution(videoresolution_t resolution);
videoresolution_t video_get_resolution();
videoresolution_t video_best_fit_resolution();
v2d_t video_get_window_size();

/* video mode: controls the size of the screen / backbuffer */
typedef enum videomode_t {
    VIDEOMODE_DEFAULT,      /* the screen size is set to its default (usually 426x240) */
    VIDEOMODE_FILL,         /* the size of the screen is set to the size of the window */
    VIDEOMODE_BESTFIT       /* similar to FILL, but maintains the aspect ratio of the default screen size */
} videomode_t;

void video_set_mode(videomode_t mode);
videomode_t video_get_mode();

/* video quality */
typedef enum videoquality_t {
    VIDEOQUALITY_LOW,
    VIDEOQUALITY_MEDIUM,
    VIDEOQUALITY_HIGH
} videoquality_t;

#define VIDEOQUALITY_DEFAULT VIDEOQUALITY_MEDIUM

void video_set_quality(videoquality_t quality);
videoquality_t video_get_quality();

/* fullscreen mode */
void video_set_fullscreen(bool fullscreen);
bool video_is_fullscreen();

/* immersive mode */
void video_set_immersive(bool immersive);
bool video_is_immersive();

/* frames per second */
void video_set_fps_visible(bool visible);
bool video_is_fps_visible();
int video_fps(); /* the FPS rate */

/* built-in console */
void video_showmessage(const char *fmt, ...);
void video_clearmessages();

/* misc */
void video_display_loading_screen();
void video_display_loading_screen_ex(double progress);
const char* video_get_window_title();
v2d_t video_convert_window_to_screen(v2d_t window_coordinates);
struct image_t* video_take_snapshot();
bool video_use_default_shader();
bool video_is_using_gles();
void video_flush();

#endif
