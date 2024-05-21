/*
 * Open Surge Engine
 * shader.c - managed shaders
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

#include <allegro5/allegro.h>
#include "shader.h"
#include "../util/dictionary.h"
#include "../util/iterator.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../util/numeric.h"
#include "../core/logfile.h"
#include "../core/image.h"
#include "../core/video.h"

/* shader struct */
struct shader_t
{
    ALLEGRO_SHADER* shader; /* the first field */
    char* fs;
    char* vs;
    dictionary_t* uniforms;
    int next_texture_unit;
};

/* uniform variables */
typedef struct shader_uniform_t shader_uniform_t;
typedef enum shader_uniformtype_t shader_uniformtype_t;
#define UNIFORM_NAME_MAXLEN 63

enum shader_uniformtype_t
{
    TYPE_FLOAT,
    TYPE_INT,
    TYPE_BOOL,

    TYPE_FLOAT2, /* TYPE_FLOAT_(k+1) = TYPE_FLOAT_k + 1 */
    TYPE_FLOAT3,
    TYPE_FLOAT4,

    TYPE_SAMPLER_0, /* TYPE_SAMPLER_k := TYPE_SAMPLER_0 + k */
    TYPE_SAMPLER_1,
    TYPE_SAMPLER_2,
    TYPE_SAMPLER_3,
    TYPE_SAMPLER_4,
    TYPE_SAMPLER_5,
    TYPE_SAMPLER_6,
    TYPE_SAMPLER_7,
    TYPE_SAMPLER_8,
    TYPE_SAMPLER_9,
    TYPE_SAMPLER_10,
    TYPE_SAMPLER_11,
    TYPE_SAMPLER_12,
    TYPE_SAMPLER_13,
    TYPE_SAMPLER_14,
    TYPE_SAMPLER_15
};

struct shader_uniform_t
{
    shader_uniformtype_t type;
    char name[1 + UNIFORM_NAME_MAXLEN];
    union {
        float f;
        int i;
        bool b;
        float fvec[4];
        const image_t* tex;
    } value;
};

static shader_uniform_t* create_uniform(shader_uniformtype_t type, const char* var_name);
static void destroy_uniform(shader_uniform_t* uniform);
static bool set_uniform(const shader_uniform_t* uniform);
static void uniform_dtor(void *uniform, void* ctx) { destroy_uniform((shader_uniform_t*)uniform); (void)ctx; }

/* default vertex shader */
static const char default_vs_glsl[] = ""
    SHADER_GLSL_PREFIX

    "#define a_position " ALLEGRO_SHADER_VAR_POS "\n"
    "#define a_color " ALLEGRO_SHADER_VAR_COLOR "\n"
    "#define a_texcoord " ALLEGRO_SHADER_VAR_TEXCOORD "\n"
    "#define projview " ALLEGRO_SHADER_VAR_PROJVIEW_MATRIX "\n"
    "#define use_texmatrix " ALLEGRO_SHADER_VAR_USE_TEX_MATRIX "\n"
    "#define texmatrix " ALLEGRO_SHADER_VAR_TEX_MATRIX "\n"

    "precision highp float;\n"

    "in vec4 a_position;\n"
    "in vec4 a_color;\n"
    "in vec2 a_texcoord;\n"

    "out vec4 v_color;\n"
    "out vec2 v_texcoord;\n"

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
    FRAGMENT_SHADER_GLSL_PREFIX("lowp")

    "uniform sampler2D tex;\n"
    "uniform bool use_tex;\n"

    "const vec3 MASK_COLOR = vec3(1.0, 0.0, 1.0);\n" /* magenta */

    "void main()\n"
    "{\n"
    "   vec4 p = use_tex ? texture(tex, v_texcoord) : vec4(1.0);\n"
    "   p *= float(p.rgb != MASK_COLOR);\n" /* set all components to zero; we use a premultiplied alpha workflow */

    "   color = v_color * p;\n"
    "}\n"
"";

