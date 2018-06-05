/*
 * Open Surge Engine
 * bouncingcollectible.h - bouncing collectible
 * Copyright (C) 2011, 2014  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _BOUNCINGCOLLECTIBLE_H
#define _BOUNCINGCOLLECTIBLE_H

#include "../item.h"

/* public methods */
item_t* bouncingcollectible_create();
void bouncingcollectible_set_speed(item_t *item, v2d_t speed);

#endif
