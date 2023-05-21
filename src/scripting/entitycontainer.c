/*
 * Open Surge Engine
 * entitycontainer.c - scripting system: Entity Container
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
#include "../core/util.h"
#include "../core/video.h"
#include "../core/sprite.h"
#include "../entities/renderqueue.h"

static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_tostring(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_reparent(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_bubbleupentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_releasechildren(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_selectactiveentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_notifyentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

static surgescript_var_t* fun_awake_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_awake_reparent(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_awake_selectactiveentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

enum { RENDERFLAG_SPECIALS = 0x1, RENDERFLAG_GIZMOS = 0x2 };
static inline surgescript_object_t* get_entity_manager(surgescript_object_t* entity_container);
static bool render_subtree(surgescript_object_t* object, void* data);
static inline void notify_entity(surgescript_object_t* entity, const char* fun_name);
static inline v2d_t entity_position(surgescript_object_t* entity);
static inline bool is_entity_inside_roi(surgescript_object_t* entity_manager, surgescript_object_t* entity);
static inline bool is_entity_inside_screen(surgescript_object_t* entity_manager, surgescript_object_t* entity);
static bool is_entity_position_inside_screen(surgescript_object_t* entity_manager, surgescript_object_t* entity, v2d_t entity_position);
static bool is_sprite_inside_screen(v2d_t camera_position, const char* sprite_name, v2d_t sprite_position, float sprite_rotation, v2d_t sprite_scale);


/*
 * scripting_register_entitycontainer()
 * Register the EntityContainer object
 */
void scripting_register_entitycontainer(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "EntityContainer", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "EntityContainer", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "EntityContainer", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "EntityContainer", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "EntityContainer", "toString", fun_tostring, 0);
    surgescript_vm_bind(vm, "EntityContainer", "reparent", fun_reparent, 1);
    surgescript_vm_bind(vm, "EntityContainer", "bubbleUpEntities", fun_bubbleupentities, 0);
    surgescript_vm_bind(vm, "EntityContainer", "render", fun_render, 2);
    surgescript_vm_bind(vm, "EntityContainer", "selectActiveEntities", fun_selectactiveentities, 2);
    surgescript_vm_bind(vm, "EntityContainer", "notifyEntities", fun_notifyentities, 1);
    surgescript_vm_bind(vm, "EntityContainer", "__releaseChildren", fun_releasechildren, 0);

    /* AwakeEntityContainer "inherits" from EntityContainer */
    surgescript_vm_bind(vm, "AwakeEntityContainer", "state:main", fun_awake_main, 0);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "toString", fun_tostring, 0);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "reparent", fun_awake_reparent, 1);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "bubbleUpEntities", fun_bubbleupentities, 0);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "render", fun_render, 2);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "selectActiveEntities", fun_awake_selectactiveentities, 2);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "notifyEntities", fun_notifyentities, 1);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "__releaseChildren", fun_releasechildren, 0);
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);

    /* validate */
    const char* parent_name = surgescript_object_name(parent);
    if(!(0 == strcmp(parent_name, "EntityTreeLeaf") || 0 == strcmp(parent_name, "EntityManager")))
        fatal_error("%s must not be a child of %s", surgescript_object_name(object), parent_name);

    /* set the internal pointer */
    surgescript_objecthandle_t entity_manager_handle = surgescript_object_find_ascendant(object, "EntityManager");
    surgescript_object_t* entity_manager = surgescript_objectmanager_get(manager, entity_manager_handle);
    surgescript_object_set_userdata(object, entity_manager);

    /* done */
    return NULL;
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_object_t* entity_manager = get_entity_manager(object);
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };

    /* for each entity */
    int child_count = surgescript_object_child_count(object);
    for(int i = 0; i < child_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* skip deleted entities */
        if(surgescript_object_is_killed(entity)) {
            entitymanager_remove_entity_info(entity_manager, entity_handle);
            continue;
        }

        /* is the entity inside the region of interest? */
        if(is_entity_inside_roi(entity_manager, entity)) {

            /* the entity is active */
            surgescript_object_set_active(entity, true);

            /* the entity is not sleeping */
            entitymanager_set_entity_sleeping(entity_manager, entity_handle, false);

            /* does this entity implement lateUpdate() ? */
            if(surgescript_object_has_function(entity, "lateUpdate")) {
                surgescript_var_set_objecthandle(arg, entity_handle);
                surgescript_object_call_function(entity_manager, "addToLateUpdateQueue", args, 1, NULL);
            }

        }
        else if(!surgescript_object_has_tag(entity, "disposable")) {

            /* reset the entity */
            if(
                /*entitymanager_has_entity_info(entity_manager, entity_handle) &&*/ /* <-- no need */
                !entitymanager_is_entity_sleeping(entity_manager, entity_handle) &&
                entitymanager_is_entity_persistent(entity_manager, entity_handle)
            ) {

                v2d_t spawn_point = entitymanager_get_entity_spawn_point(entity_manager, entity_handle);
                /*if(!entitymanager_is_inside_roi(entity_manager, spawn_point)) {*/ /* <-- not good enough; misses a lot */
                if(!is_entity_position_inside_screen(entity_manager, entity, spawn_point)) {

                    /* move it back to its spawn point */
                    surgescript_transform_t* transform = surgescript_object_transform(entity);
                    surgescript_transform_setposition2d(transform, spawn_point.x, spawn_point.y);

                    /* notify the entity */
                    notify_entity(entity, "onReset");

                    /* put it to sleep */
                    entitymanager_set_entity_sleeping(entity_manager, entity_handle, true);

                }

            }

            /* the entity is no longer active */
            surgescript_object_set_active(entity, false);

        }
        else {

            /* remove the disposable entity */
            surgescript_object_kill(entity);
            entitymanager_remove_entity_info(entity_manager, entity_handle);

        }
    }

    /* done */
    surgescript_var_destroy(arg);
    return NULL;
}

