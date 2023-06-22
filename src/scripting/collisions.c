/*
 * Open Surge Engine
 * collisions.c - scripting system: collision system
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
#include <stdint.h>
#include "scripting.h"
#include "../core/image.h"
#include "../core/video.h"
#include "../util/darray.h"
#include "../util/v2d.h"

/* private */
typedef enum { COLLIDER_TYPE_BOX, COLLIDER_TYPE_BALL } collidertype_t;

typedef struct collider_t collider_t;
struct collider_t
{
    collidertype_t type;
    surgescript_objecthandle_t entity;
    surgescript_objecthandle_t colmgr;
    DARRAY(surgescript_objecthandle_t, prev_collisions);
    DARRAY(surgescript_objecthandle_t, curr_collisions);
    v2d_t worldpos;
    v2d_t anchor;
    uint8_t flags;
};

typedef struct boxcollider_t boxcollider_t;
struct boxcollider_t
{
    collider_t collider;
    double width;
    double height;
};

typedef struct ballcollider_t ballcollider_t;
struct ballcollider_t
{
    collider_t collider;
    double radius; /* in pixels */
};

typedef struct collisionmanager_t collisionmanager_t;
struct collisionmanager_t
{
    DARRAY(surgescript_objecthandle_t, colliders);
};

#define COLLIDER_FLAG_ISVISIBLE             0x1
#define COLLIDER_FLAG_NOTIFYONCOLLISION     0x2
#define COLLIDER_FLAG_NOTIFYONOVERLAP       0x4
#define COLLIDER_FLAG_ISDISABLED            0x8
#define COLLIDER_COLOR(flags)               (color_rgba(255, 255, 0, (flags) & COLLIDER_FLAG_ISDISABLED ? 127 : 255))
static const surgescript_heapptr_t CENTER_ADDR = 0;
static const surgescript_heapptr_t ANCHOR_ADDR = 1;
#define unsafe_get_collider(object) ((collider_t*)surgescript_object_userdata(object))
static inline collider_t* safe_get_collider(surgescript_object_t* object);
static inline bool is_collider(const surgescript_object_t* object);
static inline bool quick_bounding_box_test(const collider_t* a, const collider_t* b);
static inline void quickly_get_bounding_box(const collider_t* collider, double* left, double* top, double* right, double* bottom);

