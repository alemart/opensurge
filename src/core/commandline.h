/*
 * Open Surge Engine
 * commandline.h - command line parser
 * Copyright (C) 2010-2013  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _COMMANDLINE_H
#define _COMMANDLINE_H

/* command line structure */
typedef struct commandline_t {
    /* video stuff */
    int video_resolution;
    int smooth_graphics;
    int fullscreen; /* fullscreen mode? */
    int color_depth; /* bits per pixel */
    int show_fps;

    /* run custom level */
    int custom_level; /* user needs to run a custom level? */
    char custom_level_path[1024]; /* filepath */

    /* run custom quest */
    int custom_quest; /* user needs to run a custom quest? */
    char custom_quest_path[1024]; /* filepath */

    /* other */
    char language_filepath[1024];
    char basedir[1024];
    int use_gamepad;
    int optimize_cpu_usage;
    int allow_font_smoothing;
} commandline_t;

/* command line interface */
commandline_t commandline_parse(int argc, char **argv);

#endif
