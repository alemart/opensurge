/*
 * Open Surge Engine
 * actor.c - scripting system: actor component
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
#include <math.h>
#include "../core/v2d.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../core/util.h"
#include "../entities/actor.h"
#include "../entities/camera.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_sethflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gethflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setalpha(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getalpha(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setsprite(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getsprite(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettransform(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_animationfinished(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
extern surgescript_objecthandle_t require_component(const surgescript_object_t* object, const char* component_name);
extern v2d_t world_position(const surgescript_object_t* object);
static float world_angle(const surgescript_object_t* object);
static v2d_t world_lossyscale(const surgescript_object_t* object);
static const surgescript_heapptr_t ZINDEX_ADDR = 0;
static const surgescript_heapptr_t TRANSFORM_ADDR = 1;
static const surgescript_heapptr_t DETACHED_ADDR = 2;
static const surgescript_heapptr_t SPRITE_ADDR = 3;
static const surgescript_heapptr_t ANIM_ADDR = 4;
static const float DEFAULT_ZINDEX = 0.5f;
static const char* DEFAULT_SPRITE = "SD_QUESTIONMARK";
static const int DEFAULT_ANIM = 0;
static const double DEG2RAD = 0.01745329251994329576;

/*
 * scripting_register_actor()
 * Register this component
 */
void scripting_register_actor(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "Actor", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Actor", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Actor", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "Actor", "render", fun_render, 0);
    surgescript_vm_bind(vm, "Actor", "set_zindex", fun_setzindex, 1);
    surgescript_vm_bind(vm, "Actor", "get_zindex", fun_getzindex, 0);
    surgescript_vm_bind(vm, "Actor", "get_hflip", fun_gethflip, 0);
    surgescript_vm_bind(vm, "Actor", "set_hflip", fun_sethflip, 1);
    surgescript_vm_bind(vm, "Actor", "get_vflip", fun_getvflip, 0);
    surgescript_vm_bind(vm, "Actor", "set_vflip", fun_setvflip, 1);
    surgescript_vm_bind(vm, "Actor", "set_alpha", fun_setalpha, 1);
    surgescript_vm_bind(vm, "Actor", "get_alpha", fun_getalpha, 0);
    surgescript_vm_bind(vm, "Actor", "set_visible", fun_setvisible, 1);
    surgescript_vm_bind(vm, "Actor", "get_visible", fun_getvisible, 0);
    surgescript_vm_bind(vm, "Actor", "set_anim", fun_setanim, 1);
    surgescript_vm_bind(vm, "Actor", "get_anim", fun_getanim, 0);
    surgescript_vm_bind(vm, "Actor", "set___sprite", fun_setsprite, 1);
    surgescript_vm_bind(vm, "Actor", "get___sprite", fun_getsprite, 0);
    surgescript_vm_bind(vm, "Actor", "get_width", fun_getwidth, 0);
    surgescript_vm_bind(vm, "Actor", "get_height", fun_getheight, 0);
    surgescript_vm_bind(vm, "Actor", "get_transform", fun_gettransform, 0);
    surgescript_vm_bind(vm, "Actor", "animationFinished", fun_animationfinished, 0);
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
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t transform = require_component(object, "Transform2D");
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    bool is_detached = surgescript_object_has_tag(parent, "detached");
    surgescript_heap_t* heap = surgescript_object_heap(object);
    actor_t* actor = actor_create();

    /* internal data */
    ssassert(ZINDEX_ADDR == surgescript_heap_malloc(heap));
    ssassert(TRANSFORM_ADDR == surgescript_heap_malloc(heap));
    ssassert(DETACHED_ADDR == surgescript_heap_malloc(heap));
    ssassert(SPRITE_ADDR == surgescript_heap_malloc(heap));
    ssassert(ANIM_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_number(surgescript_heap_at(heap, ZINDEX_ADDR), DEFAULT_ZINDEX);
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, TRANSFORM_ADDR), transform);
    surgescript_var_set_bool(surgescript_heap_at(heap, DETACHED_ADDR), is_detached);
    surgescript_var_set_string(surgescript_heap_at(heap, SPRITE_ADDR), DEFAULT_SPRITE);
    surgescript_var_set_number(surgescript_heap_at(heap, ANIM_ADDR), DEFAULT_ANIM);

    /* initial configuration */
    surgescript_object_set_userdata(object, actor);
    actor_change_animation(actor, sprite_get_animation(DEFAULT_SPRITE, DEFAULT_ANIM));
    actor->spawn_point = world_position(object);

    /* sanity check */
    if(!surgescript_object_has_tag(parent, "entity")) {
        const char* parent_name = surgescript_object_name(parent);
        fatal_error("Scripting Error: object \"%s\" spawns an Actor. Hence, it should be tagged as an \"entity\".", parent_name);
    }

    /* done! */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    actor_destroy(actor);
    return NULL;
}

/* render */
surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    bool is_detached = surgescript_var_get_bool(surgescript_heap_at(heap, DETACHED_ADDR));
    v2d_t camera = !is_detached ? camera_get_position() : v2d_new(VIDEO_SCREEN_W / 2, VIDEO_SCREEN_H / 2);
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);

    actor->position = world_position(object);
    actor->angle = world_angle(object) * DEG2RAD;
    actor->scale = world_lossyscale(object);

    actor_render(actor, camera);
    return NULL;
}