static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getcenter(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_notify(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

static surgescript_var_t* fun_collisionbox_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_collideswith(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_contains(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_setanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_getleft(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_getright(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_gettop(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_getbottom(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_setwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_setheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_onrendergizmos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

static surgescript_var_t* fun_collisionball_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_collideswith(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_contains(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_setanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_setradius(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_getradius(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_onrendergizmos(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

static surgescript_var_t* fun_manager_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_manager_notify(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/*
 * scripting_register_collisions()
 * Register built-in functions for the collision system
 */
void scripting_register_collisions(surgescript_vm_t* vm)
{
    /* tags */
    surgescript_tagsystem_t* tag_system = surgescript_vm_tagsystem(vm);
    surgescript_tagsystem_add_tag(tag_system, "CollisionBox", "collider");
    surgescript_tagsystem_add_tag(tag_system, "CollisionBox", "renderable");
    surgescript_tagsystem_add_tag(tag_system, "CollisionBox", "gizmo");
    surgescript_tagsystem_add_tag(tag_system, "CollisionBall", "collider");
    surgescript_tagsystem_add_tag(tag_system, "CollisionBall", "renderable");
    surgescript_tagsystem_add_tag(tag_system, "CollisionBall", "gizmo");

    /* methods */
    surgescript_vm_bind(vm, "CollisionBox", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "CollisionBox", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_entity", fun_getentity, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_visible", fun_getvisible, 0);
    surgescript_vm_bind(vm, "CollisionBox", "set_visible", fun_setvisible, 1);
    surgescript_vm_bind(vm, "CollisionBox", "get_enabled", fun_getenabled, 0);
    surgescript_vm_bind(vm, "CollisionBox", "set_enabled", fun_setenabled, 1);
    surgescript_vm_bind(vm, "CollisionBox", "get_center", fun_getcenter, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_anchor", fun_getanchor, 0);
    surgescript_vm_bind(vm, "CollisionBox", "set_anchor", fun_setanchor, 1);
    surgescript_vm_bind(vm, "CollisionBox", "__notify", fun_notify, 1);
    surgescript_vm_bind(vm, "CollisionBox", "__init", fun_collisionbox_init, 3);
    surgescript_vm_bind(vm, "CollisionBox", "constructor", fun_collisionbox_constructor, 0);
    surgescript_vm_bind(vm, "CollisionBox", "collidesWith", fun_collisionbox_collideswith, 1);
    surgescript_vm_bind(vm, "CollisionBox", "contains", fun_collisionbox_contains, 1);
    surgescript_vm_bind(vm, "CollisionBox", "setAnchor", fun_collisionbox_setanchor, 2);
    surgescript_vm_bind(vm, "CollisionBox", "get_left", fun_collisionbox_getleft, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_right", fun_collisionbox_getright, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_top", fun_collisionbox_gettop, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_bottom", fun_collisionbox_getbottom, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_width", fun_collisionbox_getwidth, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_height", fun_collisionbox_getheight, 0);
    surgescript_vm_bind(vm, "CollisionBox", "set_width", fun_collisionbox_setwidth, 1);
    surgescript_vm_bind(vm, "CollisionBox", "set_height", fun_collisionbox_setheight, 1);
    surgescript_vm_bind(vm, "CollisionBox", "zindex", fun_collisionbox_zindex, 0);
    surgescript_vm_bind(vm, "CollisionBox", "onRender", fun_collisionbox_onrender, 0);
    surgescript_vm_bind(vm, "CollisionBox", "onRenderGizmos", fun_collisionbox_onrendergizmos, 0);

    surgescript_vm_bind(vm, "CollisionBall", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "CollisionBall", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "CollisionBall", "get_entity", fun_getentity, 0);
    surgescript_vm_bind(vm, "CollisionBall", "get_visible", fun_getvisible, 0);
    surgescript_vm_bind(vm, "CollisionBall", "set_visible", fun_setvisible, 1);
    surgescript_vm_bind(vm, "CollisionBall", "get_enabled", fun_getenabled, 0);
    surgescript_vm_bind(vm, "CollisionBall", "set_enabled", fun_setenabled, 1);
    surgescript_vm_bind(vm, "CollisionBall", "get_center", fun_getcenter, 0);
    surgescript_vm_bind(vm, "CollisionBall", "get_anchor", fun_getanchor, 0);
    surgescript_vm_bind(vm, "CollisionBall", "set_anchor", fun_setanchor, 1);
    surgescript_vm_bind(vm, "CollisionBall", "__notify", fun_notify, 1);
    surgescript_vm_bind(vm, "CollisionBall", "__init", fun_collisionball_init, 2);
    surgescript_vm_bind(vm, "CollisionBall", "constructor", fun_collisionball_constructor, 0);
    surgescript_vm_bind(vm, "CollisionBall", "collidesWith", fun_collisionball_collideswith, 1);
    surgescript_vm_bind(vm, "CollisionBall", "contains", fun_collisionball_contains, 1);
    surgescript_vm_bind(vm, "CollisionBall", "setAnchor", fun_collisionball_setanchor, 2);
    surgescript_vm_bind(vm, "CollisionBall", "get_radius", fun_collisionball_getradius, 0);
    surgescript_vm_bind(vm, "CollisionBall", "set_radius", fun_collisionball_setradius, 1);
    surgescript_vm_bind(vm, "CollisionBall", "zindex", fun_collisionball_zindex, 0);
    surgescript_vm_bind(vm, "CollisionBall", "onRender", fun_collisionball_onrender, 0);
    surgescript_vm_bind(vm, "CollisionBall", "onRenderGizmos", fun_collisionball_onrendergizmos, 0);

    surgescript_vm_bind(vm, "CollisionManager", "state:main", fun_manager_main, 0);
    surgescript_vm_bind(vm, "CollisionManager", "constructor", fun_manager_constructor, 0);
    surgescript_vm_bind(vm, "CollisionManager", "destructor", fun_manager_destructor, 0);
    surgescript_vm_bind(vm, "CollisionManager", "destroy", fun_manager_destroy, 0);
    surgescript_vm_bind(vm, "CollisionManager", "__notify", fun_manager_notify, 1);
}

/* checks if an object is a collider */
bool is_collider(const surgescript_object_t* object)
{
    /*return surgescript_object_has_tag(object, "collider"); // unreliable */
    const char* name = surgescript_object_name(object);
    return (0 == strcmp(name, "CollisionBox") || 0 == strcmp(name, "CollisionBall"));
}

/* Returns the collider structure if the given object is a collider,
   or a crash if it isn't */
collider_t* safe_get_collider(surgescript_object_t* object)
{
    if(!is_collider(object)) {
        const char* name = surgescript_object_name(object);
        scripting_error(object, "\"%s\" isn't a collider", name);
        return NULL;
    }

    return unsafe_get_collider(object);
}

/* Get the bounding box of a collider in world space coordinates */
void quickly_get_bounding_box(const collider_t* collider, double* left, double* top, double* right, double* bottom)
{
    double center_x = collider->worldpos.x;
    double center_y = collider->worldpos.y;

    switch(collider->type) {
        case COLLIDER_TYPE_BOX: {
            const boxcollider_t* box = (const boxcollider_t*)collider;
            double half_width = box->width * 0.5;
            double half_height = box->height * 0.5;

            *left = center_x - half_width;
            *top = center_y - half_height;
            *right = center_x + half_width;
            *bottom = center_y + half_height;

            break;
        }

        case COLLIDER_TYPE_BALL: {
            const ballcollider_t* ball = (const ballcollider_t*)collider;

            *left = center_x - ball->radius;
            *top = center_y - ball->radius;
            *right = center_x + ball->radius;
            *bottom = center_y + ball->radius;

            break;
        }
    }
}

/* Quick bounding box test between two colliders */
bool quick_bounding_box_test(const collider_t* a, const collider_t* b)
{
    double al, at, ar, ab, bl, bt, br, bb;

    quickly_get_bounding_box(a, &al, &at, &ar, &ab);
    quickly_get_bounding_box(b, &bl, &bt, &br, &bb);

    return !(
        ar < bl || al >= br || ab < bt || at >= bb
    );
}



/* ----------------------- CollisionManager --------------------------------- */

/* detect collisions between colliders */
surgescript_var_t* fun_manager_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    collisionmanager_t* colmgr = surgescript_object_userdata(object);
    surgescript_var_t* tmp = surgescript_var_create();
    surgescript_var_t* ret = surgescript_var_create();
    const surgescript_var_t* p[] = { tmp };

    /*
     * TODO: make this faster?
     * simple, quadratic algorithm
     */
    for(int i = 1; i < darray_length(colmgr->colliders); i++) {
        surgescript_object_t* collider = surgescript_objectmanager_get(manager, colmgr->colliders[i]);
        for(int j = 0; j < i; j++) {
            surgescript_object_t* other_collider = surgescript_objectmanager_get(manager, colmgr->colliders[j]);

            /* quickly discard a collision test */
            if(!quick_bounding_box_test(
                unsafe_get_collider(collider),
                unsafe_get_collider(other_collider)
            ))
                continue;

            /* perform a collision test */
            surgescript_var_set_objecthandle(tmp, colmgr->colliders[j]);
            surgescript_object_call_function(collider, "collidesWith", p, 1, ret);
            if(surgescript_var_get_bool(ret)) {
                /* notify the colliders */
                surgescript_object_call_function(collider, "__notify", p, 1, NULL);
                surgescript_var_set_objecthandle(tmp, colmgr->colliders[i]);
                surgescript_object_call_function(other_collider, "__notify", p, 1, NULL);
            }
        }
    }

    darray_clear(colmgr->colliders);
    surgescript_var_destroy(ret);
    surgescript_var_destroy(tmp);
    return NULL;
}

/* constructor */
surgescript_var_t* fun_manager_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collisionmanager_t* colmgr = mallocx(sizeof *colmgr);
    darray_init(colmgr->colliders);
    surgescript_object_set_userdata(object, colmgr);
    return NULL;
}

/* destructor */
surgescript_var_t* fun_manager_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collisionmanager_t* colmgr = surgescript_object_userdata(object);
    darray_release(colmgr->colliders);
    free(colmgr);
    return NULL;
}

/* destroy */
surgescript_var_t* fun_manager_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* do nothing */
    return NULL;
}

/* notify: I'm told that a collider is available at this moment (game step) */
surgescript_var_t* fun_manager_notify(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t collider_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* collider = surgescript_objectmanager_get(manager, collider_handle);
    collisionmanager_t* colmgr = surgescript_object_userdata(object);

    /* validate the input */
    if(is_collider(collider))
        darray_push(colmgr->colliders, collider_handle);

    return NULL;
}



/* ------------------------- Collider routines ---------------------------- */

/* collider destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    darray_release(collider->curr_collisions);
    darray_release(collider->prev_collisions);
    free(collider);
    return NULL;
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* update world position (regardless if the collider is enabled or not) */
    collider_t* collider = unsafe_get_collider(object);
    collider->worldpos = scripting_util_world_position(object); /* cached */

    /* if the collider is active, notify the collision manager */
    if(!(collider->flags & COLLIDER_FLAG_ISDISABLED)) {
        surgescript_objectmanager_t* manager = surgescript_object_manager(object);
        surgescript_var_t* tmp = surgescript_var_create();
        const surgescript_var_t* p[] = { tmp };
        int i;

        /* update collisions */
        darray_clear(collider->prev_collisions);
        for(i = 0; i < darray_length(collider->curr_collisions); i++)
            darray_push(collider->prev_collisions, collider->curr_collisions[i]);
        darray_clear(collider->curr_collisions);

        /* notify the collision manager: I am active! */
        surgescript_var_set_objecthandle(tmp, surgescript_object_handle(object));
        surgescript_object_call_function(
            surgescript_objectmanager_get(manager, collider->colmgr),
            "__notify",
            p, 1, NULL
        );

        /* release resources */
        surgescript_var_destroy(tmp);
    }
    else {
        /* the collider is disabled */
        darray_clear(collider->prev_collisions);
        darray_clear(collider->curr_collisions);
    }

    /* done */
    return NULL;
}

/* get the entity associated with the collider */
surgescript_var_t* fun_getentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    return surgescript_var_set_objecthandle(surgescript_var_create(), collider->entity);
}

/* is the collider visible? */
surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    return surgescript_var_set_bool(surgescript_var_create(), (collider->flags & COLLIDER_FLAG_ISVISIBLE) != 0);
}

/* change collider visibility */
surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    bool visible = surgescript_var_get_bool(param[0]);

    if(!visible)
        collider->flags &= ~COLLIDER_FLAG_ISVISIBLE;
    else
        collider->flags |= COLLIDER_FLAG_ISVISIBLE;

    return NULL;
}

/* is the collider enabled? */
surgescript_var_t* fun_getenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    return surgescript_var_set_bool(surgescript_var_create(), (collider->flags & COLLIDER_FLAG_ISDISABLED) == 0);
}

/* enable/disable collider */
surgescript_var_t* fun_setenabled(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    bool enabled = surgescript_var_get_bool(param[0]);

    if(enabled)
        collider->flags &= ~COLLIDER_FLAG_ISDISABLED;
    else
        collider->flags |= COLLIDER_FLAG_ISDISABLED;

    return NULL;
}

/* the collision manager is telling us about a collision with some other collider */
surgescript_var_t* fun_notify(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    surgescript_objecthandle_t other_collider = surgescript_var_get_objecthandle(param[0]);

    darray_push(collider->curr_collisions, other_collider);
    if(collider->flags & (COLLIDER_FLAG_NOTIFYONCOLLISION | COLLIDER_FLAG_NOTIFYONOVERLAP)) {
        surgescript_objectmanager_t* manager = surgescript_object_manager(object);
        surgescript_var_t* tmp = surgescript_var_create();
        const surgescript_var_t* p[] = { tmp };

        /* call entity.onCollision() */
        if(collider->flags & COLLIDER_FLAG_NOTIFYONCOLLISION) {
            bool skip = false;

            /* skip if other_collider is in prev_collisions */
            for(int i = 0; i < darray_length(collider->prev_collisions) && !skip; i++) {
                if(collider->prev_collisions[i] == other_collider)
                    skip = true;
            }

            /* call entity.onCollision() */
            if(!skip) {
                surgescript_object_t* entity = surgescript_objectmanager_get(manager, collider->entity);
                surgescript_var_set_objecthandle(tmp, other_collider);
                surgescript_object_call_function(entity, "onCollision", p, 1, NULL);
            }
        }

        /* call entity.onOverlap() */
        if(collider->flags & COLLIDER_FLAG_NOTIFYONOVERLAP) {
            surgescript_object_t* entity = surgescript_objectmanager_get(manager, collider->entity);
            surgescript_var_set_objecthandle(tmp, other_collider);
            surgescript_object_call_function(entity, "onOverlap", p, 1, NULL);
        }

        /* done */
        surgescript_var_destroy(tmp);
    }

    /* done */
    return NULL;
}

/* get center: Vector2 (world coordinates) */
surgescript_var_t* fun_getcenter(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_var_t* center = surgescript_heap_at(heap, CENTER_ADDR);
    surgescript_objecthandle_t handle;
    surgescript_object_t* v2;

    /* lazy evaluation */
    if(surgescript_var_is_null(center)) {
        surgescript_objecthandle_t me = surgescript_object_handle(object);
        handle = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
        surgescript_var_set_objecthandle(center, handle);
    }
    else
        handle = surgescript_var_get_objecthandle(center);

    /* get center */
    v2 = surgescript_objectmanager_get(manager, handle);
    scripting_vector2_update(v2, collider->worldpos.x, collider->worldpos.y);
    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* get anchor: Vector2 */
surgescript_var_t* fun_getanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* anchor = (0.5, 0.5) is the default (i.e., the anchor is at the center of the collider) */
    collider_t* collider = unsafe_get_collider(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_var_t* anchor = surgescript_heap_at(heap, ANCHOR_ADDR);
    surgescript_objecthandle_t handle;
    surgescript_object_t* v2;

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
    scripting_vector2_update(v2, collider->anchor.x, collider->anchor.y);
    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* set anchor (Vector2) */
surgescript_var_t* fun_setanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t v2h = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, v2h);
    surgescript_var_t* x = surgescript_var_create();
    surgescript_var_t* y = surgescript_var_create();
    const surgescript_var_t* p[2] = { x, y };
    double v2x = 0.0, v2y = 0.0;

    /* read the Vector2 parameter */
    scripting_vector2_read(v2, &v2x, &v2y);

    /* call subclass.setAnchor(x, y) */
    surgescript_var_set_number(x, v2x);
    surgescript_var_set_number(y, v2y);
    surgescript_object_call_function(object, "setAnchor", p, 2, NULL);

    /* done! */
    surgescript_var_destroy(y);
    surgescript_var_destroy(x);
    return NULL;
}



/* ----------------------------- CollisionBox routines ------------------------------- */

/* box constructor */
surgescript_var_t* fun_collisionbox_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = mallocx(sizeof(boxcollider_t));
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t root = surgescript_objectmanager_root(manager);
    surgescript_objecthandle_t parent = surgescript_object_parent(object);
    surgescript_object_t* entity = NULL;

    /* collider initialization */
    collider->type = COLLIDER_TYPE_BOX;
    collider->entity = surgescript_objectmanager_null(manager);
    collider->colmgr = surgescript_objectmanager_null(manager);
    collider->worldpos = v2d_new(0.0f, 0.0f); /* the center of the collider in world coordinates */
    collider->anchor = v2d_new(0.5f, 0.5f); /* default anchor: at the center of the collider */
    collider->flags = 0;
    darray_init(collider->prev_collisions);
    darray_init(collider->curr_collisions);
    ((boxcollider_t*)collider)->width = 0.0;
    ((boxcollider_t*)collider)->height = 0.0;
    surgescript_object_set_userdata(object, collider);

    /* center of the collider (lazy evaluation) */
    ssassert(CENTER_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, CENTER_ADDR));

    /* anchor (lazy evaluation) */
    ssassert(ANCHOR_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, ANCHOR_ADDR));

    /* get entity */
    while(!surgescript_object_has_tag(surgescript_objectmanager_get(manager, parent), "entity")) {
        parent = surgescript_object_parent(surgescript_objectmanager_get(manager, parent));
        if(parent == root) {
            scripting_error(object, 
                "Collider \"%s\" must be a descendant of an entity (parent is \"%s\")",
                surgescript_object_name(object),
                surgescript_object_name(surgescript_objectmanager_get(manager, surgescript_object_parent(object)))
            );
            break;
        }
    }
    collider->entity = parent;
    entity = surgescript_objectmanager_get(manager, collider->entity);

    /* validation */
    if(surgescript_object_has_tag(entity, "detached")) {
        scripting_error(object, 
            "\"%s\" won't work with detached entities like \"%s\"",
            surgescript_object_name(object),
            surgescript_object_name(entity)
        );
    }

    /* collision flags */
    if(surgescript_object_has_function(entity, "onCollision"))
        collider->flags |= COLLIDER_FLAG_NOTIFYONCOLLISION;
    if(surgescript_object_has_function(entity, "onOverlap"))
        collider->flags |= COLLIDER_FLAG_NOTIFYONOVERLAP;

    /* done */
    return NULL;
}

/* init variables */
surgescript_var_t* fun_collisionbox_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    collider->colmgr = surgescript_var_get_objecthandle(param[0]); /* collision manager */
    boxcollider->width = max(1.0, surgescript_var_get_number(param[1])); /* collider width */
    boxcollider->height = max(1.0, surgescript_var_get_number(param[2])); /* collider height */
    return NULL;
}

/* setAnchor() */
surgescript_var_t* fun_collisionbox_setanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /*
    * sets the anchor of the collider to a certain position (x,y),
    * where 0 <= x, y <= 1. Defaults to (0.5, 0.5), the center of
    * the collider. (0,0) is the top-left; (1,1), the bottom-right
    * Note: the anchor will be aligned to the hot_spot of the entity
    */
    boxcollider_t* collider = (boxcollider_t*)unsafe_get_collider(object);
    surgescript_transform_t* transform = surgescript_object_transform(object);
    double width = collider->width, height = collider->height;
    double x = surgescript_var_get_number(param[0]);
    double y = surgescript_var_get_number(param[1]);
    surgescript_transform_setposition2d(transform, (0.5 - x) * width, (0.5 - y) * height);
    ((collider_t*)collider)->worldpos = scripting_util_world_position(object); /* update worldpos */
    ((collider_t*)collider)->anchor = v2d_new(x, y);

    /* return the object itself (this) */
    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(object));
}

/* contains(): checks if world-position pos = (x, y) is inside the collider */
surgescript_var_t* fun_collisionbox_contains(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* pos = surgescript_objectmanager_get(manager, handle);
    boxcollider_t* collider = (boxcollider_t*)unsafe_get_collider(object);
    v2d_t worldpos = ((collider_t*)collider)->worldpos;
    double halfWidth = collider->width / 2.0, halfHeight = collider->height / 2.0;
    double x = 0.0, y = 0.0;

    scripting_vector2_read(pos, &x, &y);
    return surgescript_var_set_bool(surgescript_var_create(),
        x >= worldpos.x - halfWidth && x <= worldpos.x + halfWidth &&
        y >= worldpos.y - halfHeight && y <= worldpos.y + halfHeight
    );
}

/* returns true if this collider collides with another collider */
surgescript_var_t* fun_collisionbox_collideswith(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t other_collider = surgescript_var_get_objecthandle(param[0]);
    boxcollider_t* collider = (boxcollider_t*)unsafe_get_collider(object);
    double my_left = ((collider_t*)collider)->worldpos.x - collider->width / 2.0;
    double my_right = ((collider_t*)collider)->worldpos.x + collider->width / 2.0;
    double my_top = ((collider_t*)collider)->worldpos.y - collider->height / 2.0;
    double my_bottom = ((collider_t*)collider)->worldpos.y + collider->height / 2.0;
    collider_t* other = safe_get_collider(surgescript_objectmanager_get(manager, other_collider));

    switch(other->type) {
        case COLLIDER_TYPE_BOX: {
            double other_left = other->worldpos.x - ((boxcollider_t*)other)->width / 2.0;
            double other_right = other->worldpos.x + ((boxcollider_t*)other)->width / 2.0;
            double other_top = other->worldpos.y - ((boxcollider_t*)other)->height / 2.0;
            double other_bottom = other->worldpos.y + ((boxcollider_t*)other)->height / 2.0;
            return surgescript_var_set_bool(surgescript_var_create(),
                my_left < other_right && my_right > other_left &&
                my_top < other_bottom && my_bottom > other_top
            );
        }

        case COLLIDER_TYPE_BALL: {
            double cx = other->worldpos.x;
            double cy = other->worldpos.y;
            double r = ((ballcollider_t*)other)->radius;
            double dx = cx - clip(cx, my_left, my_right);
            double dy = cy - clip(cy, my_top, my_bottom);
            return surgescript_var_set_bool(surgescript_var_create(),
                dx * dx + dy * dy < r * r
            );
        }
    }

    return surgescript_var_set_bool(surgescript_var_create(), false);
}

/* set dimensions */
surgescript_var_t* fun_collisionbox_setwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    double width = surgescript_var_get_number(param[0]);
    boxcollider->width = max(1.0, width);
    return NULL;
}

