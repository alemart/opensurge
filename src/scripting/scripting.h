/*
 * Open Surge Engine
 * scripting.h - scripting system
 * Copyright (C) 2018-2019  Alexandre Martins <alemartf@gmail.com>
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
#include "../core/v2d.h"
#include "../entities/brick.h"

/* scripting API */
void scripting_init(int argc, const char** argv);
void scripting_release();
bool scripting_testmode();
surgescript_vm_t* surgescript_vm(); /* SurgeScript VM instance */
void scripting_reload();

/* scripting utilities */
surgescript_objecthandle_t scripting_util_require_component(const surgescript_object_t* object, const char* component_name);
v2d_t scripting_util_world_position(const surgescript_object_t* object);
float scripting_util_world_angle(const surgescript_object_t* object);
void scripting_util_set_world_position(surgescript_object_t* object, v2d_t position);
void scripting_util_set_world_angle(surgescript_object_t* object, float angle);
v2d_t scripting_util_object_camera(const surgescript_object_t* object);
v2d_t scripting_util_parent_camera(const surgescript_object_t* object);
int scripting_util_is_object_inside_screen(const surgescript_object_t* object);
float scripting_util_object_zindex(surgescript_object_t* object);
const char* scripting_util_parent_name(const surgescript_object_t* object);
surgescript_object_t* scripting_util_surgeengine_object(surgescript_vm_t* vm);
surgescript_object_t* scripting_util_surgeengine_component(surgescript_vm_t* vm, const char* component_name);
surgescript_object_t* scripting_util_get_component(surgescript_object_t* object, const char* component_name);
surgescript_object_t* scripting_util_spawn_temp(surgescript_vm_t* vm, const char* object_name);
void scripting_error(const surgescript_object_t* object, const char* fmt, ...);

/* obtain data from objects */
struct actor_t;
struct animation_t;
struct collisionmask_t;
struct music_t;
struct player_t;
struct obstaclemap_t;

extern void scripting_vector2_update(surgescript_object_t* object, double x, double y);
extern void scripting_vector2_read(const surgescript_object_t* object, double* x, double* y);
extern v2d_t scripting_vector2_to_v2d(const surgescript_object_t* object);

extern struct actor_t* scripting_actor_ptr(const surgescript_object_t* object);
extern struct player_t* scripting_player_ptr(const surgescript_object_t* object);
extern struct music_t* scripting_music_ptr(const surgescript_object_t* object);

extern const struct animation_t* scripting_animation_ptr(const surgescript_object_t* object);
extern void scripting_animation_overwrite_id(const surgescript_object_t* object, int anim_id);

extern bricktype_t scripting_brick_type(const surgescript_object_t* object);
extern bricklayer_t scripting_brick_layer(const surgescript_object_t* object);
extern bool scripting_brick_enabled(const surgescript_object_t* object);
extern v2d_t scripting_brick_hotspot(const surgescript_object_t* object);
extern struct collisionmask_t* scripting_brick_mask(const surgescript_object_t* object);

extern struct obstaclemap_t* scripting_obstaclemap_ptr(const surgescript_object_t* object);

#endif