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
static bool is_entity_inside_roi(surgescript_object_t* entity_manager, surgescript_object_t* entity);

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
    surgescript_vm_bind(vm, "EntityContainer", "bubbleUpEntities", fun_bubbleupentities, 1);
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
    surgescript_vm_bind(vm, "AwakeEntityContainer", "bubbleUpEntities", fun_bubbleupentities, 1);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "render", fun_render, 2);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "selectActiveEntities", fun_awake_selectactiveentities, 2);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "notifyEntities", fun_notifyentities, 1);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "__releaseChildren", fun_releasechildren, 0);
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t null_handle = surgescript_objectmanager_null(manager);
    surgescript_objecthandle_t entity_manager_handle = surgescript_object_find_ascendant(object, "EntityManager");

    /* validate */
    if(entity_manager_handle == null_handle) {
        fatal_error("%s must be a descendant of EntityManager", surgescript_object_name(object));
        return NULL;
    }

    /* set the internal pointer */
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
                if(!entitymanager_is_inside_roi(entity_manager, spawn_point)) {

                    /* move it back to its spawn point */
                    surgescript_transform_t* transform = surgescript_object_transform(entity);
                    surgescript_transform_setposition2d(transform, spawn_point.x, spawn_point.y);

                    /* notify the object */
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

            /* skip entities that can be clipped (not detached) */
            else if(!is_entity_inside_roi(entity_manager, entity))
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

            /* skip entities that can be clipped (not detached) */
            else if(!is_entity_inside_roi(entity_manager, entity))
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
            if(!(
                is_entity_inside_roi(entity_manager, entity) || /* note: it depends on the size of the entity... what about large ones? */
                surgescript_object_has_tag(entity, "detached")
            ))
                continue;

            /* search the sub-tree for renderables */
            int render_flags = must_display_gizmos ? RENDERFLAG_GIZMOS : 0;
            surgescript_object_traverse_tree_ex(entity, &render_flags, render_subtree);
        }

    }

    /* done */
    return NULL;
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
    char buf[32];

    snprintf(buf, sizeof(buf), "[EntityContainer:%x]", handle);
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

/* given a sector of an EntityTree, call sector.bubbleUp(entity) for each entity stored in this container */
surgescript_var_t* fun_bubbleupentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t sector_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* sector = surgescript_objectmanager_get(manager, sector_handle);
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };

    /* validity check */
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    ssassert(sector_handle == parent_handle);
    /*ssassert(0 == strcmp(surgescript_object_name(sector), "EntityTreeLeaf"));*/

    /* for each entity */
    int child_count = surgescript_object_child_count(object);
    for(int i = 0; i < child_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* skip entity? */
        if(surgescript_object_is_killed(entity))
            continue;

        /* call sector.bubbleUp(entity) */
        surgescript_var_set_objecthandle(arg, entity_handle);
        surgescript_object_call_function(sector, "bubbleUp", args, 1, NULL);
    }

    /* done */
    surgescript_var_destroy(arg);
    return NULL;
}

/* release all children */
surgescript_var_t* fun_releasechildren(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    int child_count = surgescript_object_child_count(object);

    /* release children immediately and call their destructors (if any) */
    for(int i = child_count - 1; i >= 0; i--) {
        surgescript_objecthandle_t child_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* child = surgescript_objectmanager_get(manager, child_handle);
        surgescript_object_kill(child);
        surgescript_objectmanager_delete(manager, child_handle); /* release immediately */
    }

    /* done! */
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

        /* clip it out?
           what if it's an entity of large dimensions compared to the screen size? */
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

bool is_entity_inside_roi(surgescript_object_t* entity_manager, surgescript_object_t* entity)
{
    surgescript_transform_t* transform = surgescript_object_transform(entity);

    /* (x,y) is in world space if the entity is a direct child of an entity container */
    float xpos, ypos;
    surgescript_transform_getposition2d(transform, &xpos, &ypos);

    /* ROI test */
    return entitymanager_is_inside_roi(entity_manager, v2d_new(xpos, ypos));
}