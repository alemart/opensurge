/*
 * Open Surge Engine
 * brickparticle.c - scripting system: brick particle
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

#include <surgescript.h>
#include "scripting.h"
#include "../core/timer.h"
#include "../core/image.h"
#include "../core/video.h"
#include "../util/v2d.h"
#include "../util/util.h"
#include "../entities/brick.h"
#include "../entities/camera.h"
#include "../scenes/level.h"

/* brick particle data */
typedef struct particledata_t particledata_t;
struct particledata_t
{
    const image_t* image; /* possibly NULL */
    int src_x, src_y, width, height;
    double zindex;
    v2d_t velocity;
};

/* constants */
static const double DEFAULT_ZINDEX = 0.5;

/* SurgeScript functions */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfilepathofrenderable(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettexturehandle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getistranslucent(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setbrick(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvelocity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static inline particledata_t* get_particledata(const surgescript_object_t* object);
static particledata_t* create_particledata();
static particledata_t* destroy_particledata(particledata_t* pd);





/*
 * scripting_register_brickparticle()
 * Register this component
 */
void scripting_register_brickparticle(surgescript_vm_t* vm)
{
    /* tags */
    surgescript_tagsystem_t* tag_system = surgescript_vm_tagsystem(vm);
    surgescript_tagsystem_add_tag(tag_system, "BrickParticle", "renderable");
    surgescript_tagsystem_add_tag(tag_system, "BrickParticle", "entity");
    surgescript_tagsystem_add_tag(tag_system, "BrickParticle", "private");
    surgescript_tagsystem_add_tag(tag_system, "BrickParticle", "disposable");

    /* methods */
    surgescript_vm_bind(vm, "BrickParticle", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "BrickParticle", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "BrickParticle", "destructor", fun_destructor, 0);

    surgescript_vm_bind(vm, "BrickParticle", "setBrick", fun_setbrick, 5);
    surgescript_vm_bind(vm, "BrickParticle", "setVelocity", fun_setvelocity, 2);

    surgescript_vm_bind(vm, "BrickParticle", "set_zindex", fun_setzindex, 1);
    surgescript_vm_bind(vm, "BrickParticle", "get_zindex", fun_getzindex, 0);

    surgescript_vm_bind(vm, "BrickParticle", "get___filepathOfRenderable", fun_getfilepathofrenderable, 0);
    surgescript_vm_bind(vm, "BrickParticle", "get___textureHandle", fun_gettexturehandle, 0);
    surgescript_vm_bind(vm, "BrickParticle", "get___isTranslucent", fun_getistranslucent, 0);
    surgescript_vm_bind(vm, "BrickParticle", "onRender", fun_onrender, 0);
}




/*

SurgeScript functions

*/

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(object);
    particledata_t* pd = get_particledata(object);
    float dt = timer_get_delta();

    /* update velocity */
    float grv = level_gravity();
    pd->velocity.y += grv * dt;

    /* update position */
    float dx = pd->velocity.x * dt;
    float dy = pd->velocity.y * dt;
    surgescript_transform_translate2d(transform, dx, dy);

    /* this disposable entity will be removed automatically by the Entity Manager */

    /* done */
    return NULL;
}

/* render */
surgescript_var_t* fun_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    v2d_t position = v2d_new(0.0f, 0.0f);
    v2d_t camera = camera_get_position();

    /* nothing to do? */
    const particledata_t* pd = get_particledata(object);
    if(pd->image == NULL)
        return NULL; /* image not yet set */

    /* get position in world space */
    const surgescript_transform_t* transform = surgescript_object_transform(object);
    surgescript_transform_getposition2d(transform, &position.x, &position.y);

    /* convert position to screen space */
    v2d_t center_of_screen = v2d_multiply(video_get_screen_size(), 0.5f);
    v2d_t topleft_of_screen = v2d_subtract(camera, center_of_screen);
    position = v2d_subtract(position, topleft_of_screen);

    /* render */
    image_blit(
        pd->image,
        pd->src_x,
        pd->src_y,
        (int)position.x,
        (int)position.y,
        pd->width,
        pd->height
    );

    /* done */
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* create particle data */
    particledata_t* pd = create_particledata();
    surgescript_object_set_userdata(object, pd);

    /* done */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* destroy particle data */
    particledata_t* pd = get_particledata(object);
    destroy_particledata(pd);

    /* done */
    return NULL;
}

/* set brick */
surgescript_var_t* fun_setbrick(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    int brick_id = surgescript_var_get_number(param[0]);
    int src_x = surgescript_var_get_number(param[1]);
    int src_y = surgescript_var_get_number(param[2]);
    int width = surgescript_var_get_number(param[3]);
    int height = surgescript_var_get_number(param[4]);

    if(brick_exists(brick_id)) {
        const image_t* brick_image = brick_image_preview(brick_id);
        int brick_width = image_width(brick_image);
        int brick_height = image_height(brick_image);
        float zindex = brick_zindex_preview(brick_id);

        particledata_t* pd = get_particledata(object);
        pd->image = brick_image;
        pd->width = clip(width, 0, brick_width);
        pd->height = clip(height, 0, brick_height);
        pd->src_x = clip(src_x, 0, brick_width - pd->width);
        pd->src_y = clip(src_y, 0, brick_height - pd->height);
        pd->zindex = max(0.0f, zindex);
    }

    return NULL;
}

/* set velocity */
surgescript_var_t* fun_setvelocity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    double xvel = surgescript_var_get_number(param[0]);
    double yvel = surgescript_var_get_number(param[1]);

    particledata_t* pd = get_particledata(object);
    pd->velocity = v2d_new(xvel, yvel);

    return NULL;
}

