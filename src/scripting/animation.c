/*
 * Open Surge Engine
 * animation.c - scripting system: animation object
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

#include <surgescript.h>
#include <string.h>
#include "scripting.h"
#include "../core/util.h"
#include "../core/sprite.h"
#include "../entities/actor.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setid(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getid(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfps(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfinished(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getrepeats(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gethotspotx(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gethotspoty(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getspritename(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t SPRITENAME_ADDR = 0;
static const surgescript_heapptr_t ANIMID_ADDR = 1;
extern actor_t* scripting_actor_ptr(const surgescript_object_t* object);

/*
 * scripting_register_animation()
 * Register this component
 */
void scripting_register_animation(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Animation", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Animation", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Animation", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "Animation", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "Animation", "__init", fun_init, 1);
    surgescript_vm_bind(vm, "Animation", "set_id", fun_setid, 1);
    surgescript_vm_bind(vm, "Animation", "get_id", fun_getid, 0);
    surgescript_vm_bind(vm, "Animation", "get_fps", fun_getfps, 0);
    surgescript_vm_bind(vm, "Animation", "get_finished", fun_getfinished, 0);
    surgescript_vm_bind(vm, "Animation", "get_repeats", fun_getrepeats, 0);
    surgescript_vm_bind(vm, "Animation", "get_hotspotX", fun_gethotspotx, 0);
    surgescript_vm_bind(vm, "Animation", "get_hotspotY", fun_gethotspoty, 0);
    surgescript_vm_bind(vm, "Animation", "get_spriteName", fun_getspritename, 0);
}

/*
 * scripting_animation_ptr()
 * Returns a built-in animation_t*, given an Animation SurgeScript object
 */
const animation_t* scripting_animation_ptr(const surgescript_object_t* object)
{
    return (const animation_t*)surgescript_object_userdata(object);
}

/* private */

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    const char* parent_name = scripting_util_parent_name(object);
    animation_t* animation = sprite_get_animation(NULL, 0);

    /* internal data */
    ssassert(SPRITENAME_ADDR == surgescript_heap_malloc(heap));
    ssassert(ANIMID_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_string(surgescript_heap_at(heap, SPRITENAME_ADDR), "");
    surgescript_var_set_number(surgescript_heap_at(heap, ANIMID_ADDR), 0);
    surgescript_object_set_userdata(object, animation);

    /* sanity check */
    if(strcmp(parent_name, "Actor") != 0) {
        fatal_error("Scripting Error: object \"%s\" can't spawn an Animation object.", parent_name);
        /* note: Animation.finished depends on the parent */
    }

    /* done! */
    return NULL;
}

/* spawn */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* can't spawn */
    return NULL;
}

/* destroy */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* can't destroy */
    return NULL;
}

/* __init(spriteName) */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    int anim_id = (int)surgescript_var_get_number(surgescript_heap_at(heap, ANIMID_ADDR));
    char* sprite_name = surgescript_var_get_string(param[0], manager);

    /* update data */
    if(sprite_animation_exists(sprite_name, anim_id)) {
        animation_t* animation = sprite_get_animation(sprite_name, anim_id);
        surgescript_var_set_string(surgescript_heap_at(heap, SPRITENAME_ADDR), sprite_name);
        surgescript_object_set_userdata(object, animation);
    }
    else {
        animation_t* animation = sprite_get_animation(NULL, 0);
        surgescript_var_set_string(surgescript_heap_at(heap, SPRITENAME_ADDR), sprite_name);
        surgescript_object_set_userdata(object, animation);
    }

    /* done! */
    ssfree(sprite_name);
    return NULL;
}

/* set animation id */
surgescript_var_t* fun_setid(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    const char* sprite_name = surgescript_var_fast_get_string(surgescript_heap_at(heap, SPRITENAME_ADDR));
    int anim_id = (int)surgescript_var_get_number(param[0]);

    /* update data */
    if(sprite_animation_exists(sprite_name, anim_id)) {
        animation_t* animation = sprite_get_animation(sprite_name, anim_id);
        surgescript_var_set_number(surgescript_heap_at(heap, ANIMID_ADDR), anim_id);
        surgescript_object_set_userdata(object, animation);
    }
    else {
        animation_t* animation = sprite_get_animation(NULL, 0);
        surgescript_var_set_number(surgescript_heap_at(heap, ANIMID_ADDR), anim_id);
        surgescript_object_set_userdata(object, animation);
    }

    /* done! */
    return NULL;
}

/* get animation id */
surgescript_var_t* fun_getid(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ANIMID_ADDR));
}

/* get sprite name */
surgescript_var_t* fun_getspritename(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, SPRITENAME_ADDR));
}

/* fps rate */
surgescript_var_t* fun_getfps(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const animation_t* animation = scripting_animation_ptr(object);
    return surgescript_var_set_number(surgescript_var_create(), animation->fps);
}

/* animation finished? */
surgescript_var_t* fun_getfinished(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    actor_t* actor = scripting_actor_ptr(parent);
    return surgescript_var_set_bool(surgescript_var_create(), actor_animation_finished(actor));
}

/* does the animation repeat? */
surgescript_var_t* fun_getrepeats(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const animation_t* animation = scripting_animation_ptr(object);
    return surgescript_var_set_bool(surgescript_var_create(), animation->repeat);
}

/* x-coordinate of the hotspot */
surgescript_var_t* fun_gethotspotx(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const animation_t* animation = scripting_animation_ptr(object);
    return surgescript_var_set_number(surgescript_var_create(), animation->hot_spot.x);
}

/* y-coordinate of the hotspot */
surgescript_var_t* fun_gethotspoty(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const animation_t* animation = scripting_animation_ptr(object);
    return surgescript_var_set_number(surgescript_var_create(), animation->hot_spot.y);
}
