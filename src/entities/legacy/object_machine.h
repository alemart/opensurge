/*
 * Open Surge Engine
 * object_machine.h - Legacy API: Object Machine (handles the actions of this object)
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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

#ifndef _OBJECT_MACHINE_H
#define _OBJECT_MACHINE_H

#include "../../util/v2d.h"
#include "../player.h"
#include "../brick.h"
#include "../actor.h"
#include "item.h"
#include "enemy.h"

/* object machine: an object is a state machine; each state is a machine */
typedef struct objectmachine_t objectmachine_t;
struct objectmachine_t {
    void (*init)(objectmachine_t*); /* initializes the object */
    void (*release)(objectmachine_t*); /* releases the object */
    void (*update)(objectmachine_t*, player_t**, int, brick_list_t*, item_list_t*, object_list_t*); /* updates the object (runs every frame) */
    void (*render)(objectmachine_t*, v2d_t); /* renders the object */
    object_t* (*get_object_instance)(objectmachine_t*);
};

objectmachine_t* objectbasicmachine_new(object_t *object); /* constructs a basic, empty machine */

#endif

