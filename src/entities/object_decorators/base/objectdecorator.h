/*
 * Open Surge Engine
 * objectdecorator.h - Abstract decorator class: the Decorator Design pattern is applied to the objects
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

#ifndef _OD_OBJECTDECORATOR_H
#define _OD_OBJECTDECORATOR_H

#include "objectmachine.h"

/* <<abstract>> object decorator class */
typedef struct objectdecorator_t objectdecorator_t;
struct objectdecorator_t {
    objectmachine_t base; /* objectdecorator_t implements the objectmachine_t interface */
    objectmachine_t *decorated_machine; /* what are we decorating? */
};

/* not all methods are abstract, though */
object_t* objectdecorator_get_object_instance(objectmachine_t *obj);

#endif

