/*
 * Open Surge Engine
 * actor.c - scripting system: actor component
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

#include <surgescript.h>
#include <string.h>
#include <math.h>
#include "scripting.h"
#include "../core/video.h"
#include "../core/image.h"
#include "../util/v2d.h"
#include "../util/numeric.h"
#include "../util/util.h"
#include "../entities/actor.h"
#include "../entities/camera.h"

/* private */
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_canbeclippedout(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getfilepathofrenderable(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettexturehandle(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getistranslucent(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
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
static surgescript_var_t* fun_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gettransform(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getanimation(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getanim(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_gethotspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getactionspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getactionoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_onanimationchange(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static v2d_t world_lossyscale(const surgescript_object_t* object);
static const surgescript_heapptr_t ZINDEX_ADDR = 0;
static const surgescript_heapptr_t TRANSFORM_ADDR = 1;
static const surgescript_heapptr_t DETACHED_ADDR = 2;
static const surgescript_heapptr_t ANIMATION_ADDR = 3;
static const surgescript_heapptr_t OFFSET_ADDR = 4;
static const double DEFAULT_ZINDEX = 0.5;
static inline surgescript_object_t* get_animation(surgescript_object_t* object);

/*
 * scripting_register_actor()
 * Register this component
 */
void scripting_register_actor(surgescript_vm_t* vm)
{
    /* tags */
    surgescript_tagsystem_t* tag_system = surgescript_vm_tagsystem(vm);
    surgescript_tagsystem_add_tag(tag_system, "Actor", "renderable");

    /* methods */
    surgescript_vm_bind(vm, "Actor", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "Actor", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "Actor", "destructor", fun_destructor, 0);
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
    surgescript_vm_bind(vm, "Actor", "get_width", fun_getwidth, 0);
    surgescript_vm_bind(vm, "Actor", "get_height", fun_getheight, 0);
    surgescript_vm_bind(vm, "Actor", "get_transform", fun_gettransform, 0);
    surgescript_vm_bind(vm, "Actor", "get_entity", fun_getentity, 0);
    surgescript_vm_bind(vm, "Actor", "get_offset", fun_getoffset, 0);
    surgescript_vm_bind(vm, "Actor", "set_offset", fun_setoffset, 1);
    surgescript_vm_bind(vm, "Actor", "onRender", fun_onrender, 2);
    surgescript_vm_bind(vm, "Actor", "__canBeClippedOut", fun_canbeclippedout, 0);
    surgescript_vm_bind(vm, "Actor", "get___filepathOfRenderable", fun_getfilepathofrenderable, 0);
    surgescript_vm_bind(vm, "Actor", "get___textureHandle", fun_gettexturehandle, 0);
    surgescript_vm_bind(vm, "Actor", "get___isTranslucent", fun_getistranslucent, 0);

    /* animation methods */
    surgescript_vm_bind(vm, "Actor", "get_animation", fun_getanimation, 0);
    surgescript_vm_bind(vm, "Actor", "set_anim", fun_setanim, 1);
    surgescript_vm_bind(vm, "Actor", "get_anim", fun_getanim, 0);
    surgescript_vm_bind(vm, "Actor", "get_anchor", fun_getanchor, 0);
    surgescript_vm_bind(vm, "Actor", "get_hotSpot", fun_gethotspot, 0);
    surgescript_vm_bind(vm, "Actor", "get_actionSpot", fun_getactionspot, 0);
    surgescript_vm_bind(vm, "Actor", "get_actionOffset", fun_getactionoffset, 0);
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
    surgescript_objecthandle_t transform = scripting_util_require_component(object, "Transform");
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object); 
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    bool is_detached = scripting_util_is_effectively_detached_entity(parent);
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
    surgescript_var_set_null(surgescript_heap_at(heap, OFFSET_ADDR)); /* lazy evaluation */

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
surgescript_var_t* fun_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    double camera_x = surgescript_var_get_number(param[0]);
    double camera_y = surgescript_var_get_number(param[1]);
    v2d_t camera = v2d_new(camera_x, camera_y);
    bool is_detached = surgescript_var_get_bool(surgescript_heap_at(heap, DETACHED_ADDR));

    if(is_detached)
        camera = v2d_multiply(video_get_screen_size(), 0.5f);

    actor->position = scripting_util_world_position(object);
    actor->angle = scripting_util_world_angle(object) * DEG2RAD;
    actor->scale = world_lossyscale(object); /* FIXME: use A5 transformations */

    actor_render(actor, camera);
    return NULL;
}

/* can this renderable be clipped out from the screen, so that no rendering takes place? */
surgescript_var_t* fun_canbeclippedout(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    bool can_be_clipped_out = !actor->visible || actor->alpha == 0.0f; /* clip out invisible actors */

    /* we do a simple, fast bounding-box check */
    /* could we leave this to the scissor test...? */
    if(!can_be_clipped_out && actor->angle == 0.0f && actor->scale.x == 1.0f && actor->scale.y == 1.0f) {
        surgescript_heap_t* heap = surgescript_object_heap(object);
        bool is_detached = surgescript_var_get_bool(surgescript_heap_at(heap, DETACHED_ADDR));

        v2d_t screen_size = video_get_screen_size();
        v2d_t center_of_screen = v2d_multiply(screen_size, 0.5f);
        v2d_t camera = !is_detached ? camera_get_position() : center_of_screen;
        v2d_t camera_topleft = v2d_subtract(camera, center_of_screen);

        v2d_t position_in_world_space = scripting_util_world_position(object);
        v2d_t position_in_screen_space = v2d_subtract(position_in_world_space, camera_topleft);

        const image_t* image = actor_image(actor);
        int width = image_width(image);
        int height = image_height(image);

        v2d_t topleft = v2d_subtract(position_in_screen_space, actor->hot_spot);
        v2d_t bottomright = v2d_add(topleft, v2d_new(width, height));

        can_be_clipped_out = (topleft.x >= screen_size.x) || (bottomright.x <= 0.0f) || (topleft.y >= screen_size.y) || (bottomright.y <= 0.0f);
    }

    /* done! */
    return surgescript_var_set_bool(surgescript_var_create(), can_be_clipped_out);
}

/* the filepath of this renderable (used by the render queue) */
surgescript_var_t* fun_getfilepathofrenderable(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const actor_t* actor = scripting_actor_ptr(object);
    const image_t* image = actor_image(actor);
    const char* filepath = image_filepath(image);

    return surgescript_var_set_string(surgescript_var_create(), filepath);
}

/* the texture handle of this renderable (used by the render queue) */
surgescript_var_t* fun_gettexturehandle(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    const actor_t* actor = scripting_actor_ptr(object);
    const image_t* image = actor_image(actor);
    texturehandle_t tex = image_texture(image);

    return surgescript_var_set_rawbits(surgescript_var_create(), tex);
}

/* is this renderable translucent? (used by the render queue) */
surgescript_var_t* fun_getistranslucent(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    actor_t* actor = scripting_actor_ptr(object);
    bool is_translucent = (actor->alpha < 1.0f); /* doesn't take individual pixels into account */

    if(!is_translucent) {
        const surgescript_object_t* animation = get_animation(object);
        const animation_t* anim = scripting_animation_ptr(animation);

        if(animation_has_keyframes(anim)) /* FIXME should be has_keyframes_with_changed_opacity or similar */
            is_translucent = true;
    }

    return surgescript_var_set_bool(surgescript_var_create(), is_translucent);
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
    actor->alpha = clip01(alpha);
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
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_var_t* offset = surgescript_heap_at(heap, OFFSET_ADDR);
    surgescript_objecthandle_t handle;

    /* lazy evaluation */
    if(surgescript_var_is_null(offset)) {
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        handle = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
        surgescript_var_set_objecthandle(offset, handle);
    }
    else
        handle = surgescript_var_get_objecthandle(offset);

    /* read transform */
    surgescript_transform_t* transform = surgescript_object_transform(object);
    float x, y;
    surgescript_transform_getposition2d(transform, &x, &y);

    /* get offset */
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, handle);
    scripting_vector2_update(v2, x, y);
    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* set offset */
surgescript_var_t* fun_setoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_transform_t* transform = surgescript_object_transform(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    double x = 0.0, y = 0.0;

    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, v2h);
    scripting_vector2_read(v2, &x, &y);
    surgescript_transform_setposition2d(transform, x, y);

    return NULL;
}

/* get animation hotspot */
surgescript_var_t* fun_gethotspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* call animation.get_hotspot */
    surgescript_object_t* animation = get_animation(object);
    surgescript_var_t* anim_hotspot = surgescript_var_create();
    surgescript_object_call_function(animation, "get_hotSpot", NULL, 0, anim_hotspot);
    return anim_hotspot;
}

/* get animation anchor */
surgescript_var_t* fun_getanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* call animation.get_anchor */
    surgescript_object_t* animation = get_animation(object);
    surgescript_var_t* anim_anchor = surgescript_var_create();
    surgescript_object_call_function(animation, "get_anchor", NULL, 0, anim_anchor);
    return anim_anchor;
}

/* get animation action spot */
surgescript_var_t* fun_getactionspot(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* call animation.get_actionSpot */
    surgescript_object_t* animation = get_animation(object);
    surgescript_var_t* action_spot = surgescript_var_create();
    surgescript_object_call_function(animation, "get_actionSpot", NULL, 0, action_spot);
    return action_spot;
}

/* get animation action offset */
surgescript_var_t* fun_getactionoffset(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* call animation.get_actionOffset */
    surgescript_object_t* animation = get_animation(object);
    surgescript_var_t* action_offset = surgescript_var_create();
    surgescript_object_call_function(animation, "get_actionOffset", NULL, 0, action_offset);
    return action_offset;
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
