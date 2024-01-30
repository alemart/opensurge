/*
 * Open Surge Engine
 * fadefx.h - fade effects
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

#ifndef _FADEFX_H
#define _FADEFX_H

#include <stdbool.h>
#include "color.h"

/* easy-to-use interface */
void fadefx_in(color_t color, float seconds); /* fade in */
void fadefx_out(color_t color, float seconds); /* fade out */
bool fadefx_is_over(); /* end of fade effect? (only one action when this event loops) */
bool fadefx_is_fading(); /* is the fade effect ocurring? */

/* engine routines */
void fadefx_init();
void fadefx_release();
void fadefx_update();

#endif