/* render the entities in this container */
surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_object_t* entity_manager = get_entity_manager(object);
    int mode = surgescript_var_get_number(param[0]);
    bool must_display_gizmos = surgescript_var_get_bool(param[1]);
    bool is_in_debug_mode = (mode == 1);
    bool is_in_level_editor = (mode == 2);
    int child_count = surgescript_object_child_count(object);

    /* can we clip out an entity? */
    #if 1
    /* it depends on the size of the entity... what about large ones? */
    #define can_clip_entity(entity) (!is_entity_inside_roi(entity_manager, (entity)))
    #else
    /* a more expensive test */
    #define can_clip_entity(entity) (!is_entity_inside_screen(entity_manager, (entity)))
    #endif

    /* render */
    if(is_in_level_editor) {

        /* LEVEL EDITOR */

        /* for each entity */
        for(int i = 0; i < child_count; i++) {
            surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
            surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

            /* skip deleted entities */
            if(surgescript_object_is_killed(entity)) {
                entitymanager_remove_entity_info(entity_manager, entity_handle);
                continue;
            }

            /* skip private entities */
            else if(surgescript_object_has_tag(entity, "private"))
                continue;

            /* skip detached entities */
            else if(surgescript_object_has_tag(entity, "detached"))
                continue;

            /* skip entities that can be clipped */
            else if(can_clip_entity(entity))
                continue;

            /* we're in the editor. Objects tagged "gizmo" SHOULD NOT
               provoke any data or state changes within SurgeScript */

            /* render the entity */
            renderqueue_enqueue_ssobject_debug(entity);
        }

    }
    else if(is_in_debug_mode) {

        /* DEBUG MODE */

        /* for each entity */
        for(int i = 0; i < child_count; i++) {
            surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
            surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

            /* skip deleted entities */
            if(surgescript_object_is_killed(entity)) {
                entitymanager_remove_entity_info(entity_manager, entity_handle);
                continue;
            }

            /* skip inactive entities */
            else if(!surgescript_object_is_active(entity))
                continue;
#if 0
            /* skip sleeping entities */
            else if(entitymanager_is_entity_sleeping(entity_manager, entity_handle))
                continue;
#endif
            /* skip detached entities */
            else if(surgescript_object_has_tag(entity, "detached")) {
                if(0 != strcmp(surgescript_object_name(entity), "Debug Mode"))
                    continue;
            }

            /* skip entities that can be clipped */
            else if(can_clip_entity(entity))
                continue;

            /* search the sub-tree for renderables */
            int render_flags = RENDERFLAG_SPECIALS | (must_display_gizmos ? RENDERFLAG_GIZMOS : 0);
            surgescript_object_traverse_tree_ex(entity, &render_flags, render_subtree);
        }

    }
    else {

        /* REGULAR GAMEPLAY */

        /* for each entity */
        for(int i = 0; i < child_count; i++) {
            surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
            surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

            /* skip deleted entities */
            if(surgescript_object_is_killed(entity)) {
                entitymanager_remove_entity_info(entity_manager, entity_handle);
                continue;
            }

            /* skip inactive entities */
            else if(!surgescript_object_is_active(entity))
                continue;
#if 0
            /* skip sleeping entities */
            else if(entitymanager_is_entity_sleeping(entity_manager, entity_handle))
                continue;
#endif
            /* skip entities that can be clipped */
            if(
                can_clip_entity(entity) &&
                !surgescript_object_has_tag(entity, "detached")
            )
                continue;

            /* search the sub-tree for renderables */
            int render_flags = must_display_gizmos ? RENDERFLAG_GIZMOS : 0;
            surgescript_object_traverse_tree_ex(entity, &render_flags, render_subtree);
        }

    }

    /* done */
    return NULL;

    #undef can_clip_entity
}