/* static assertions */
static const char glsl_version_prefix[] = SHADER_GLSL_PREFIX;
static const char glsl_es_version_prefix[] = SHADER_GLSL_ES_PREFIX;
STATIC_ASSERTX(sizeof glsl_version_prefix == sizeof glsl_es_version_prefix, glsl_version_prefix_of_same_size);

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
static const shader_t* active_shader = NULL;
static dictionary_t* registry = NULL;



/*
 * shader_init()
 * Initialize the shader system
 */
void shader_init()
{
    LOG("Initializing...");
    default_shader = NULL;
    active_shader = NULL;

    /* initialize the registry of shaders */
    registry = dictionary_create(false, destroy_shader_callback, NULL);

    /* create the default shader */
    default_shader = shader_create_ex(DEFAULT_SHADER_NAME, default_fs_glsl, default_vs_glsl);

    /* use the default shader */
    shader_set_active(default_shader);
    assertx(active_shader == default_shader);
}

/*
 * shader_release()
 * Deinitialize the shader system
 */
void shader_release()
{
    LOG("Releasing...");

    /* use Allegro's default shader */
    al_use_shader(NULL);

    /* destroy the registry of shaders (as well as each registered shader) */
    registry = dictionary_destroy(registry);

    /* reset the default_shader pointer (already released) */
    default_shader = NULL;

    /* reset the active_shader pointer */
    active_shader = NULL;
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
    return (registry != NULL) && (dictionary_get(registry, name) != NULL);
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

    /* log */
    LOG("Creating shader \"%s\"...", name);

    /* create GLSL shader */
    shader->shader = create_glsl_shader(fs_glsl, vs_glsl, error, sizeof error);
    if(shader->shader == NULL) {
        LOG("Can't create shader!");
        FATAL("%s", error);
    }

    /* store the source code */
    shader->vs = str_dup(vs_glsl);
    shader->fs = str_dup(fs_glsl);

    /* create the dictionary of uniforms */
    shader->uniforms = dictionary_create(true, uniform_dtor, NULL);

    /* set the next texture unit */
    shader->next_texture_unit = 1; /* unit 0 is used by Allegro */

    /* register the shader */
    assertx(registry != NULL);
    dictionary_put(registry, name, shader);

    /* done! */
    return shader;
}

/*
 * shader_set_active()
 * Use the shader for the subsequent drawing operations on the current target image
 * Returns true on success
 */
bool shader_set_active(const shader_t* shader)
{
    /* According to the Allegro manual, al_use_shader() "uses the shader for
       subsequent drawing operations on the current target bitmap".

       https://liballeg.org/a5docs/trunk/shader.html */

    /* use the shader */
    bool success = al_use_shader(shader->shader);

    /* set uniform variables */
    if(success) {
        iterator_t* it = dictionary_values(shader->uniforms);
        while(iterator_has_next(it)) {
            const shader_uniform_t* uniform = iterator_next(it);
            set_uniform(uniform);
        }
        iterator_destroy(it);
    }

    /* update active shader */
    if(success)
        active_shader = shader;

    /* done! */
    return success;
}

/*
 * shader_get_active()
 * Get the shader currently used for subsequent drawing operations
 */
const shader_t* shader_get_active()
{
    assertx(active_shader != NULL);
    return active_shader;
}

/*
 * shader_get_default()
 * Get the default shader
 */
const shader_t* shader_get_default()
{
    assertx(default_shader != NULL);
    return default_shader;
}

/*
 * shader_set_float()
 * Set the value of a floating-point uniform variable
 */
void shader_set_float(shader_t* shader, const char* var_name, float value)
{
    shader_uniform_t* stored_uniform = dictionary_get(shader->uniforms, var_name);

    if(stored_uniform == NULL) {
        /* add new uniform */
        stored_uniform = create_uniform(TYPE_FLOAT, var_name);
        stored_uniform->value.f = value;
        dictionary_put(shader->uniforms, var_name, stored_uniform);
    }
    else {
        /* update uniform */
        assertx(stored_uniform->type == TYPE_FLOAT, "Can't change uniform type");
        stored_uniform->value.f = value;
    }
}

