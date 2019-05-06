/*
 * Open Surge Engine
 * brick.c - scripting system: brick-like object
 * Copyright (C) 2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdbool.h>
#include <surgescript.h>
#include "../core/util.h"
#include "../core/sprite.h"
#include "../entities/brick.h"
#include "../physics/collisionmask.h"

/* brick-like object structure */
typedef struct bricklike_data_t bricklike_data_t;
struct bricklike_data_t {
    bricktype_t type;
    bricklayer_t layer;
    collisionmask_t* mask;
    v2d_t hot_spot;
    bool enabled;
};

/* private */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettype(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_settype(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getlayer(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setlayer(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static inline bricklike_data_t* get_data(const surgescript_object_t* object);
static const int BRICKLIKE_ANIMATION_ID = 0; /* which animation number should be used to extract the collision mask? */

/*
 * scripting_register_brick()
 * Register the object
 */
void scripting_register_brick(surgescript_vm_t* vm)
{
    /* tags */
    surgescript_tagsystem_t* tag_system = surgescript_vm_tagsystem(vm);
    surgescript_tagsystem_add_tag(tag_system, "Brick", "entity");
    surgescript_tagsystem_add_tag(tag_system, "Brick", "private");

    /* methods */
    surgescript_vm_bind(vm, "Brick", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Brick", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Brick", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Brick", "__init", fun_init, 1);
    surgescript_vm_bind(vm, "Brick", "get_type", fun_gettype, 0);
    surgescript_vm_bind(vm, "Brick", "set_type", fun_settype, 1);
    surgescript_vm_bind(vm, "Brick", "get_layer", fun_getlayer, 0);
    surgescript_vm_bind(vm, "Brick", "set_layer", fun_setlayer, 1);
    surgescript_vm_bind(vm, "Brick", "get_enabled", fun_getenabled, 0);
    surgescript_vm_bind(vm, "Brick", "set_enabled", fun_setenabled, 1);
}

/*
 * scripting_brick_type()
 * Checks the type of a brick-like object
 * WARNING: Be sure that the referenced object is a Brick. This function won't check it.
 */
bricktype_t scripting_brick_type(const surgescript_object_t* object)
{
    const bricklike_data_t* data = get_data(object);
    return data->type;
}

/*
 * scripting_brick_layer()
 * Checks the layer of a brick-like object
 * WARNING: Be sure that the referenced object is a Brick. This function won't check it.
 */
bricklayer_t scripting_brick_layer(const surgescript_object_t* object)
{
    const bricklike_data_t* data = get_data(object);
    return data->layer;
}

/*
 * scripting_brick_enabled()
 * Checks if the given brick-like object is enabled
 * WARNING: Be sure that the referenced object is a Brick. This function won't check it.
 */
bool scripting_brick_enabled(const surgescript_object_t* object)
{
    const bricklike_data_t* data = get_data(object);
    return data->enabled;
}

/*
 * scripting_brick_hotspot()
 * Returns the hot spot of the sprite associated with a brick-like object
 * WARNING: Be sure that the referenced object is a Brick. This function won't check it.
 */
v2d_t scripting_brick_hotspot(const surgescript_object_t* object)
{
    const bricklike_data_t* data = get_data(object);
    return data->hot_spot;
}

/*
 * scripting_brick_mask()
 * Returns the collision mask associated with a brick-like object
 * This function may return NULL (if the associated sprite doesn't exist)
 * WARNING: Be sure that the referenced object is a Brick. This function won't check it.
 */
collisionmask_t* scripting_brick_mask(const surgescript_object_t* object)
{
    const bricklike_data_t* data = get_data(object);
    return data->mask;
}



/* Console routines */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t root = surgescript_objectmanager_root(manager);
    surgescript_objecthandle_t parent = surgescript_object_parent(object);
    bricklike_data_t* data = mallocx(sizeof *data);

    /* default values */
    data->type = BRK_SOLID;
    data->layer = BRL_DEFAULT;
    data->mask = NULL;
    data->hot_spot = v2d_new(0, 0);
    data->enabled = true;
    surgescript_object_set_userdata(object, data);

    /* sanity check */
    while(!surgescript_object_has_tag(surgescript_objectmanager_get(manager, parent), "entity")) {
        parent = surgescript_object_parent(surgescript_objectmanager_get(manager, parent));
        if(parent == root) {
            fatal_error(
                "Scripting Error: object \"%s\" must be a descendant of an entity (parent is \"%s\")",
                surgescript_object_name(object),
                surgescript_object_name(surgescript_objectmanager_get(manager, surgescript_object_parent(object)))
            );
            break;
        }
    }

    /* done */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bricklike_data_t* data = get_data(object);

    if(data->mask != NULL)
        collisionmask_destroy(data->mask);

    free(data);
    return NULL;
}

/* init function: receives a sprite name and computes the bricklike_data */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    char* sprite_name = surgescript_var_get_string(param[0], manager);
    int anim_id = BRICKLIKE_ANIMATION_ID;
    animation_t* animation = sprite_animation_exists(sprite_name, anim_id) ? sprite_get_animation(sprite_name, anim_id) : sprite_get_animation(NULL, 0);
    image_t* brick_image = sprite_get_image(animation, 0); /* get the first frame of the animation */
    bricklike_data_t* data = get_data(object);

    if(data->mask != NULL)
        collisionmask_destroy(data->mask);

    data->mask = collisionmask_create(brick_image, 0, 0, image_width(brick_image), image_height(brick_image));
    data->hot_spot = animation->hot_spot;

    ssfree(sprite_name);
    return NULL;
}

/* gets the solidity of the brick. One of the following: "solid", "cloud" */
surgescript_var_t* fun_gettype(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bricklike_data_t* data = get_data(object);

    switch(data->type) {
        case BRK_SOLID:
            return surgescript_var_set_string(surgescript_var_create(), "solid");

        case BRK_CLOUD:
            return surgescript_var_set_string(surgescript_var_create(), "cloud");

        default:
            return surgescript_var_set_string(surgescript_var_create(), "");
    }
}

/* sets the solidity of the brick to one of the following: "solid", "cloud" */
surgescript_var_t* fun_settype(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* type = surgescript_var_fast_get_string(param[0]);
    bricklike_data_t* data = get_data(object);

    if(strcmp(type, "solid") == 0)
        data->type = BRK_SOLID;
    else if(strcmp(type, "cloud") == 0)
        data->type = BRK_CLOUD;

    return NULL;
}

/* get the layer of the brick. One of the following: "green", "yellow", "default" */
surgescript_var_t* fun_getlayer(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bricklike_data_t* data = get_data(object);

    switch(data->layer) {
        case BRL_GREEN:
            return surgescript_var_set_string(surgescript_var_create(), "green");

        case BRL_YELLOW:
            return surgescript_var_set_string(surgescript_var_create(), "yellow");

        case BRL_DEFAULT:
            return surgescript_var_set_string(surgescript_var_create(), "default");
    }

    return surgescript_var_set_string(surgescript_var_create(), "");
}

/* set the layer of the brick to one of the following: "green", "yellow", "default" */
surgescript_var_t* fun_setlayer(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* layer = surgescript_var_fast_get_string(param[0]);
    bricklike_data_t* data = get_data(object);

    if(strcmp(layer, "green") == 0)
        data->layer = BRL_GREEN;
    else if(strcmp(layer, "yellow") == 0)
        data->layer = BRL_YELLOW;
    else if(strcmp(layer, "default") == 0)
        data->layer = BRL_DEFAULT;

    return NULL;
}

/* checks if the brick is enabled */
surgescript_var_t* fun_getenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bricklike_data_t* data = get_data(object);
    return surgescript_var_set_bool(surgescript_var_create(), data->enabled);
}

/* enables/disables the brick */
surgescript_var_t* fun_setenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    bricklike_data_t* data = get_data(object);
    data->enabled = surgescript_var_get_bool(param[0]);
    return NULL;
}

/* -- private -- */

/* gets the brickdata structure (without checking the validity of the object) */
bricklike_data_t* get_data(const surgescript_object_t* object)
{
    return (bricklike_data_t*)(surgescript_object_userdata(object));
}

/* djb2 hash function */
uint64_t hash(const char *str)
{
    int c; uint64_t hash = 5381;

    while((c = *((unsigned char*)(str++))))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}