surgescript_var_t* fun_collisionbox_setheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    double height = surgescript_var_get_number(param[0]);
    boxcollider->height = max(1.0, height);
    return NULL;
}

/* get dimensions */
surgescript_var_t* fun_collisionbox_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    boxcollider_t* boxcollider = (boxcollider_t*)unsafe_get_collider(object);
    return surgescript_var_set_number(surgescript_var_create(), boxcollider->width);
}

surgescript_var_t* fun_collisionbox_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    boxcollider_t* boxcollider = (boxcollider_t*)unsafe_get_collider(object);
    return surgescript_var_set_number(surgescript_var_create(), boxcollider->height);
}

/* get coordinates */
surgescript_var_t* fun_collisionbox_getleft(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    return surgescript_var_set_number(surgescript_var_create(), collider->worldpos.x - boxcollider->width / 2.0);
}

surgescript_var_t* fun_collisionbox_getright(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    return surgescript_var_set_number(surgescript_var_create(), collider->worldpos.x + boxcollider->width / 2.0);
}

surgescript_var_t* fun_collisionbox_gettop(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    return surgescript_var_set_number(surgescript_var_create(), collider->worldpos.y - boxcollider->height / 2.0);
}

surgescript_var_t* fun_collisionbox_getbottom(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    return surgescript_var_set_number(surgescript_var_create(), collider->worldpos.y + boxcollider->height / 2.0);
}

