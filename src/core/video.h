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

/* game mode (regular gameplay vs level editor) */
void video_set_game_mode(bool in_game_mode);
bool video_is_in_game_mode();

/* fullscreen mode */
void video_set_fullscreen(bool fullscreen);
bool video_is_fullscreen();

/* frames per second */
void video_set_fps_visible(bool visible);
bool video_is_fps_visible();
int video_fps(); /* the FPS rate */

/* built-in console */
void video_showmessage(const char *fmt, ...);

/* loading screen */
void video_display_loading_screen();

#endif