/* spawn function */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* destroy function */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* toString function */
surgescript_var_t* fun_tostring(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objecthandle_t handle = surgescript_object_handle(object);
    const char* name = surgescript_object_name(object);
    char buf[32];

    snprintf(buf, sizeof(buf), "[%s:%x]", name, handle);
    return surgescript_var_set_string(surgescript_var_create(), buf);
}

/* make this container a parent of an entity */
surgescript_var_t* fun_reparent(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t this_handle = surgescript_object_handle(object);
    surgescript_objecthandle_t entity_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

    /* we guarantee that only unawake and non-detached entities are children of this container */
    if(surgescript_object_has_tag(entity, "entity") && !(
        surgescript_object_has_tag(entity, "awake") ||
        surgescript_object_has_tag(entity, "detached")
    )) {

        /* reparent */
        surgescript_object_reparent(entity, this_handle, 0);

        /* inactivate (just in case) */
        surgescript_object_t* entity_manager = get_entity_manager(object);
        if(!is_entity_inside_roi(entity_manager, entity)) {
            surgescript_object_set_active(entity, false);
        }

    }

    /* done */
    return NULL;
}

/* call sector.bubbleUp(entity) for each entity stored in this container, where sector = parent */
surgescript_var_t* fun_bubbleupentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t sector_handle = surgescript_object_parent(object);
    surgescript_object_t* sector = surgescript_objectmanager_get(manager, sector_handle);
    int child_count = surgescript_object_child_count(object);
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };

    /* validate */
    ssassert(0 == strcmp(surgescript_object_name(sector), "EntityTreeLeaf"));

    /* allocate temporary array */
    surgescript_objecthandle_t _tmp[256]; /* optimize with stack storage. use alloca() ? */
    const size_t _tmp_maxlen = sizeof(_tmp) / sizeof(_tmp[0]);
    surgescript_objecthandle_t* arr = child_count <= _tmp_maxlen ? _tmp : mallocx(child_count * sizeof(*arr));
    int len = 0;

    /* for each entity */
    for(int i = 0; i < child_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* skip entity? */
        if(surgescript_object_is_killed(entity))
            continue;

        /* copy the entity to a temporary array
           it may be reparented, so we don't bubble it up in this loop */
        arr[len++] = entity_handle;
    }

    /* for each entity that can be reparented */
    for(int i = 0; i < len; i++) {
        surgescript_objecthandle_t entity_handle = arr[i];

        /* call sector.bubbleUp(entity) */
        surgescript_var_set_objecthandle(arg, entity_handle);
        surgescript_object_call_function(sector, "bubbleUp", args, 1, NULL);
    }

    /* deallocate temporary array */
    if(arr != _tmp)
        free(arr);

    /* done */
    surgescript_var_destroy(arg);
    return NULL;
}