/* set zindex */
surgescript_var_t* fun_setzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_copy(surgescript_heap_at(heap, ZINDEX_ADDR), param[0]);
    return NULL;
}

/* get zindex (defaults to DEFAULT_ZINDEX) */
surgescript_var_t* fun_getzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ZINDEX_ADDR));
}

/* set sprite name */
surgescript_var_t* fun_setsprite(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    const char* prev_sprite_name = surgescript_var_fast_get_string(surgescript_heap_at(heap, SPRITE_ADDR));
    char* sprite_name = surgescript_var_get_string(param[0], surgescript_object_manager(object));
    if(strcmp(sprite_name, prev_sprite_name) != 0) {
        int anim_id = (int)surgescript_var_get_number(surgescript_heap_at(heap, ANIM_ADDR));
        surgescript_var_set_string(surgescript_heap_at(heap, SPRITE_ADDR), sprite_name);
        actor_change_animation(actor, sprite_get_animation(sprite_name, anim_id));
    }
    ssfree(sprite_name);
    return NULL;
}

/* get sprite name */
surgescript_var_t* fun_getsprite(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, SPRITE_ADDR));
}

/* set animation number */
surgescript_var_t* fun_setanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    int anim_id = (int)surgescript_var_get_number(param[0]);
    int prev_anim_id = (int)surgescript_var_get_number(surgescript_heap_at(heap, ANIM_ADDR));
    if(prev_anim_id != anim_id) {
        const char* sprite_name = surgescript_var_fast_get_string(surgescript_heap_at(heap, SPRITE_ADDR));
        surgescript_var_set_number(surgescript_heap_at(heap, ANIM_ADDR), anim_id);
        actor_change_animation(actor, sprite_get_animation(sprite_name, anim_id));
    }
    return NULL;
}

/* get animation number */
surgescript_var_t* fun_getanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ANIM_ADDR));
}

/* has the animation finished playing? */
surgescript_var_t* fun_animationfinished(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    return surgescript_var_set_bool(surgescript_var_create(), actor_animation_finished(actor));
}

/* get horizontal flip */
surgescript_var_t* fun_gethflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    return surgescript_var_set_bool(surgescript_var_create(), actor->mirror & IF_HFLIP);
}

/* set horizontal flip */
surgescript_var_t* fun_sethflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    bool hflip = surgescript_var_get_bool(param[0]);
    actor->mirror = hflip ? (actor->mirror | IF_HFLIP) : (actor->mirror & ~IF_HFLIP);
    return NULL;
}

/* get vertical flip */
surgescript_var_t* fun_getvflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    return surgescript_var_set_bool(surgescript_var_create(), actor->mirror & IF_VFLIP);
}

/* set vertical flip */
surgescript_var_t* fun_setvflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    bool vflip = surgescript_var_get_bool(param[0]);
    actor->mirror = vflip ? (actor->mirror | IF_VFLIP) : (actor->mirror & ~IF_VFLIP);
    return NULL;
}

/* get 0.0 (invisible) <= alpha <= 1.0 (opaque) */
surgescript_var_t* fun_getalpha(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    return surgescript_var_set_number(surgescript_var_create(), actor->alpha);
}

/* set alpha */
surgescript_var_t* fun_setalpha(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    float alpha = surgescript_var_get_number(param[0]);
    actor->alpha = clip(alpha, 0.0f, 1.0f);
    return NULL;
}

/* is this actor visible? */
surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    return surgescript_var_set_bool(surgescript_var_create(), actor->visible);
}

/* set actor visibility */
surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    actor->visible = surgescript_var_get_bool(param[0]);
    return NULL;
}

/* actor frame width */
surgescript_var_t* fun_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    return surgescript_var_set_number(surgescript_var_create(), image_width(actor_image(actor)));
}

/* actor frame height */
surgescript_var_t* fun_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = (actor_t*)surgescript_object_userdata(object);
    return surgescript_var_set_number(surgescript_var_create(), image_height(actor_image(actor)));
}

/* get the local transform */
surgescript_var_t* fun_gettransform(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, TRANSFORM_ADDR));
}




/* --- helpers --- */

/* compute the world angle of an object */
float world_angle(const surgescript_object_t* object)
{
    surgescript_transform_t transform;
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, surgescript_object_parent(object));
    float parent_angle = (parent != object) ? world_angle(parent) : 0.0f;
    surgescript_object_peek_transform(object, &transform);
    return parent_angle + transform.rotation.z;
}

/* computes the approximate scale (not very accurate; does not account for shearing) */
v2d_t world_lossyscale(const surgescript_object_t* object)
{
    static const float eps = 1e-5;
    surgescript_transform_t transform;
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);   
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, surgescript_object_parent(object));
    v2d_t scale = (parent != object) ? world_lossyscale(parent) : v2d_new(1.0f, 1.0f);

    surgescript_object_peek_transform(object, &transform);
    if(transform.scale.x >= 1.0f - eps && transform.scale.x <= 1.0f + eps && transform.scale.y >= 1.0f - eps && transform.scale.y <= 1.0f + eps)
        return scale;

    scale.x *= transform.scale.x;
    scale.y *= transform.scale.y;
    return v2d_new(ssmax(scale.x, 0), ssmax(scale.y, 0));
}