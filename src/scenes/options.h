/*
 * Open Surge Engine
 * options.h - options screen
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

#ifndef _OPTIONS_H
#define _OPTIONS_H

/* public data */
#define OPTIONS_MUSICFILE        "musics/options.ogg"

/* public functions */
void options_init(void*);
void options_release();
void options_update();
void options_render();

#endif
