/*
 * Open Surge Engine
 * fps.h - Framerate utility
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

#ifndef _FPS_H
#define _FPS_H

#define TARGET_FPS 60 /* target framerate */

double fps_current(); /* the current framerate */
double fps_stability(); /* a percentage of overall smoothness */
double fps_noise(); /* another measure of smoothness */

void fps_init();
void fps_release();
void fps_update(double elapsed_time, double delta_time);

#endif