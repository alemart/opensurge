/*
 * Open Surge Engine
 * animation.c - scripting system: animation object
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
#include <string.h>
#include "scripting.h"
#include "../util/util.h"
#include "../util/stringutil.h"
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
static surgescript_var_t* fun_getduration(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfinished(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getrepeats(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gethotspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getactionspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getactionoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getsprite(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getframe(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setframe(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getframecount(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getspeedfactor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setspeedfactor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getsync(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setsync(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getexists(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_prop(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t ANIMID_ADDR = 0;
static const surgescript_heapptr_t SPRITENAME_ADDR = 1;
static const surgescript_heapptr_t HOTSPOT_ADDR = 2;
static const surgescript_heapptr_t ANCHOR_ADDR = 3;
static const surgescript_heapptr_t ACTIONSPOT_ADDR = 4;
static const surgescript_heapptr_t ACTIONOFFSET_ADDR = 5;
static const char* ONCHANGE = "onAnimationChange"; /* fun onAnimationChange(animation) will be called on the parent object */
static void notify_change(const surgescript_object_t* object);
static actor_t* get_animation_actor(const surgescript_object_t* object);
static const animation_t* null_animation();
static surgescript_var_t* convert_string_to_var(surgescript_var_t* var, const char* string);

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
    surgescript_vm_bind(vm, "Animation", "get_duration", fun_getduration, 0);
    surgescript_vm_bind(vm, "Animation", "get_finished", fun_getfinished, 0);
    surgescript_vm_bind(vm, "Animation", "get_repeats", fun_getrepeats, 0);
    surgescript_vm_bind(vm, "Animation", "get_anchor", fun_getanchor, 0);
    surgescript_vm_bind(vm, "Animation", "get_hotspot", fun_gethotspot, 0); /* legacy name kept for retro-compatibility with Open Surge 0.5.x */
    surgescript_vm_bind(vm, "Animation", "get_hotSpot", fun_gethotspot, 0);
    surgescript_vm_bind(vm, "Animation", "get_actionSpot", fun_getactionspot, 0);
    surgescript_vm_bind(vm, "Animation", "get_actionOffset", fun_getactionoffset, 0);
    surgescript_vm_bind(vm, "Animation", "get_sprite", fun_getsprite, 0);
    surgescript_vm_bind(vm, "Animation", "get_frame", fun_getframe, 0);
    surgescript_vm_bind(vm, "Animation", "set_frame", fun_setframe, 1);
    surgescript_vm_bind(vm, "Animation", "get_frameCount", fun_getframecount, 0);
    surgescript_vm_bind(vm, "Animation", "get_speedFactor", fun_getspeedfactor, 0);
    surgescript_vm_bind(vm, "Animation", "set_speedFactor", fun_setspeedfactor, 1);
    surgescript_vm_bind(vm, "Animation", "get_sync", fun_getsync, 0);
    surgescript_vm_bind(vm, "Animation", "set_sync", fun_setsync, 1);
    surgescript_vm_bind(vm, "Animation", "get_exists", fun_getexists, 0);
    surgescript_vm_bind(vm, "Animation", "prop", fun_prop, 1);
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
 * scripting_animation_overwrite_ptr()
 * Forces a new animation without notifying changes
 */
