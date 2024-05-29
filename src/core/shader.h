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



/*
 * Shader templates
 */

#define GLSL_ES_VERSION_DIRECTIVE \
    "#version 300 es\n  \n"

#define GLSL_VERSION_DIRECTIVE \
    "#version 330 core\n\n"

#if defined(__ANDROID__)
#define SHADER_GLSL_PREFIX GLSL_ES_VERSION_DIRECTIVE
#else
#define SHADER_GLSL_PREFIX GLSL_VERSION_DIRECTIVE
#endif

#define FRAGMENT_SHADER_GLSL_PREFIX "" \
    FRAGMENT_SHADER_GLSL_PREFIX_EX("lowp")

#define FRAGMENT_SHADER_GLSL_PREFIX_EX(default_precision) "" \
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

#define VERTEX_SHADER_GLSL_PREFIX "" \
    SHADER_GLSL_PREFIX \
    \
    "#define a_position " ALLEGRO_SHADER_VAR_POS "\n" \
    "#define a_color " ALLEGRO_SHADER_VAR_COLOR "\n" \
    "#define a_texcoord " ALLEGRO_SHADER_VAR_TEXCOORD "\n" \
    "#define projview " ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX "\n" \
    "#define use_texmatrix " ALLEGRO_SHADER_VAR_USE_TEX_MATRIX "\n" \
    "#define texmatrix " ALLEGRO_SHADER_VAR_TEX_MATRIX "\n" \
    \
    "precision highp float;\n" \
    \
    "in vec4 a_position;\n" \
    "in vec4 a_color;\n" \
    "in vec2 a_texcoord;\n" \
    \
    "out vec4 v_color;\n" \
    "out vec2 v_texcoord;\n" \
    ""

#endif
