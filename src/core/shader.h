/*
 * Open Surge Engine
 * shader.h - managed shaders
 * Copyright 2008-2024 Alexandre Martins <alemartf(at)gmail.com>
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

#ifndef _SHADER_H
#define _SHADER_H

#include <allegro5/allegro.h>

typedef struct shader_t shader_t;
struct image_t;

void shader_init();
void shader_release();
void shader_discard_all();
void shader_recreate_all();

shader_t* shader_create(const char* name, const char* fs_glsl);
shader_t* shader_create_ex(const char* name, const char* fs_glsl, const char* vs_glsl);
shader_t* shader_get(const char* name);
bool shader_exists(const char* name);

bool shader_set_active(const shader_t* shader);
const shader_t* shader_get_active();
const shader_t* shader_get_default();

void shader_set_float(shader_t* shader, const char* var_name, float value);
void shader_set_int(shader_t* shader, const char* var_name, int value);
void shader_set_bool(shader_t* shader, const char* var_name, bool value);
void shader_set_float_vector(shader_t* shader, const char* var_name, int num_components, const float* value);
void shader_set_sampler(shader_t* shader, const char* var_name, const struct image_t* image);

#if !defined(__ANDROID__)
#define SHADER_GLSL_PREFIX "#version 330 core\n"
#else
#define SHADER_GLSL_PREFIX "#version 300 es\n"
#endif

#define FRAGMENT_SHADER_GLSL_PREFIX(default_precision) "" \
    SHADER_GLSL_PREFIX \
    \
    "#define use_tex " ALLEGRO_SHADER_VAR_USE_TEX "\n" \
    "#define tex " ALLEGRO_SHADER_VAR_TEX "\n" \
    "#define texcoord v_texcoord\n" \
    \
    "precision " default_precision " float;\n" \
    \
    "in highp vec2 v_texcoord;\n" \
    "in lowp vec4 v_color;\n" /* tint color */ \
    "out lowp vec4 color;\n" /* fragment color */ \
    ""

#endif