/* render */
surgescript_var_t* fun_collisionbox_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    boxcollider_t* collider = (boxcollider_t*)unsafe_get_collider(object);
    int visible = ((collider_t*)collider)->flags & COLLIDER_FLAG_ISVISIBLE;

    if(visible)
        fun_collisionbox_onrendergizmos(object, param, num_params);

    return NULL;
}

/* render gizmos */
surgescript_var_t* fun_collisionbox_onrendergizmos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    boxcollider_t* collider = (boxcollider_t*)unsafe_get_collider(object);

    if(scripting_util_is_object_inside_screen(object)) {
        color_t color = COLLIDER_COLOR(collider->collider.flags);
        /*v2d_t center = ((collider_t*)collider)->worldpos;*/ /* this cached value may become outdated if an ancestor object changes its position in lateUpdate() */
        v2d_t center = scripting_util_world_position(object);
        v2d_t camera = scripting_util_object_camera(object);
        v2d_t half_screen = v2d_multiply(video_get_screen_size(), 0.5f);

        double left = center.x - floor(collider->width / 2.0);
        double right = center.x + ceil(collider->width / 2.0);
        double top = center.y - ceil(collider->height / 2.0);
        double bottom = center.y + floor(collider->height / 2.0);

        int l = left - (camera.x - half_screen.x);
        int r = right - (camera.x - half_screen.x) - 1;
        int t = top - (camera.y - half_screen.y);
        int b = bottom - (camera.y - half_screen.y) - 1;
        
        image_rect(l, t, r, b, color);
    }

    return NULL;
}

