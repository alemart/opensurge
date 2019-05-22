/*
 * Open Surge Engine
 * camera.h - camera routines
 * Copyright (C) 2010  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _CAMERA_H
#define _CAMERA_H

#include "../core/v2d.h"

/* initializes the camera */
void camera_init();

/* updates the camera */
void camera_update();

/* releases the camera */
void camera_release();

/* moves the camera to a new position within a few seconds */
void camera_move_to(v2d_t position, float seconds);

/* locks the camera, so it will only move within the given rectangle (in pixels) */
void camera_lock(int x1, int y1, int x2, int y2);

/* unlocks the camera, so it will move freely in the level */
void camera_unlock();

/* is the camera locked? */
int camera_is_locked();

/* returns the position of the camera */
v2d_t camera_get_position();

/* sets a new position */
void camera_set_position(v2d_t position);

#endif
