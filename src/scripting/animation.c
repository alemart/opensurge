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
#include "../entities/player.h"

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
static surgescript_var_t* fun_gethotspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getsprite(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getframe(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getframecount(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getspeedfactor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setspeedfactor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t ANIMID_ADDR = 0;
static const surgescript_heapptr_t SPRITENAME_ADDR = 1;
static const surgescript_heapptr_t HOTSPOT_ADDR = 2;
static const char* ONCHANGE = "onAnimationChange"; /* fun onAnimationChange(animation) will be called on the parent object */
static void notify_change(const surgescript_object_t* object);
static const actor_t* get_animation_actor(const surgescript_object_t* object);
extern actor_t* scripting_actor_ptr(const surgescript_object_t* object);
extern player_t* scripting_player_ptr(const surgescript_object_t* object);
extern void scripting_vector2_update(surgescript_object_t* object, double x, double y);

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
    surgescript_vm_bind(vm, "Animation", "get_hotspot", fun_gethotspot, 0);
    surgescript_vm_bind(vm, "Animation", "get_sprite", fun_getsprite, 0);
    surgescript_vm_bind(vm, "Animation", "get_frame", fun_getframe, 0);
    surgescript_vm_bind(vm, "Animation", "get_frameCount", fun_getframecount, 0);
    surgescript_vm_bind(vm, "Animation", "get_speedFactor", fun_getspeedfactor, 0);
    surgescript_vm_bind(vm, "Animation", "set_speedFactor", fun_setspeedfactor, 1);
}

/*
 * scripting_animation_ptr()
 * Returns a built-in animation_t*, given an Animation SurgeScript object
 */
const animation_t* scripting_animation_ptr(const surgescript_object_t* object)
{
    /* must always return a valid animation_t* */
    return (const animation_t*)surgescript_object_userdata(object);
}

/*
 * scripting_animation_overwrite_id()
 * Forces a new animation id without notifying changes
 */
void scripting_animation_overwrite_id(const surgescript_object_t* object, int anim_id)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_set_number(surgescript_heap_at(heap, ANIMID_ADDR), anim_id);
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
    surgescript_heap_t* heap = surgescript_object_heap(object);
    const char* parent_name = scripting_util_parent_name(object);
    animation_t* animation = sprite_get_animation(NULL, 0);
    surgescript_objecthandle_t hotspot = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);

    /* internal data */
    ssassert(ANIMID_ADDR == surgescript_heap_malloc(heap));
    ssassert(SPRITENAME_ADDR == surgescript_heap_malloc(heap));
    ssassert(HOTSPOT_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_number(surgescript_heap_at(heap, ANIMID_ADDR), 0);
    surgescript_var_set_string(surgescript_heap_at(heap, SPRITENAME_ADDR), "");
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, HOTSPOT_ADDR), hotspot);
    surgescript_object_set_userdata(object, animation);

    /* sanity check */
    if(strcmp(parent_name, "Actor") != 0 && strcmp(parent_name, "Player") != 0) {
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
    notify_change(object);
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
    notify_change(object);
    return NULL;
}

/* get animation id */
surgescript_var_t* fun_getid(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    return surgescript_var_clone(surgescript_heap_at(heap, ANIMID_ADDR));
}

/* get sprite name */
surgescript_var_t* fun_getsprite(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
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
    const actor_t* actor = get_animation_actor(object);
    return surgescript_var_set_bool(surgescript_var_create(), actor != NULL ? actor_animation_finished(actor) : true);
}

/* does the animation repeat? */
surgescript_var_t* fun_getrepeats(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const animation_t* animation = scripting_animation_ptr(object);
    return surgescript_var_set_bool(surgescript_var_create(), animation->repeat);
}

/* the (x,y) coordinates of the hotspot */
surgescript_var_t* fun_gethotspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, HOTSPOT_ADDR));
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, handle);
    const animation_t* animation = scripting_animation_ptr(object);

    scripting_vector2_update(v2, animation->hot_spot.x, animation->hot_spot.y);

    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* current frame, a number in [0, frameCount - 1] */
surgescript_var_t* fun_getframe(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const actor_t* actor = get_animation_actor(object);
    return surgescript_var_set_number(surgescript_var_create(), actor != NULL ? actor_animation_frame(actor) : 0);
}

/* the number of frames in the animation */
surgescript_var_t* fun_getframecount(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const animation_t* animation = scripting_animation_ptr(object);
    return surgescript_var_set_number(surgescript_var_create(), animation->frame_count);
}

/* animation speed factor (multiplier, defaults to 1.0) */
surgescript_var_t* fun_getspeedfactor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const actor_t* actor = get_animation_actor(object);
    return surgescript_var_set_number(surgescript_var_create(), actor != NULL ? actor->animation_speed_factor : 1.0);
}

/* set animation speed factor (no need to notify the parent) */
surgescript_var_t* fun_setspeedfactor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const actor_t* actor = get_animation_actor(object);
    double factor = surgescript_var_get_number(param[0]);

    if(actor != NULL)
        actor_change_animation_speed_factor(actor, factor);
    
    return NULL;
}


/* --- misc --- */

/* given an Animation object, return its corresponding actor_t* */
const actor_t* get_animation_actor(const surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    const char* parent_name = surgescript_object_name(parent);

    if(strcmp(parent_name, "Actor") == 0)
        return scripting_actor_ptr(parent);
    else if(strcmp(parent_name, "Player") == 0)
        return scripting_player_ptr(parent)->actor;
    else
        return NULL; /* this shouldn't happen */
}

/* notify the parent object about a change in the Animation (use when changing the animation to another one) */
void notify_change(const surgescript_object_t* object)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    surgescript_var_t* self = surgescript_var_set_objecthandle(surgescript_var_create(), me);
    const surgescript_var_t* p[] = { self };
    surgescript_object_call_function(parent, ONCHANGE, p, 1, NULL);
    surgescript_var_destroy(self);
}