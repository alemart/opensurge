/*
 * Open Surge Engine
 * actor.c - scripting system: actor component
 * Copyright (C) 2018  Alexandre Martins <alemartf@gmail.com>
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
#include "scripting.h"
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
static surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
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
static surgescript_var_t* fun_getanimation(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettransform(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_onanimationchange(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static v2d_t world_lossyscale(const surgescript_object_t* object);
static const surgescript_heapptr_t ZINDEX_ADDR = 0;
static const surgescript_heapptr_t TRANSFORM_ADDR = 1;
static const surgescript_heapptr_t DETACHED_ADDR = 2;
static const surgescript_heapptr_t ANIMATION_ADDR = 3;
static const surgescript_heapptr_t OFFSET_ADDR = 4;
static const double DEFAULT_ZINDEX = 0.5;
static const double DEG2RAD = 0.01745329251994329576;
static inline surgescript_object_t* get_animation(surgescript_object_t* object);

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
    surgescript_vm_bind(vm, "Actor", "__init", fun_init, 1);
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
    surgescript_vm_bind(vm, "Actor", "get_animation", fun_getanimation, 0);
    surgescript_vm_bind(vm, "Actor", "get_width", fun_getwidth, 0);
    surgescript_vm_bind(vm, "Actor", "get_height", fun_getheight, 0);
    surgescript_vm_bind(vm, "Actor", "get_transform", fun_gettransform, 0);
    surgescript_vm_bind(vm, "Actor", "get_entity", fun_getentity, 0);
    surgescript_vm_bind(vm, "Actor", "get_offset", fun_getoffset, 0);
    surgescript_vm_bind(vm, "Actor", "set_offset", fun_setoffset, 1);
    surgescript_vm_bind(vm, "Actor", "onAnimationChange", fun_onanimationchange, 1);
}

/*
 * scripting_actor_ptr()
 * Returns a built-in actor_t*, given a SurgeScript Actor object
 */
actor_t* scripting_actor_ptr(const surgescript_object_t* object)
{
    return (actor_t*)surgescript_object_userdata(object);
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
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t offset = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
    surgescript_objecthandle_t transform = scripting_util_require_component(object, "Transform");
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    bool is_detached = surgescript_object_has_tag(parent, "detached");
    surgescript_heap_t* heap = surgescript_object_heap(object);
    actor_t* actor = actor_create();

    /* internal data */
    ssassert(ZINDEX_ADDR == surgescript_heap_malloc(heap));
    ssassert(TRANSFORM_ADDR == surgescript_heap_malloc(heap));
    ssassert(DETACHED_ADDR == surgescript_heap_malloc(heap));
    ssassert(ANIMATION_ADDR == surgescript_heap_malloc(heap));
    ssassert(OFFSET_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_number(surgescript_heap_at(heap, ZINDEX_ADDR), DEFAULT_ZINDEX);
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, TRANSFORM_ADDR), transform);
    surgescript_var_set_bool(surgescript_heap_at(heap, DETACHED_ADDR), is_detached);
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, ANIMATION_ADDR),
        surgescript_objectmanager_spawn(manager, me, "Animation", NULL)
    );
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, OFFSET_ADDR), offset);

    /* initial configuration */
    surgescript_object_set_userdata(object, actor);
    actor_change_animation(actor, sprite_get_animation(NULL, 0));
    actor->spawn_point = scripting_util_world_position(object);

    /* sanity check */
    if(!surgescript_object_has_tag(parent, "entity")) {
        const char* parent_name = surgescript_object_name(parent);
        scripting_error(object, "Object \"%s\" spawns an Actor. Hence, it should be tagged as an \"entity\".", parent_name);
    }

    /* done! */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    actor_destroy(actor);
    return NULL;
}

/* render */
surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    bool is_detached = surgescript_var_get_bool(surgescript_heap_at(heap, DETACHED_ADDR));
    v2d_t camera = !is_detached ? camera_get_position() : v2d_new(VIDEO_SCREEN_W / 2, VIDEO_SCREEN_H / 2);
    actor_t* actor = scripting_actor_ptr(object);

    actor->position = scripting_util_world_position(object);
    actor->angle = scripting_util_world_angle(object) * DEG2RAD;
    actor->scale = world_lossyscale(object); /* FIXME: use A5 transformations */

    actor_render(actor, camera);
    return NULL;
}

/* __init: set sprite name */
surgescript_var_t* fun_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* animation = get_animation(object);
    const surgescript_var_t* p[] = { param[0] };
    surgescript_object_call_function(animation, "__init", p, 1, NULL);
    return NULL;
}