/* zindex */
surgescript_var_t* fun_collisionbox_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), 1.0);
}



/* ---------------------------- CollisionBall routines ------------------------- */

/* ball constructor */
surgescript_var_t* fun_collisionball_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = mallocx(sizeof(ballcollider_t));
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t root = surgescript_objectmanager_root(manager);
    surgescript_objecthandle_t parent = surgescript_object_parent(object);
    surgescript_object_t* entity = NULL;

    /* collider initialization */
    collider->type = COLLIDER_TYPE_BALL;
    collider->entity = surgescript_objectmanager_null(manager);
    collider->colmgr = surgescript_objectmanager_null(manager);
    collider->worldpos = v2d_new(0.0f, 0.0f); /* the center of the collider in world coordinates */
    collider->anchor = v2d_new(0.5f, 0.5f); /* default anchor: at the center of the collider */
    collider->flags = 0;
    darray_init(collider->prev_collisions);
    darray_init(collider->curr_collisions);
    ((ballcollider_t*)collider)->radius = 0.0;
    surgescript_object_set_userdata(object, collider);

    /* center of the collider (lazy evaluation) */
    ssassert(CENTER_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, CENTER_ADDR));

    /* anchor (lazy evaluation) */
    ssassert(ANCHOR_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_null(surgescript_heap_at(heap, ANCHOR_ADDR));

    /* get entity */
    while(!surgescript_object_has_tag(surgescript_objectmanager_get(manager, parent), "entity")) {
        parent = surgescript_object_parent(surgescript_objectmanager_get(manager, parent));
        if(parent == root) {
            scripting_error(object, 
                "Collider \"%s\" must be a descendant of an entity (parent is \"%s\")",
                surgescript_object_name(object),
                surgescript_object_name(surgescript_objectmanager_get(manager, surgescript_object_parent(object)))
            );
            break;
        }
    }
    collider->entity = parent;
    entity = surgescript_objectmanager_get(manager, collider->entity);

    /* validation */
    if(surgescript_object_has_tag(entity, "detached")) {
        scripting_error(object, 
            "\"%s\" won't work with detached entities like \"%s\"",
            surgescript_object_name(object),
            surgescript_object_name(entity)
        );
    }

    /* collision flags */
    if(surgescript_object_has_function(entity, "onCollision"))
        collider->flags |= COLLIDER_FLAG_NOTIFYONCOLLISION;
    if(surgescript_object_has_function(entity, "onOverlap"))
        collider->flags |= COLLIDER_FLAG_NOTIFYONOVERLAP;

    /* done */
    return NULL;
}