/* release all children */
surgescript_var_t* fun_releasechildren(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    int child_count = surgescript_object_child_count(object);
    surgescript_objecthandle_t* children = mallocx(child_count * sizeof(*children));

    /* release children immediately and call their destructors (if any) */
    for(int i = child_count - 1; i >= 0; i--) {
        surgescript_objecthandle_t child_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* child = surgescript_objectmanager_get(manager, child_handle);
        surgescript_object_kill(child);
        children[i] = child_handle; /* store the handle in a temporary array */
    }

    for(int i = child_count - 1; i >= 0; i--) {
        surgescript_objecthandle_t child_handle = children[i];
        surgescript_objectmanager_delete(manager, child_handle); /* release immediately */
    }

    /* done! */
    free(children);
    return NULL;
}

/* select all entities that should be processed and add them to the output array */
surgescript_var_t* fun_selectactiveentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_object_t* entity_manager = get_entity_manager(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t output_array_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* output_array = surgescript_objectmanager_get(manager, output_array_handle);
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };
    bool skip_inactive_entities = surgescript_var_get_bool(param[1]);

    /* for each entity */
    int entity_count = surgescript_object_child_count(object);
    for(int i = 0; i < entity_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* skip entity? */
        if(surgescript_object_is_killed(entity))
            continue;

        /* skip inactive entities */
        else if(skip_inactive_entities && !surgescript_object_is_active(entity))
            continue;

        /* clip it out? */
        else if(!is_entity_inside_roi(entity_manager, entity))
            continue;

        /* add the entity to the output array */
        surgescript_var_set_objecthandle(arg, entity_handle);
        surgescript_object_call_function(output_array, "push", args, 1, NULL);
    }

    /* done */
    surgescript_var_destroy(arg);
    return NULL;
}

/* notify entities: given the name of a function with no arguments, call it in all entities */
surgescript_var_t* fun_notifyentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* note that we don't notify the descendants of these entities that also happen to be entities */
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const char* fun_name = surgescript_var_fast_get_string(param[0]);

    /* for each entity */
    int entity_count = surgescript_object_child_count(object);
    for(int i = 0; i < entity_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* notify entity */
        notify_entity(entity, fun_name);
    }

    /* done */
    return NULL;
}




/*
 * awake variant
 */

/* main state */
surgescript_var_t* fun_awake_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_object_t* entity_manager = get_entity_manager(object);
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };
    int child_count = surgescript_object_child_count(object);

    /* for each entity */
    for(int i = 0; i < child_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* is the entity removed? */
        if(surgescript_object_is_killed(entity)) {
            entitymanager_remove_entity_info(entity_manager, entity_handle);
            continue;
        }

        /* the entity must be active */
        surgescript_object_set_active(entity, true);

#if 0 /* this entity should never sleep... */
        /* the entity is not sleeping */
        entitymanager_set_entity_sleeping(entity_manager, entity_handle, false);
#endif

        /* does this entity implement lateUpdate() ? */
        if(surgescript_object_has_function(entity, "lateUpdate")) {
            surgescript_var_set_objecthandle(arg, entity_handle);
            surgescript_object_call_function(entity_manager, "addToLateUpdateQueue", args, 1, NULL);
        }
    }

    /* done */
    surgescript_var_destroy(arg);
    return NULL;
}

/* make this container a parent of an entity */
surgescript_var_t* fun_awake_reparent(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t this_handle = surgescript_object_handle(object);
    surgescript_objecthandle_t entity_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

    /* we guarantee that only awake or detached entities are children of this container */
    if(surgescript_object_has_tag(entity, "entity") && (
        surgescript_object_has_tag(entity, "awake") ||
        surgescript_object_has_tag(entity, "detached")
    )) {

        /* reparent */
        surgescript_object_reparent(entity, this_handle, 0);

    }

    /* done */
    return NULL;
}