/* set zindex */
surgescript_var_t* fun_setzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    double zindex = surgescript_var_get_number(param[0]);

    particledata_t* pd = get_particledata(object);
    pd->zindex = zindex;

    return NULL;
}

/* get zindex */
surgescript_var_t* fun_getzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const particledata_t* pd = get_particledata(object);
    return surgescript_var_set_number(surgescript_var_create(), pd->zindex);
}

/* the filepath of this renderable (used by the render queue) */
surgescript_var_t* fun_getfilepathofrenderable(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const particledata_t* pd = get_particledata(object);
    const image_t* image = pd->image;

    /* image already set? */
    if(image != NULL) {
        const char* filepath = image_filepath(image);
        return surgescript_var_set_string(surgescript_var_create(), filepath);
    }

    /* image not yet set */
    return surgescript_var_set_string(surgescript_var_create(), "<brick-particle>");
}

/* the texture handle of this renderable (used by the render queue) */
surgescript_var_t* fun_gettexturehandle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const particledata_t* pd = get_particledata(object);
    const image_t* image = pd->image;

    /* image already set? */
    if(image != NULL) {
        texturehandle_t tex = image_texture(image);
        return surgescript_var_set_rawbits(surgescript_var_create(), tex);
    }

    /* image not yet set */
    return surgescript_var_set_null(surgescript_var_create());
}

/* is this renderable translucent? (used by the render queue) */
surgescript_var_t* fun_getistranslucent(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bool is_translucent = false;
    return surgescript_var_set_bool(surgescript_var_create(), is_translucent);
}



/*

misc

*/

/* get particle data */
particledata_t* get_particledata(const surgescript_object_t* object)
{
    return (particledata_t*)surgescript_object_userdata(object);
}

/* create particle data */
particledata_t* create_particledata()
{
    particledata_t* pd = mallocx(sizeof *pd);

    pd->image = NULL;
    pd->src_x = pd->src_y = 0;
    pd->width = pd->height = 0;
    pd->zindex = DEFAULT_ZINDEX;
    pd->velocity = v2d_new(0.0f, 0.0f);

    return pd;
}

/* destroy particle data */
particledata_t* destroy_particledata(particledata_t* pd)
{
    free(pd);
    return NULL;
}