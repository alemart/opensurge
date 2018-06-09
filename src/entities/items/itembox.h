/*
 * Open Surge Engine
 * itembox.h - item boxes
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _ITEMBOX_H
#define _ITEMBOX_H

#include "../item.h"

/* public methods */
item_t* lifebox_create();
item_t* collectiblebox_create();
item_t* starbox_create();
item_t* speedbox_create();
item_t* glassesbox_create();
item_t* shieldbox_create();
item_t* fireshieldbox_create();
item_t* thundershieldbox_create();
item_t* watershieldbox_create();
item_t* acidshieldbox_create();
item_t* windshieldbox_create();
item_t* trapbox_create();
item_t* emptybox_create();

#endif