/*
 * shader_set_int()
 * Set the value of an integer uniform variable
 */
void shader_set_int(shader_t* shader, const char* var_name, int value)
{
    shader_uniform_t* stored_uniform = dictionary_get(shader->uniforms, var_name);

    if(stored_uniform == NULL) {
        /* add new uniform */
        stored_uniform = create_uniform(TYPE_INT, var_name);
        stored_uniform->value.i = value;
        dictionary_put(shader->uniforms, var_name, stored_uniform);
    }
    else {
        /* update uniform */
        assertx(stored_uniform->type == TYPE_INT, "Can't change uniform type");
        stored_uniform->value.i = value;
    }
}

/*
 * shader_set_bool()
 * Set the value of a boolean uniform variable
 */
void shader_set_bool(shader_t* shader, const char* var_name, bool value)
{
    shader_uniform_t* stored_uniform = dictionary_get(shader->uniforms, var_name);

    if(stored_uniform == NULL) {
        /* add new uniform */
        stored_uniform = create_uniform(TYPE_BOOL, var_name);
        stored_uniform->value.b = value;
        dictionary_put(shader->uniforms, var_name, stored_uniform);
    }
    else {
        /* update uniform */
        assertx(stored_uniform->type == TYPE_BOOL, "Can't change uniform type");
        stored_uniform->value.b = value;
    }
}

/*
 * shader_set_float_vector()
 * Set the value of a floating-point vector of the given number of components
 */
void shader_set_float_vector(shader_t* shader, const char* var_name, int num_components, const float* value)
{
    assertx(num_components >= 2 && num_components <= 4);
    shader_uniform_t* stored_uniform = dictionary_get(shader->uniforms, var_name);

    if(stored_uniform == NULL) {
        /* add new uniform */
        stored_uniform = create_uniform(TYPE_FLOAT2 + (num_components-2), var_name);
        memcpy(stored_uniform->value.fvec, value, num_components * sizeof(*value));
        dictionary_put(shader->uniforms, var_name, stored_uniform);
    }
    else {
        /* update uniform */
        assertx(stored_uniform->type == TYPE_FLOAT2 + (num_components-2), "Can't change uniform type");
        memcpy(stored_uniform->value.fvec, value, num_components * sizeof(*value));
    }
}

/*
 * shader_set_sampler()
 * Set a texture sampler
 */