/* init variables */
surgescript_var_t* fun_collisionball_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = unsafe_get_collider(object);
    ballcollider_t* ballcollider = (ballcollider_t*)collider;
    collider->colmgr = surgescript_var_get_objecthandle(param[0]); /* collision manager */
    ballcollider->radius = max(1.0, surgescript_var_get_number(param[1])); /* radius */
    return NULL;
}

/* returns true if this collider collides with another collider */
surgescript_var_t* fun_collisionball_collideswith(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t other_collider = surgescript_var_get_objecthandle(param[0]);
    ballcollider_t* collider = (ballcollider_t*)unsafe_get_collider(object);
    v2d_t my_center = ((collider_t*)collider)->worldpos;
    double my_radius = collider->radius;
    collider_t* other = safe_get_collider(surgescript_objectmanager_get(manager, other_collider));

    switch(other->type) {
        case COLLIDER_TYPE_BOX: {
            double other_left = other->worldpos.x - ((boxcollider_t*)other)->width / 2.0;
            double other_right = other->worldpos.x + ((boxcollider_t*)other)->width / 2.0;
            double other_top = other->worldpos.y - ((boxcollider_t*)other)->height / 2.0;
            double other_bottom = other->worldpos.y + ((boxcollider_t*)other)->height / 2.0;
            double dx = my_center.x - clip(my_center.x, other_left, other_right);
            double dy = my_center.y - clip(my_center.y, other_top, other_bottom);
            return surgescript_var_set_bool(surgescript_var_create(),
                dx * dx + dy * dy < my_radius * my_radius
            );
        }

        case COLLIDER_TYPE_BALL: {
            v2d_t other_center = other->worldpos;
            double other_radius = ((ballcollider_t*)other)->radius;
            double dx = my_center.x - other_center.x;
            double dy = my_center.y - other_center.y;
            double rr = my_radius + other_radius;
            return surgescript_var_set_bool(surgescript_var_create(),
                dx * dx + dy * dy < rr * rr
            );
        }
    }

    return surgescript_var_set_bool(surgescript_var_create(), false);
}

