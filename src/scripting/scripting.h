/*
 * Open Surge Engine
 * surgescript.h - scripting system
 * Copyright (C) 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#ifndef _SCRIPTING_H
#define _SCRIPTING_H

#include <surgescript.h>
#include "../core/util.h"

/* scripting API */
void scripting_init(int argc, const char** argv);
void scripting_release();
bool scripting_testmode();
surgescript_vm_t* surgescript_vm(); /* SurgeScript VM instance */

/* scripting utilities */
surgescript_objecthandle_t scripting_util_require_component(const surgescript_object_t* object, const char* component_name);
v2d_t scripting_util_world_position(const surgescript_object_t* object);
float scripting_util_world_angle(const surgescript_object_t* object);
v2d_t scripting_util_object_camera(const surgescript_object_t* object);
int scripting_util_is_object_inside_screen(const surgescript_object_t* object);
float scripting_util_object_zindex(surgescript_object_t* object);
const char* scripting_util_parent_name(const surgescript_object_t* object);
surgescript_object_t* scripting_util_surgeengine_object(surgescript_vm_t* vm);
surgescript_object_t* scripting_util_surgeengine_component(surgescript_vm_t* vm, const char* component_name);

#endif