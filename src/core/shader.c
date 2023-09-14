/*
 * Open Surge Engine
 * shader.c - managed shaders
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

#include <allegro5/allegro.h>
#include "shader.h"
#include "../util/dictionary.h"
#include "../util/iterator.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../core/logfile.h"

/* shader struct */
struct shader_t
{
    ALLEGRO_SHADER* shader; /* the first field */
    char* fs;
    char* vs;
};

/* default vertex shader */
static const char default_vs_glsl[] = ""
    "#define a_position " ALLEGRO_SHADER_VAR_POS "\n"
    "#define a_color " ALLEGRO_SHADER_VAR_COLOR "\n"
    "#define a_texcoord " ALLEGRO_SHADER_VAR_TEXCOORD "\n"
    "#define projview " ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX "\n"
    "#define use_texmatrix " ALLEGRO_SHADER_VAR_USE_TEX_MATRIX "\n"
    "#define texmatrix " ALLEGRO_SHADER_VAR_TEX_MATRIX "\n"

    "attribute vec4 a_position;\n"
    "attribute vec4 a_color;\n"
    "attribute vec2 a_texcoord;\n"

    "varying vec4 v_color;\n"
    "varying vec2 v_texcoord;\n"

    "uniform mat4 projview;\n"
    "uniform mat4 texmatrix;\n"
    "uniform bool use_texmatrix;\n"

    "void main()\n"
    "{\n"
    "   mat4 m = use_texmatrix ? texmatrix : mat4(1.0);\n"
    "   vec4 uv = m * vec4(a_texcoord, 0.0, 1.0);\n"

    "   v_texcoord = uv.xy;\n"
    "   v_color = a_color;\n"

    "   gl_Position = projview * a_position;\n"
    "}\n"
"";

/* default fragment shader */
static const char default_fs_glsl[] = ""
    "#ifdef GL_ES\n"
    "precision lowp float;\n"
    "#endif\n"

    "#define use_tex " ALLEGRO_SHADER_VAR_USE_TEX "\n"
    "#define tex " ALLEGRO_SHADER_VAR_TEX "\n"

    "uniform sampler2D tex;\n"
    "uniform bool use_tex;\n"
    "varying vec4 v_color;\n" /* tint */
    "varying vec2 v_texcoord;\n"

    "const vec3 MASK_COLOR = vec3(1.0, 0.0, 1.0);\n" /* magenta */

    "void main()\n"
    "{\n"
    "   vec4 p = use_tex ? texture2D(tex, v_texcoord) : vec4(1.0);\n"
    "   p *= float(p.rgb != MASK_COLOR);\n" /* set all components to zero; we use a premultiplied alpha workflow */

    "   gl_FragColor = v_color * p;\n"
    "}\n"
"";

/* Helpers */
#define LOG(...)            logfile_message("Shader - " __VA_ARGS__)
#define FATAL(...)          fatal_error("Shader - " __VA_ARGS__)
static ALLEGRO_SHADER* create_glsl_shader(const char* fs_glsl, const char* vs_glsl, char* error_string, size_t error_string_size);
static ALLEGRO_SHADER* destroy_glsl_shader(ALLEGRO_SHADER* shader);
static void destroy_shader_callback(void* shader, void* context);
static void destroy_shader(shader_t* shader);
static void discard_shader(shader_t* shader);
static void recreate_shader(shader_t* shader);
static const char DEFAULT_SHADER_NAME[] = "default";
static shader_t* default_shader = NULL;
static dictionary_t* registry = NULL;



/*
 * shader_init()
 * Initialize the shader system
 */
void shader_init()
{
    /* initialize the registry of shaders */
    registry = dictionary_create(false, destroy_shader_callback, NULL);

    /* create the default shader */
    default_shader = shader_create_ex(DEFAULT_SHADER_NAME, default_fs_glsl, default_vs_glsl);
}

/*
 * shader_release()
 * Deinitialize the shader system
 */
void shader_release()
{
    /* use Allegro's default shader */
    al_use_shader(NULL);

    /* destroy the registry of shaders (as well as each registered shader) */
    registry = dictionary_destroy(registry);

    /* reset the default_shader pointer (already released) */
    default_shader = NULL;
}

/*
 * shader_discard_all()
 * Discard all registered shaders (e.g., due to a change of context)
 */
void shader_discard_all()
{
    LOG("Discarding all shaders...");

    /* use Allegro's default shader */
    al_use_shader(NULL);

    /* discard all shaders */
    iterator_t* it = dictionary_values(registry);

    while(iterator_has_next(it)) {
        shader_t* shader = iterator_next(it);
        discard_shader(shader);
    }

    iterator_destroy(it);
}

/*
 * shader_recreate_all()
 * Recreate all registered shaders after discarding them
 */
