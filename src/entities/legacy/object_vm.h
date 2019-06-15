/*
 * Open Surge Engine
 * object_vm.h - virtual machine of the objects
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

#ifndef _OBJECT_VM_H
#define _OBJECT_VM_H

#include "object_machine.h"
#include "nanocalc/nanocalc.h"
#include "enemy.h"

/* an objectvm_t is a finite state machine.
   Every state has a name an can be decorated
   (in terms of the Decorator Design Pattern) */

typedef struct objectvm_t objectvm_t;

/* public methods */

objectvm_t* objectvm_create(enemy_t* owner); /* creates a new virtual machine */
objectvm_t* objectvm_destroy(objectvm_t* vm); /* destroys an existing VM */
objectmachine_t** objectvm_get_reference_to_current_state(objectvm_t* vm); /* returns a reference to the current state */
symboltable_t* objectvm_get_symbol_table(objectvm_t *vm); /* returns my symbol table (variables support; nanocalc stuff...) */
void objectvm_create_state(objectvm_t* vm, const char *name); /* you have to create a state before you can use it */
const char* objectvm_get_current_state(objectvm_t* vm); /* gets the current state */
void objectvm_set_current_state(objectvm_t* vm, const char *name); /* sets the current state */
void objectvm_return_to_previous_state(objectvm_t *vm); /* returns to the previous state */
void objectvm_reset_history(objectvm_t *vm); /* resets the history of states (can't return to previous state anymore) */
objectmachine_t* objectvm_get_state_by_name(objectvm_t* vm, const char *name); /* retrieves a specific state by name */

#endif