void scripting_animation_overwrite_ptr(surgescript_object_t* object, const animation_t* animation)
{
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* anim_id = surgescript_heap_at(heap, ANIMID_ADDR);

    /* transitions won't show up in scripting */
    if(animation_is_transition(animation))
        return;

    if(surgescript_var_get_number(anim_id) != animation_id(animation)) {
        surgescript_var_set_number(anim_id, animation_id(animation));
        surgescript_object_set_userdata(object, (void*)animation);
    }
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
    const animation_t* animation = null_animation();

    /* internal data */
    ssassert(ANIMID_ADDR == surgescript_heap_malloc(heap));
    ssassert(SPRITENAME_ADDR == surgescript_heap_malloc(heap));
    ssassert(HOTSPOT_ADDR == surgescript_heap_malloc(heap));
    ssassert(ANCHOR_ADDR == surgescript_heap_malloc(heap));
    ssassert(ACTIONSPOT_ADDR == surgescript_heap_malloc(heap));
    ssassert(ACTIONOFFSET_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_number(surgescript_heap_at(heap, ANIMID_ADDR), 0);
    surgescript_var_set_string(surgescript_heap_at(heap, SPRITENAME_ADDR), "");
    surgescript_var_set_null(surgescript_heap_at(heap, HOTSPOT_ADDR)); /* lazy evaluation */
    surgescript_var_set_null(surgescript_heap_at(heap, ANCHOR_ADDR)); /* lazy evaluation */
    surgescript_var_set_null(surgescript_heap_at(heap, ACTIONSPOT_ADDR)); /* lazy evaluation */
    surgescript_var_set_null(surgescript_heap_at(heap, ACTIONOFFSET_ADDR)); /* lazy evaluation */
    surgescript_object_set_userdata(object, (void*)animation);

    /* sanity check */
    if(strcmp(parent_name, "Actor") != 0 && strcmp(parent_name, "Player") != 0) {
        scripting_error(object, "Object \"%s\" can't spawn an Animation object.", parent_name);
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

    /* set sprite name */
    char* sprite_name = surgescript_var_get_string(param[0], manager);
    surgescript_var_set_string(surgescript_heap_at(heap, SPRITENAME_ADDR), sprite_name);

    /* update animation pointer */
    if(sprite_animation_exists(sprite_name, anim_id)) {
        const animation_t* animation = sprite_get_animation(sprite_name, anim_id);
        surgescript_object_set_userdata(object, (void*)animation);
    }
    else {
        const animation_t* animation = null_animation();
        surgescript_object_set_userdata(object, (void*)animation);
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
    surgescript_var_t* anim_ref = surgescript_heap_at(heap, ANIMID_ADDR);
    int anim_id = (int)surgescript_var_get_number(param[0]);

    /* no need to update? */
    if((int)surgescript_var_get_number(anim_ref) == anim_id) {
        notify_change(object); /* will enable custom player animation for this frame */
        return NULL;
    }

    /* update data */
    const char* sprite_name = surgescript_var_fast_get_string(surgescript_heap_at(heap, SPRITENAME_ADDR));
    if(sprite_animation_exists(sprite_name, anim_id)) {
        const animation_t* animation = sprite_get_animation(sprite_name, anim_id);
        surgescript_var_set_number(anim_ref, anim_id);
        surgescript_object_set_userdata(object, (void*)animation);
    }
    else {
        const animation_t* animation = null_animation();
        surgescript_var_set_number(anim_ref, anim_id);
        surgescript_object_set_userdata(object, (void*)animation);
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
    return surgescript_var_set_number(surgescript_var_create(), animation_fps(animation));
}

/* the duration of the animation, in seconds */
surgescript_var_t* fun_getduration(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const animation_t* animation = scripting_animation_ptr(object);
    return surgescript_var_set_number(surgescript_var_create(), animation_duration(animation));
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
    return surgescript_var_set_bool(surgescript_var_create(), animation_repeats(animation));
}

/* the (x,y) coordinates of the hotspot */
surgescript_var_t* fun_gethotspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* hot_spot = surgescript_heap_at(heap, HOTSPOT_ADDR);
    surgescript_objecthandle_t handle;
    surgescript_object_t* v2;
    const animation_t* animation = scripting_animation_ptr(object);
    v2d_t anim_hot_spot = animation_hot_spot(animation);

    /* lazy evaluation */
    if(surgescript_var_is_null(hot_spot)) {
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        handle = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
        surgescript_var_set_objecthandle(hot_spot, handle);
    }
    else
        handle = surgescript_var_get_objecthandle(hot_spot);

    /* get hotspot */
    v2 = surgescript_objectmanager_get(manager, handle);
    scripting_vector2_update(v2, anim_hot_spot.x, anim_hot_spot.y);
    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* the (x,y) coordinates of the hotspot normalized to [0,1] x [0,1] */
surgescript_var_t* fun_getanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* anchor = surgescript_heap_at(heap, ANCHOR_ADDR);
    surgescript_objecthandle_t handle;
    surgescript_object_t* v2;
    const animation_t* animation = scripting_animation_ptr(object);
    v2d_t hot_spot = animation_hot_spot(animation);
    double width = animation_frame_width(animation);
    double height = animation_frame_height(animation);

    /* lazy evaluation */
    if(surgescript_var_is_null(anchor)) {
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        handle = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
        surgescript_var_set_objecthandle(anchor, handle);
    }
    else
        handle = surgescript_var_get_objecthandle(anchor);

    /* get anchor */
    v2 = surgescript_objectmanager_get(manager, handle);
    scripting_vector2_update(v2, hot_spot.x / width, hot_spot.y / height);
    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* the (x,y) coordinates of the action spot */
surgescript_var_t* fun_getactionspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* action_spot = surgescript_heap_at(heap, ACTIONSPOT_ADDR);
    surgescript_objecthandle_t handle;
    surgescript_object_t* v2;

    /* get the action spot */
    const actor_t* actor = get_animation_actor(object);
    v2d_t spot = actor != NULL ? actor_action_spot(actor) : v2d_new(0, 0);

    /* lazy evaluation */
    if(surgescript_var_is_null(action_spot)) {
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        handle = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
        surgescript_var_set_objecthandle(action_spot, handle);
    }
    else
        handle = surgescript_var_get_objecthandle(action_spot);

    /* return the action spot */
    v2 = surgescript_objectmanager_get(manager, handle);
    scripting_vector2_update(v2, spot.x, spot.y);
    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* the (x,y) coordinates of the action offset */
surgescript_var_t* fun_getactionoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* action_offset = surgescript_heap_at(heap, ACTIONOFFSET_ADDR);
    surgescript_objecthandle_t handle;
    surgescript_object_t* v2;

    /* get the action offset */
    const actor_t* actor = get_animation_actor(object);
    v2d_t offset = actor != NULL ? actor_action_offset(actor) : v2d_new(0, 0);

    /* lazy evaluation */
    if(surgescript_var_is_null(action_offset)) {
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        handle = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
        surgescript_var_set_objecthandle(action_offset, handle);
    }
    else
        handle = surgescript_var_get_objecthandle(action_offset);

    /* return the action offset */
    v2 = surgescript_objectmanager_get(manager, handle);
    scripting_vector2_update(v2, offset.x, offset.y);
    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* get the current frame, a number in [0, frameCount - 1] */
surgescript_var_t* fun_getframe(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const actor_t* actor = get_animation_actor(object);
    return surgescript_var_set_number(surgescript_var_create(), actor != NULL ? actor_animation_frame(actor) : 0);
}

/* set the current frame, a number in [0, frameCount - 1] */
surgescript_var_t* fun_setframe(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = get_animation_actor(object);
    int frame = surgescript_var_get_number(param[0]);

    if(actor != NULL)
        actor_change_animation_frame(actor, frame);

    return NULL;
}

/* the number of frames in the animation */
surgescript_var_t* fun_getframecount(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const animation_t* animation = scripting_animation_ptr(object);
    return surgescript_var_set_number(surgescript_var_create(), animation_frame_count(animation));
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
    actor_t* actor = get_animation_actor(object);
    double factor = surgescript_var_get_number(param[0]);

    if(actor != NULL)
        actor_change_animation_speed_factor(actor, factor);
    
    return NULL;
}

/* is the animation synchronized? */
surgescript_var_t* fun_getsync(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const actor_t* actor = get_animation_actor(object);
    return surgescript_var_set_bool(surgescript_var_create(), actor != NULL ? actor->synchronized_animation : false);
}

/* makes the animation synchronized (or not) */
surgescript_var_t* fun_setsync(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = get_animation_actor(object);
    bool sync = surgescript_var_get_bool(param[0]);

    if(actor != NULL)
        actor_synchronize_animation(actor, sync);
    
    return NULL;
}

/* does this Animation exist? (i.e., is there a sprite and an animation number that correspond to this Animation object?) */
surgescript_var_t* fun_getexists(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const animation_t* animation = scripting_animation_ptr(object);
    return surgescript_var_set_bool(surgescript_var_create(), animation != null_animation());
}

/* read a user-defined custom property given its name. Returns null if no such property is defined */
surgescript_var_t* fun_prop(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const char* property_name = surgescript_var_fast_get_string(param[0]);
    const animation_t* animation = scripting_animation_ptr(object);
    const char* const* prop = animation_user_property(animation, property_name);
    surgescript_var_t* ret = surgescript_var_create();

    /* no such property exists */
    if(prop == NULL)
        return surgescript_var_set_null(ret);

    /* does the property have multiple elements or just a single one? */
    bool want_array = (prop[0] != NULL && prop[1] != NULL);
    if(!want_array) {
        /* the property has a single element */
        return convert_string_to_var(ret, prop[0]);
    }
    else {
        /* spawn an array */
        surgescript_objectmanager_t* manager = surgescript_object_manager(object);
        surgescript_objecthandle_t array_handle = surgescript_objectmanager_spawn_array(manager);
        surgescript_object_t* array = surgescript_objectmanager_get(manager, array_handle);

        /* for each element of the user-defined custom property, call array.push(element) */
        surgescript_var_t* tmp = ret;
        for(const char* const* it = prop; *it != NULL; it++) {
            convert_string_to_var(tmp, *it);
            surgescript_object_call_function(array, "push", (const surgescript_var_t*[]){ tmp }, 1, NULL);
        }

        /* return the new array */
        return surgescript_var_set_objecthandle(ret, array_handle);
    }
}


/* --- misc --- */

/* given an Animation object, return its corresponding actor_t* */
actor_t* get_animation_actor(const surgescript_object_t* object)
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

/* returns a pre-defined NULL animation */
const animation_t* null_animation()
{
    return sprite_get_animation(NULL, 0);
}

/* convert a string to a SurgeScript variable. The type of the variable depends on its contents */
surgescript_var_t* convert_string_to_var(surgescript_var_t* var, const char* string)
{
    if(string == NULL)
        return surgescript_var_set_null(var);

    if(str_is_numeric(string))
        return surgescript_var_set_number(var, atof(string));

    if(str_is_boolean(string))
        return surgescript_var_set_bool(var, atob(string));

    return surgescript_var_set_string(var, string);
}