void shader_set_sampler(shader_t* shader, const char* var_name, const image_t* image)
{
    shader_uniform_t* stored_uniform = dictionary_get(shader->uniforms, var_name);

    /* set the texture unit */
    int unit = (stored_uniform == NULL) ? shader->next_texture_unit++ : (int)stored_uniform->type - TYPE_SAMPLER_0;
    assertx(unit >= 0 && unit <= 15);

    if(stored_uniform == NULL) {
        /* add new uniform */
        stored_uniform = create_uniform(TYPE_SAMPLER_0 + unit, var_name);
        stored_uniform->value.tex = image;
        dictionary_put(shader->uniforms, var_name, stored_uniform);
    }
    else {
        /* update uniform */
        assertx(stored_uniform->type >= TYPE_SAMPLER_0 && stored_uniform->type <= TYPE_SAMPLER_15, "Can't change uniform type");
        stored_uniform->value.tex = image;
    }
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
    char *fs = str_dup(fs_glsl), *vs = str_dup(vs_glsl);
    char *xs[] = { vs, fs, NULL };

    /* validate the #version line - replace it if necessary */
    bool want_glsl_es = video_is_using_gles(); /* will crash if using OpenGL ES 2 with GLSL ES 3 shaders */
    for(char** glsl = xs; *glsl != NULL; glsl++) {
        if(!want_glsl_es)
            assertx(0 == strncmp(*glsl, glsl_version_prefix, strlen(glsl_version_prefix)));
        else if(0 != strncmp(*glsl, glsl_version_prefix, strlen(glsl_version_prefix)))
            assertx(0 == strncmp(*glsl, glsl_es_version_prefix, strlen(glsl_es_version_prefix)));
        else
            memcpy(*glsl, glsl_es_version_prefix, strlen(glsl_es_version_prefix));
    }

    /* create the shader */
    if(sh == NULL) {
        snprintf(error_string, error_string_size, "Can't create GLSL shader");
    }
    else if(!al_attach_shader_source(sh, ALLEGRO_VERTEX_SHADER, vs)) {
        snprintf(error_string, error_string_size, "Can't compile the vertex shader. %s\n\n%s", al_get_shader_log(sh), vs);
        al_destroy_shader(sh);
        sh = NULL;
    }
    else if(!al_attach_shader_source(sh, ALLEGRO_PIXEL_SHADER, fs)) {
        snprintf(error_string, error_string_size, "Can't compile the fragment shader. %s\n\n%s", al_get_shader_log(sh), fs);
        al_destroy_shader(sh);
        sh = NULL;
    }
    else if(!al_build_shader(sh)) {
        snprintf(error_string, error_string_size, "Can't build the shader. %s\n\n%s", al_get_shader_log(sh), fs);
        al_destroy_shader(sh);
        sh = NULL;
    }

    free(vs);
    free(fs);

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
    /* release the dictionary of uniforms */
    dictionary_destroy(shader->uniforms);

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

/* create a uniform sturct */
shader_uniform_t* create_uniform(shader_uniformtype_t type, const char* var_name)
{
    /* validate */
    if(*var_name == '\0')
        FATAL("Empty name");
    else if(strlen(var_name) > UNIFORM_NAME_MAXLEN)
        FATAL("Name is too long: %s", var_name);

    /* create uniform */
    shader_uniform_t* uniform = mallocx(sizeof *uniform);
    memset(uniform, 0, sizeof *uniform);

    /* initialize */
    uniform->type = type;
    str_cpy(uniform->name, var_name, sizeof uniform->name);

    /* done! */
    return uniform;
}

/* destroy a uniform sturct */
void destroy_uniform(shader_uniform_t* uniform)
{
    free(uniform);
}

/* set value of uniform variable (current shader) */
bool set_uniform(const shader_uniform_t* uniform)
{
    switch(uniform->type) {
        case TYPE_FLOAT:
            return al_set_shader_float(uniform->name, uniform->value.f);

        case TYPE_INT:
            return al_set_shader_int(uniform->name, uniform->value.i);

        case TYPE_BOOL:
            return al_set_shader_bool(uniform->name, uniform->value.b);

        case TYPE_FLOAT2:
            return al_set_shader_float_vector(uniform->name, 2, uniform->value.fvec, 1);

        case TYPE_FLOAT3:
            return al_set_shader_float_vector(uniform->name, 3, uniform->value.fvec, 1);

        case TYPE_FLOAT4:
            return al_set_shader_float_vector(uniform->name, 4, uniform->value.fvec, 1);

        case TYPE_SAMPLER_0:
        case TYPE_SAMPLER_1:
        case TYPE_SAMPLER_2:
        case TYPE_SAMPLER_3:
        case TYPE_SAMPLER_4:
        case TYPE_SAMPLER_5:
        case TYPE_SAMPLER_6:
        case TYPE_SAMPLER_7:
        case TYPE_SAMPLER_8:
        case TYPE_SAMPLER_9:
        case TYPE_SAMPLER_10:
        case TYPE_SAMPLER_11:
        case TYPE_SAMPLER_12:
        case TYPE_SAMPLER_13:
        case TYPE_SAMPLER_14:
        case TYPE_SAMPLER_15:
            return al_set_shader_sampler(uniform->name, IMAGE2BITMAP(uniform->value.tex), (int)uniform->type - TYPE_SAMPLER_0);
    }

    return false;
}