/* select all entities that should be processed and add them to the output array */
surgescript_var_t* fun_awake_selectactiveentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t output_array_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* output_array = surgescript_objectmanager_get(manager, output_array_handle);
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };
    bool skip_inactive_entities = surgescript_var_get_bool(param[1]);

    /* for each entity */
    int entity_count = surgescript_object_child_count(object);
    for(int i = 0; i < entity_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        const surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* skip entity? */
        if(surgescript_object_is_killed(entity))
            continue;
        else if(skip_inactive_entities && !surgescript_object_is_active(entity))
            continue;

        /* add entity to the output array */
        surgescript_var_set_objecthandle(arg, entity_handle);
        surgescript_object_call_function(output_array, "push", args, 1, NULL);
    }

    /* done */
    surgescript_var_destroy(arg);
    return NULL;
}



/*
 * helpers
 */

surgescript_object_t* get_entity_manager(surgescript_object_t* entity_container)
{
    return (surgescript_object_t*)surgescript_object_userdata(entity_container);
}

bool render_subtree(surgescript_object_t* object, void* data)
{
    int flags = *((int*)data);
    bool must_render_gizmos = (0 != (flags & RENDERFLAG_GIZMOS));
    bool must_render_specials = (0 != (flags & RENDERFLAG_SPECIALS));

    /* skip inactive objects */
    if(!surgescript_object_is_active(object) || surgescript_object_is_killed(object))
        return false;

    /* will render objects tagged "renderable" */
    if(surgescript_object_has_tag(object, "renderable"))
        renderqueue_enqueue_ssobject(object);

    /* render special entities that are normally invisible */
    if(must_render_specials) {
        if(surgescript_object_has_tag(object, "special")) {
            if(!surgescript_object_has_tag(object, "private"))
                renderqueue_enqueue_ssobject_debug(object);
        }
    }

    /* will render objects tagged "gizmo" */
    if(must_render_gizmos) {
        if(surgescript_object_has_tag(object, "gizmo"))
            renderqueue_enqueue_ssobject_gizmo(object);
    }

    /* visit the children */
    return true;
}

void notify_entity(surgescript_object_t* entity, const char* fun_name)
{
    if(surgescript_object_has_function(entity, fun_name))
        surgescript_object_call_function(entity, fun_name, NULL, 0, NULL);
}

v2d_t entity_position(surgescript_object_t* entity)
{
    const surgescript_transform_t* transform = surgescript_object_transform(entity);

    /* (x,y) is in world space if the entity is a direct child of an entity container */
    float xpos, ypos;
    surgescript_transform_getposition2d(transform, &xpos, &ypos);

    return v2d_new(xpos, ypos);
}

bool is_entity_inside_roi(surgescript_object_t* entity_manager, surgescript_object_t* entity)
{
    /* ROI test */
    return entitymanager_is_inside_roi(entity_manager, entity_position(entity));
}

bool is_entity_inside_screen(surgescript_object_t* entity_manager, surgescript_object_t* entity)
{
    return is_entity_position_inside_screen(entity_manager, entity, entity_position(entity));
}

bool is_entity_position_inside_screen(surgescript_object_t* entity_manager, surgescript_object_t* entity, v2d_t entity_position)
{
    /* guess the position of the camera */
    int top, left, bottom, right, center_x, center_y;
    entitymanager_get_roi(entity_manager, &top, &left, &bottom, &right);
    center_x = (left + (right + 1)) / 2;
    center_y = (top + (bottom + 1)) / 2;

    v2d_t camera_position = v2d_new(center_x, center_y);

    /* guess the sprite of the entity */
    const char* sprite_name = surgescript_object_name(entity);

    /* get the position, rotation and scale of the entity in world space */
    const surgescript_transform_t* transform = surgescript_object_transform(entity);
    float sx, sy, deg;

    deg = surgescript_transform_getrotation2d(transform); /* local rotation == global rotation */
    surgescript_transform_getscale2d(transform, &sx, &sy);

    v2d_t sprite_position = entity_position;
    v2d_t sprite_scale = v2d_new(sx, sy);
    float sprite_rotation = deg * DEG2RAD;

    /* this is only an approximation, because we don't know the actual size of
       the entity (e.g., what kind of graphics does it display? are there any
       other entities attached to it? and so on...)

       Nonetheless, this works well enough in practice and is fast to compute
       (comparatively speaking). */
    return is_sprite_inside_screen(camera_position, sprite_name, sprite_position, sprite_rotation, sprite_scale);
}