/* contains(): checks if world-position pos = (x, y) is inside the collider */
surgescript_var_t* fun_collisionball_contains(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_object_t* pos = surgescript_objectmanager_get(manager, handle);
    collider_t* collider = unsafe_get_collider(object);
    double r = ((ballcollider_t*)collider)->radius;
    double x = 0.0, y = 0.0, dx = 0.0, dy = 0.0;

    scripting_vector2_read(pos, &x, &y);
    dx = x - collider->worldpos.x;
    dy = y - collider->worldpos.y;
    
    return surgescript_var_set_bool(surgescript_var_create(), dx * dx + dy * dy <= r * r);
}

/* setAnchor() */
surgescript_var_t* fun_collisionball_setanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /*
    * sets the anchor of the collider to a certain position (x,y),
    * where 0 <= x, y <= 1. Defaults to (0.5, 0.5), the center of
    * the collider. (0,0) is the top-left; (1,1), the bottom-right
    * Note: the anchor will be aligned to the hot_spot of the entity
    */
    ballcollider_t* collider = (ballcollider_t*)unsafe_get_collider(object);
    surgescript_transform_t* transform = surgescript_object_transform(object);
    double size = collider->radius * 2.0;
    double x = surgescript_var_get_number(param[0]);
    double y = surgescript_var_get_number(param[1]);
    surgescript_transform_setposition2d(transform, (0.5 - x) * size, (0.5 - y) * size);
    ((collider_t*)collider)->worldpos = scripting_util_world_position(object); /* update worldpos */
    ((collider_t*)collider)->anchor = v2d_new(x, y);

    /* return the object itself (this) */
    return surgescript_var_set_objecthandle(surgescript_var_create(), surgescript_object_handle(object));
}

