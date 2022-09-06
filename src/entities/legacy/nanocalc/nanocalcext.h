/*
 * Open Surge Engine
 * nanocalcext.h - nanocalc extensions
 * Copyright (C) 2008-2022  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _NANOCALCEXT_H
#define _NANOCALCEXT_H

struct enemy_t;
struct enemy_list_t;
struct item_list_t;
struct brick_list_t;

void nanocalcext_register_bifs();
void nanocalcext_set_target_object(struct enemy_t *o, struct brick_list_t *bricks_nearby, struct item_list_t *items_nearby, struct enemy_list_t *objects_nearby);

#endif

