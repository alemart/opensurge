/*
 * Open Surge Engine
 * stageselect.h - stage selection screen
 * Copyright (C) 2010, 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _STAGESELECT_H
#define _STAGESELECT_H

/* public functions */
void stageselect_init(void *should_enable_debug); /* pass TRUE or FALSE. debug mode = display all levels, including hidden ones. */
void stageselect_release();
void stageselect_update();
void stageselect_render();

#endif