void shader_recreate_all()
{
    LOG("Recreating all shaders...");

    /* recreate all shaders */
    iterator_t* it = dictionary_values(registry);

    while(iterator_has_next(it)) {
        shader_t* shader = iterator_next(it);
        recreate_shader(shader);
    }

    iterator_destroy(it);
}




/*
 * shader_exists()
 * Checks if a managed shader exists
 */
bool shader_exists(const char* name)
{
    return (registry != NULL) && dictionary_get(registry, name) != NULL;
}

/*
 * shader_get()
 * Get a managed shader by its name
 */
shader_t* shader_get(const char* name)
{
    assertx(registry != NULL);

    shader_t* shader = dictionary_get(registry, name);
    if(shader == NULL)
        FATAL("Can't find shader \"%s\"", name);

    return shader;
}

/*
 * shader_create()
 * Create a managed shader given the code of a fragment shader
 */
shader_t* shader_create(const char* name, const char* fs_glsl)
{
    return shader_create_ex(name, fs_glsl, default_vs_glsl);
}

/*
 * shader_create_ex()
 * Create a managed shader given the code of a fragment and of a vertex shader
 */
shader_t* shader_create_ex(const char* name, const char* fs_glsl, const char* vs_glsl)
{
    shader_t* shader = mallocx(sizeof *shader);
    char error[256] = "";

    /* create GLSL shader */
    shader->shader = create_glsl_shader(fs_glsl, vs_glsl, error, sizeof error);
    if(shader->shader == NULL) {
        LOG("Can't create shader!");
        FATAL("%s", error);
    }

    /* store the source code */
    shader->vs = str_dup(vs_glsl);
    shader->fs = str_dup(fs_glsl);

    /* register the shader */
    assertx(registry != NULL);
    dictionary_put(registry, name, shader);

    /* done! */
    return shader;
}

/*
 * shader_use()
 * Use the shader for the next drawing operations on the current target image
 * Pass NULL to use the default shader. Returns true on success
 */
bool shader_use(const shader_t* shader)
{
    /* According to the Allegro manual, al_use_shader() "uses the shader for
       subsequent drawing operations on the current target bitmap".

       https://liballeg.org/a5docs/trunk/shader.html */

    if(shader != NULL)
        return al_use_shader(shader->shader);
    else if(default_shader != NULL)
        return al_use_shader(default_shader->shader);
    else
        return false;
}




/*
 *
 * private
 *
 */

/* create a GLSL shader. On error, returns NULL and sets an error string */
ALLEGRO_SHADER* create_glsl_shader(const char* fs_glsl, const char* vs_glsl, char* error_string, size_t error_string_size)
{
    ALLEGRO_SHADER* sh = al_create_shader(ALLEGRO_SHADER_GLSL);

    if(sh == NULL) {
        snprintf(error_string, error_string_size, "Can't create GLSL shader");
    }
    else if(!al_attach_shader_source(sh, ALLEGRO_VERTEX_SHADER, vs_glsl)) {
        snprintf(error_string, error_string_size, "Can't compile the vertex shader. %s", al_get_shader_log(sh));
        al_destroy_shader(sh);
        sh = NULL;
    }
    else if(!al_attach_shader_source(sh, ALLEGRO_PIXEL_SHADER, fs_glsl)) {
        snprintf(error_string, error_string_size, "Can't compile the fragment shader. %s", al_get_shader_log(sh));
        al_destroy_shader(sh);
        sh = NULL;
    }
    else if(!al_build_shader(sh)) {
        snprintf(error_string, error_string_size, "Can't build the shader. %s", al_get_shader_log(sh));
        al_destroy_shader(sh);
        sh = NULL;
    }

    return sh;
}

/* destroy a GLSL shader */
ALLEGRO_SHADER* destroy_glsl_shader(ALLEGRO_SHADER* shader)
{
    al_destroy_shader(shader);
    return NULL;
}

/* callback: destroy a shader */
void destroy_shader_callback(void* shader, void* context)
{
    destroy_shader((shader_t*)shader);
    (void)context;
}

/* destroy a shader instance */
void destroy_shader(shader_t* shader)
{
    /* release the source code */
    free(shader->fs);
    free(shader->vs);

    /* release the GLSL shader */
    destroy_glsl_shader(shader->shader);

    /* release the instance */
    free(shader);
}

/* discard a shader */
void discard_shader(shader_t* shader)
{
    assertx(shader->shader != NULL);

    shader->shader = destroy_glsl_shader(shader->shader);
}

/* recreate a shader (after discarding it) */
void recreate_shader(shader_t* shader)
{
    char error[256] = "";
    assertx(shader->shader == NULL);

    shader->shader = create_glsl_shader(shader->fs, shader->vs, error, sizeof error);
    if(shader->shader == NULL) {
        LOG("Can't recreate shader!");
        FATAL("%s", error);
    }
}
