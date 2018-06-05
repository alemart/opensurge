/*
 * Open Surge Engine
 * objectdecorator.c - The Decorator Design Pattern is applied to the objects
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#include "objectdecorator.h"

object_t* objectdecorator_get_object_instance(objectmachine_t *obj)
{
    objectdecorator_t *me = (objectdecorator_t*)obj;
    objectmachine_t *decorated_machine = me->decorated_machine;

    return decorated_machine->get_object_instance(decorated_machine);
}

