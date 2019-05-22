/*
 * Open Surge Engine
 * timer.h - time handler
 * Copyright (C) 2010, 2012  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _TIMER_H
#define _TIMER_H

#include <stdint.h>

/* time manager */
void timer_init();
void timer_update();
void timer_release();

/* main utilities */
float timer_get_delta();
uint32_t timer_get_ticks();

#endif
