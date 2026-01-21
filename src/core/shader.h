/*
 * Open Surge Engine
 * shader.h - managed shaders
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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
void shader_set_int_vector(shader_t* shader, const char* var_name, int num_components, const int* value);
void shader_set_sampler(shader_t* shader, const char* var_name, const struct image_t* image);

/* Use GLSL ES shaders? */
#ifndef WANT_GLES
#define WANT_GLES 0
#endif

/*
 * Shader templates
 */

#if WANT_GLES

#define SHADER_GLSL_PREFIX() \
    "#version 100\n" \
    "#ifndef GL_FRAGMENT_PRECISION_HIGH\n" \
    "# define highp mediump\n" \
    "#endif\n"

#else

#define SHADER_GLSL_PREFIX() \
    "#version 120\n" \
    "#define lowp\n" \
    "#define mediump\n" \
    "#define highp\n"

#endif

#define FRAGMENT_SHADER_GLSL_PREFIX(default_precision) "" \
    SHADER_GLSL_PREFIX() \
    \
    "#define use_tex " ALLEGRO_SHADER_VAR_USE_TEX "\n" \
    "#define tex " ALLEGRO_SHADER_VAR_TEX "\n" \
    "#define texcoord v_texcoord\n" \
    \
    "#ifdef GL_ES\n" \
    "precision " default_precision " float;\n" /* highp may not be available (GLSL ES 1.0) */ \
    "#endif\n" \
    \
    "varying highp vec2 v_texcoord;\n" \
    "varying lowp vec4 v_color;\n" /* tint color */ \
    ""

#define VERTEX_SHADER_GLSL_PREFIX() "" \
    SHADER_GLSL_PREFIX() \
    \
    "#define a_position " ALLEGRO_SHADER_VAR_POS "\n" \
    "#define a_color " ALLEGRO_SHADER_VAR_COLOR "\n" \
    "#define a_texcoord " ALLEGRO_SHADER_VAR_TEXCOORD "\n" \
    "#define projview " ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX "\n" \
    "#define use_texmatrix " ALLEGRO_SHADER_VAR_USE_TEX_MATRIX "\n" \
    "#define texmatrix " ALLEGRO_SHADER_VAR_TEX_MATRIX "\n" \
    \
    "#ifdef GL_ES\n" \
    "# ifdef GL_FRAGMENT_PRECISION_HIGH\n" \
    "precision highp float;\n" \
    "# else\n" \
    "precision mediump float;\n" \
    "# endif\n" \
    "#endif\n" \
    \
    "attribute vec4 a_position;\n" \
    "attribute vec4 a_color;\n" \
    "attribute vec2 a_texcoord;\n" \
    \
    "varying vec4 v_color;\n" \
    "varying vec2 v_texcoord;\n" \
    \
    "uniform mat4 projview;\n" \
    "uniform mat4 texmatrix;\n" \
    "uniform bool use_texmatrix;\n" \
    ""

#define VERTEX_SHADER_GLSL_INFIX(main) "" \
    "void " main "()\n" \
    "{\n" \
    "   mat4 m = use_texmatrix ? texmatrix : mat4(1.0);\n" \
    "   vec4 uv = m * vec4(a_texcoord, 0.0, 1.0);\n" \
    \
    "   v_texcoord = uv.xy;\n" \
    "   v_color = a_color;\n" \
    \
    "   gl_Position = projview * a_position;\n" \
    "}\n" \
    ""

#endif