/* set zindex */
surgescript_var_t* fun_setzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    double zindex = surgescript_var_get_number(param[0]);
    surgescript_var_set_number(surgescript_heap_at(heap, ZINDEX_ADDR), zindex);
    return NULL;
}

/* get zindex (defaults to DEFAULT_ZINDEX) */
surgescript_var_t* fun_getzindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ZINDEX_ADDR));
}

/* set animation number */
surgescript_var_t* fun_setanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* call animation.set_id */
    surgescript_object_t* animation = get_animation(object);
    const surgescript_var_t* p[] = { param[0] };
    surgescript_object_call_function(animation, "set_id", p, 1, NULL);
    return NULL;
}

/* get animation number */
surgescript_var_t* fun_getanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* call animation.get_id */
    surgescript_object_t* animation = get_animation(object);
    surgescript_var_t* anim_id = surgescript_var_create();
    surgescript_object_call_function(animation, "get_id", NULL, 0, anim_id);
    return anim_id;
}

/* get animation object */
surgescript_var_t* fun_getanimation(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ANIMATION_ADDR));
}

/* animation change callback */
surgescript_var_t* fun_onanimationchange(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t animation_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* animation = surgescript_objectmanager_get(manager, animation_handle);
    actor_t* actor = scripting_actor_ptr(object);
    actor_change_animation(actor, scripting_animation_ptr(animation));
    return NULL;
}

/* get horizontal flip */
surgescript_var_t* fun_gethflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    return surgescript_var_set_bool(surgescript_var_create(), actor->mirror & IF_HFLIP);
}

/* set horizontal flip */
surgescript_var_t* fun_sethflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    bool hflip = surgescript_var_get_bool(param[0]);
    actor->mirror = hflip ? (actor->mirror | IF_HFLIP) : (actor->mirror & ~IF_HFLIP);
    return NULL;
}

/* get vertical flip */
surgescript_var_t* fun_getvflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    return surgescript_var_set_bool(surgescript_var_create(), actor->mirror & IF_VFLIP);
}

/* set vertical flip */
surgescript_var_t* fun_setvflip(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    bool vflip = surgescript_var_get_bool(param[0]);
    actor->mirror = vflip ? (actor->mirror | IF_VFLIP) : (actor->mirror & ~IF_VFLIP);
    return NULL;
}

/* get 0.0 (invisible) <= alpha <= 1.0 (opaque) */
surgescript_var_t* fun_getalpha(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    return surgescript_var_set_number(surgescript_var_create(), actor->alpha);
}

/* set alpha */
surgescript_var_t* fun_setalpha(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    float alpha = surgescript_var_get_number(param[0]);
    actor->alpha = clip(alpha, 0.0f, 1.0f);
    return NULL;
}

/* is this actor visible? */
surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    return surgescript_var_set_bool(surgescript_var_create(), actor->visible);
}

/* set actor visibility */
surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    actor->visible = surgescript_var_get_bool(param[0]);
    return NULL;
}

/* actor frame width */
surgescript_var_t* fun_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    return surgescript_var_set_number(surgescript_var_create(), image_width(actor_image(actor)));
}

/* actor frame height */
surgescript_var_t* fun_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    return surgescript_var_set_number(surgescript_var_create(), image_height(actor_image(actor)));
}

/* get the local transform */
surgescript_var_t* fun_gettransform(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, TRANSFORM_ADDR));
}

/* get the associated entity */
surgescript_var_t* fun_getentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objecthandle_t parent = surgescript_object_parent(object);
    return surgescript_var_set_objecthandle(surgescript_var_create(), parent);
}

/* get offset */
surgescript_var_t* fun_getoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, OFFSET_ADDR));
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, handle);

    scripting_vector2_update(v2, transform->position.x, transform->position.y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* set offset */
surgescript_var_t* fun_setoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double x = 0.0, y = 0.0;

    scripting_vector2_read(surgescript_objectmanager_get(manager, v2h), &x, &y);
    transform->position.x = x;
    transform->position.y = y;

    return NULL;
}






/* --- helpers --- */

/* computes the approximate scale */
v2d_t world_lossyscale(const surgescript_object_t* object)
{
    v2d_t scale;
    surgescript_transform_util_lossyscale2d(object, &scale.x, &scale.y);
    return scale;
}

/* get the Animation SurgeScript object (child object) */
surgescript_object_t* get_animation(surgescript_object_t* object)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t animation_handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, ANIMATION_ADDR));
    surgescript_object_t* animation = surgescript_objectmanager_get(manager, animation_handle);
    return animation;
}