/* set radius, in pixels */
surgescript_var_t* fun_collisionball_setradius(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    ballcollider_t* collider = (ballcollider_t*)unsafe_get_collider(object);
    double radius = surgescript_var_get_number(param[0]);
    collider->radius = max(1, radius);
    return NULL;
}

/* get radius, in pixels */
surgescript_var_t* fun_collisionball_getradius(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    ballcollider_t* collider = (ballcollider_t*)unsafe_get_collider(object);
    return surgescript_var_set_number(surgescript_var_create(), collider->radius);
}

/* render */
surgescript_var_t* fun_collisionball_onrender(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    ballcollider_t* collider = (ballcollider_t*)unsafe_get_collider(object);
    int visible = ((collider_t*)collider)->flags & COLLIDER_FLAG_ISVISIBLE;

    if(visible)
        fun_collisionball_onrendergizmos(object, param, num_params);

    return NULL;
}

/* render gizmos */
surgescript_var_t* fun_collisionball_onrendergizmos(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    ballcollider_t* collider = (ballcollider_t*)unsafe_get_collider(object);

    if(scripting_util_is_object_inside_screen(object)) {
        /*v2d_t center = ((collider_t*)collider)->worldpos;*/ /* this cached value may become outdated if an ancestor object changes its position in lateUpdate() */
        v2d_t center = scripting_util_world_position(object);
        v2d_t camera = scripting_util_object_camera(object);
        v2d_t half_screen = v2d_multiply(video_get_screen_size(), 0.5f);
        double r = collider->radius;
        center.x -= (camera.x - half_screen.x);
        center.y -= (camera.y - half_screen.y);
        image_ellipse(center.x, center.y, r, r, COLLIDER_COLOR(collider->collider.flags));
    }

    return NULL;
}

/* zindex */
surgescript_var_t* fun_collisionball_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), 1.0f);
}