bool is_sprite_inside_screen(v2d_t camera_position, const char* sprite_name, v2d_t sprite_position, float sprite_rotation, v2d_t sprite_scale)
{
    const animation_t* anim = sprite_animation_exists(sprite_name, 0) ? sprite_get_animation(sprite_name, 0) : sprite_get_animation(NULL, 0);
    const image_t* img = sprite_get_image(anim, 0);
    v2d_t hot_spot = anim->hot_spot;
    v2d_t sprite_size = v2d_new(image_width(img), image_height(img));
    v2d_t screen_size = video_get_screen_size();

    /* rectangle of the screen in world coordinates (inclusive) */
    float screen_top = camera_position.y - screen_size.y * 0.5f;
    float screen_left = camera_position.x - screen_size.x * 0.5f;
    float screen_bottom = screen_top + (screen_size.y - 1.0f);
    float screen_right = screen_left + (screen_size.x - 1.0f);

    /* rectangle of the scaled sprite in world coordinates (inclusive) */
    float scaled_sprite_top = sprite_position.y - hot_spot.y * sprite_scale.y;
    float scaled_sprite_left = sprite_position.x - hot_spot.x * sprite_scale.x;
    float scaled_sprite_bottom = scaled_sprite_top + (sprite_size.y - 1.0f) * sprite_scale.y;
    float scaled_sprite_right = scaled_sprite_left + (sprite_size.x - 1.0f) * sprite_scale.x;

    /* rectangle of the sprite: assume that there is no rotation */
    float sprite_top = scaled_sprite_top;
    float sprite_left = scaled_sprite_left;
    float sprite_bottom = scaled_sprite_bottom;
    float sprite_right = scaled_sprite_right;

    /* it turns out that there is rotation
       pick the bounding box of the rotated sprite */
    if(sprite_rotation != 0.0f) {

        v2d_t scaled_sprite_topleft = v2d_new(scaled_sprite_left, scaled_sprite_top);
        v2d_t scaled_sprite_topright = v2d_new(scaled_sprite_right, scaled_sprite_top);
        v2d_t scaled_sprite_bottomleft = v2d_new(scaled_sprite_left, scaled_sprite_bottom);
        v2d_t scaled_sprite_bottomright = v2d_new(scaled_sprite_right, scaled_sprite_bottom);

        float radians = -sprite_rotation; /* the y-axis grows downwards. sin(-x) = -sin(x) and cos(-x) = cos(x). */
        v2d_t rotated_sprite_corner[4] = {
            scaled_sprite_topleft,
            scaled_sprite_topright,
            scaled_sprite_bottomleft,
            scaled_sprite_bottomright
        };
        v2d_rotate_all(rotated_sprite_corner, 4, radians);

        v2d_t min01 = v2d_new(
            min(rotated_sprite_corner[0].x, rotated_sprite_corner[1].x),
            min(rotated_sprite_corner[0].y, rotated_sprite_corner[1].y)
        ), min23 = v2d_new(
            min(rotated_sprite_corner[2].x, rotated_sprite_corner[3].x),
            min(rotated_sprite_corner[2].y, rotated_sprite_corner[3].y)
        ), max01 = v2d_new(
            min(rotated_sprite_corner[0].x, rotated_sprite_corner[1].x),
            min(rotated_sprite_corner[0].y, rotated_sprite_corner[1].y)
        ), max23 = v2d_new(
            max(rotated_sprite_corner[2].x, rotated_sprite_corner[3].x),
            max(rotated_sprite_corner[2].y, rotated_sprite_corner[3].y)
        );

        sprite_top = min(min01.y, min23.y);
        sprite_left = min(min01.x, min23.x);
        sprite_bottom = max(max01.y, max23.y);
        sprite_right = max(max01.x, max23.x);

    }

    /* bounding box test */
    return !(
        sprite_right < screen_left || sprite_left > screen_right ||
        sprite_bottom < screen_top || sprite_top > screen_bottom
    );
}