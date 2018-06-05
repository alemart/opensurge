/*
 * Open Surge Engine
 * confirmbox.h - confirm box
 * Copyright (C) 2008-2009  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _CONFIRMBOX_H
#define _CONFIRMBOX_H

/* confirm box data structure */
typedef const char* confirmboxdata_t[3]; /* an array of 3 strings: text, option 1, option 2. ps: option 2 may be null. */

/* public functions */
void confirmbox_init(void *text_and_options); /* pass a pointer to a confirmboxdata_t */
void confirmbox_release();
void confirmbox_update();
void confirmbox_render();

/* interface */
int confirmbox_selected_option(); /* returns 1 or 2 (or 0 if nothing has been selected) */

#endif
