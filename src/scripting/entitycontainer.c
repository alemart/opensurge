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
#include "../scenes/level.h"

static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_reparent(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_releasechildren(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setroi(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_selectactiveentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

static surgescript_var_t* fun_awake_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_awake_reparent(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_awake_selectactiveentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

static bool render_subtree(surgescript_object_t* object, void* data);

#define get_entity_manager(object) ((surgescript_object_t*)surgescript_object_userdata(object))
extern bool entitymanager_has_entity_info(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
extern void entitymanager_remove_entity_info(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
extern v2d_t entitymanager_get_entity_spawn_point(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
extern bool entitymanager_is_entity_persistent(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
extern bool entitymanager_is_entity_sleeping(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
extern void entitymanager_set_entity_sleeping(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle, bool is_sleeping);
extern bool entitymanager_is_entity_inside_roi(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
extern void entitymanager_set_entity_inside_roi(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle, bool is_inside_roi);
extern bool entitymanager_is_inside_roi(surgescript_object_t* entity_manager, v2d_t position);

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
    surgescript_vm_bind(vm, "EntityContainer", "reparent", fun_reparent, 1);
    surgescript_vm_bind(vm, "EntityContainer", "render", fun_render, 0);
    surgescript_vm_bind(vm, "EntityContainer", "selectActiveEntities", fun_selectactiveentities, 1);
    surgescript_vm_bind(vm, "EntityContainer", "setROI", fun_setroi, 4);
    surgescript_vm_bind(vm, "EntityContainer", "__releaseChildren", fun_releasechildren, 0);

    /* AwakeEntityContainer "inherits" from EntityContainer */
    surgescript_vm_bind(vm, "AwakeEntityContainer", "state:main", fun_awake_main, 0);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "reparent", fun_awake_reparent, 1);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "render", fun_render, 0);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "selectActiveEntities", fun_awake_selectactiveentities, 1);
    surgescript_vm_bind(vm, "AwakeEntityContainer", "setROI", fun_setroi, 4);
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
#if 0
        float xpos, ypos;
        surgescript_transform_t* transform = surgescript_object_transform(entity);
        surgescript_transform_getposition2d(transform, &xpos, &ypos);
        if(entitymanager_is_inside_roi(entity_manager, v2d_new(xpos, ypos))) {
#else
        if(entitymanager_is_entity_inside_roi(entity_manager, entity_handle)) {
#endif
            /* the entity is active */
            surgescript_object_set_active(entity, true);

            /* the entity is not sleeping */
            entitymanager_set_entity_sleeping(entity_manager, entity_handle, false);

            /* does this entity implement lateUpdate() ? */
            if(surgescript_object_has_function(entity, "lateUpdate")) {
                surgescript_var_set_objecthandle(arg, entity_handle);
                surgescript_object_call_function(entity_manager, "addToLateUpdateQueue", args, 1, NULL);
            }

            /* TODO brick-like */
            ;

        }
        else if(!surgescript_object_has_tag(entity, "disposable")) {

            /* the entity is no longer active */
            surgescript_object_set_active(entity, false);

            /* put it to sleep */
            entitymanager_set_entity_sleeping(entity_manager, entity_handle, true);

            /* reset the entity */
            if(
                entitymanager_has_entity_info(entity_manager, entity_handle) &&
                entitymanager_is_entity_persistent(entity_manager, entity_handle) &&
                !entitymanager_is_entity_sleeping(entity_manager, entity_handle)
            ) {

                v2d_t spawn_point = entitymanager_get_entity_spawn_point(entity_manager, entity_handle);
                if(!entitymanager_is_inside_roi(entity_manager, spawn_point)) {

                    /* move it back to its spawn point */
                    surgescript_transform_t* transform = surgescript_object_transform(entity);
                    surgescript_transform_setposition2d(transform, spawn_point.x, spawn_point.y);
                    /* update roi flag? */

                    /* notify the object */
                    if(surgescript_object_has_function(entity, "onReset"))
                        surgescript_object_call_function(entity, "onReset", NULL, 0, NULL);

                }

            }

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
    surgescript_var_t* arg = surgescript_var_create();
    const surgescript_var_t* args[] = { arg };
    bool must_display_gizmos = false;
    int child_count = surgescript_object_child_count(object);

    /* for each entity */
    for(int i = 0; i < child_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* cut processing time */
        if(!surgescript_object_is_active(entity))
            continue;

        /* skip deleted entities */
        else if(surgescript_object_is_killed(entity))
            continue;

        /* clip it out */
        else if(!(
            entitymanager_is_entity_inside_roi(entity_manager, entity_handle) ||
            surgescript_object_has_tag(entity, "detached")
        ))
            continue;

        /* render */
        if(level_editmode()) {

            /* LEVEL EDITOR */

            /* skip private entities */
            if(surgescript_object_has_tag(entity, "private") || surgescript_object_has_tag(entity, "detached"))
                continue;

            /* we're in the editor. Objects tagged "gizmo" SHOULD NOT
               provoke any data or state changes within SurgeScript */

            /* render the entity */
            renderqueue_enqueue_ssobject_debug(entity);

        }
        else if(level_is_in_debug_mode()) {

            /* DEBUG MODE */

        }
        else {

            /* REGULAR GAMEPLAY */

            /* skip inactive / sleeping entities */
            if(!surgescript_object_is_active(entity) || entitymanager_is_entity_sleeping(entity_manager, entity_handle))
                continue;

            /* search the sub-tree for renderables */
            surgescript_object_traverse_tree_ex(entity, &must_display_gizmos, render_subtree);

        }
    }

    /* done */
    surgescript_var_destroy(arg);
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

/* make this container a parent of an entity */
surgescript_var_t* fun_reparent(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t this_handle = surgescript_object_handle(object);
    surgescript_objecthandle_t entity_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

    /* we guarantee that only unawake and attached entities are children of this container */
    if(surgescript_object_has_tag(entity, "entity") && !(
        surgescript_object_has_tag(entity, "awake") ||
        surgescript_object_has_tag(entity, "detached")
    )) {

        /* reparent */
        surgescript_object_reparent(entity, this_handle, 0);

    }

    /* done */
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

/* set the (left, top, right, bottom) region of interest */
surgescript_var_t* fun_setroi(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_object_t* entity_manager = get_entity_manager(object);
    double left = surgescript_var_get_number(param[0]);
    double top = surgescript_var_get_number(param[1]);
    double right = surgescript_var_get_number(param[2]);
    double bottom = surgescript_var_get_number(param[3]);

    /* for each entity */
    int entity_count = surgescript_object_child_count(object);
    for(int i = 0; i < entity_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* is the entity outside the region of interest? */
        surgescript_transform_t* transform = surgescript_object_transform(entity);
        float x, y;
        surgescript_transform_getposition2d(transform, &x, &y);
        bool is_outside_roi = (x < left || x > right || y < top || y > bottom);

        /* update the ROI flag */
        entitymanager_set_entity_inside_roi(entity_manager, entity_handle, !is_outside_roi);
    }

    /* done */
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

    /* for each entity */
    int entity_count = surgescript_object_child_count(object);
    for(int i = 0; i < entity_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* skip entity? */
        if(surgescript_object_is_killed(entity))
            continue;

        /* clip it out?
           what if it's an entity of large dimensions compared to the screen size? */
        else if(!entitymanager_is_entity_inside_roi(entity_manager, entity_handle))
            continue;

        /* add the entity to the output array */
        surgescript_var_set_objecthandle(arg, entity_handle);
        surgescript_object_call_function(output_array, "push", args, 1, NULL);
    }

    /* done */
    surgescript_var_destroy(arg);
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

        /* the entity is not sleeping */
        entitymanager_set_entity_sleeping(entity_manager, entity_handle, false);

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

    /* for each entity */
    int entity_count = surgescript_object_child_count(object);
    for(int i = 0; i < entity_count; i++) {
        surgescript_objecthandle_t entity_handle = surgescript_object_nth_child(object, i);
        const surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);

        /* skip entity? */
        if(surgescript_object_is_killed(entity))
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

bool render_subtree(surgescript_object_t* object, void* data)
{
    bool must_display_gizmos = *((bool*)data);

    /* skip inactive objects */
    if(!surgescript_object_is_active(object) || surgescript_object_is_killed(object))
        return false;

    /* will render objects tagged "renderable" */
    if(surgescript_object_has_tag(object, "renderable"))
        renderqueue_enqueue_ssobject(object);

    /* will render objects tagged "gizmo" */
    if(must_display_gizmos && surgescript_object_has_tag(object, "gizmo"))
        renderqueue_enqueue_ssobject_gizmo(object);

    /* visit the children */
    return true;
}