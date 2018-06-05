/*
 * Open Surge Engine
 * objectmachine.h - Object Machine (handles the actions of this object)
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _OD_OBJECTMACHINE_H
#define _OD_OBJECTMACHINE_H

#include "../../../core/v2d.h"
#include "../../player.h"
#include "../../brick.h"
#include "../../enemy.h"
#include "../../item.h"
#include "../../actor.h"

/* <<interface>> object machine */
typedef struct objectmachine_t objectmachine_t;
struct objectmachine_t {
    void (*init)(objectmachine_t*); /* initializes the object */
    void (*release)(objectmachine_t*); /* releases the object */
    void (*update)(objectmachine_t*, player_t**, int, brick_list_t*, item_list_t*, object_list_t*); /* updates the object (runs every frame) */
    void (*render)(objectmachine_t*, v2d_t); /* renders the object */

    object_t* (*get_object_instance)(objectmachine_t*);
};

#endif

