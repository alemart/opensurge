/*
 * Open Surge Engine
 * fadefx.h - fade effects
 * Copyright (C) 2013  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _FADEFX_H
#define _FADEFX_H

#include "global.h"

/* easy-to-use interface */
void fadefx_in(uint32 color, float seconds); /* fade in */
void fadefx_out(uint32 color, float seconds); /* fade out */
int fadefx_over(); /* end of fade effect? (only one action when this event loops) */
int fadefx_is_fading(); /* is the fade effect ocurring? */

/* engine routines */
void fadefx_init();
void fadefx_release();
void fadefx_update();

#endif
