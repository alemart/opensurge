/*
 * Open Surge Engine
 * collisions.c - scripting system: collision system
 * Copyright (C) 2018-2019  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include "../core/v2d.h"
#include "../core/darray.h"
#include "../core/image.h"
#include "../core/video.h"

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
    uint8_t flags;
};

typedef struct boxcollider_t boxcollider_t;
struct boxcollider_t
{
    collider_t collider;
    v2d_t size; /* width, height */
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
#define COLLIDER_FLAG_MUSTNOTIFYENTITY      0x2
#define COLLIDER_COLOR()                    (color_rgb(255, 255, 0))
static const surgescript_heapptr_t BALL_CENTER_ADDR = 0;
static inline collider_t* safe_get_collider(surgescript_object_t* object);

static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
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
static surgescript_var_t* fun_collisionbox_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionbox_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

static surgescript_var_t* fun_collisionball_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_collideswith(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_contains(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_setanchor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_getcenter(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_setradius(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_getradius(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_collisionball_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

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
    surgescript_tagsystem_add_tag(tag_system, "CollisionBall", "collider");

    /* methods */
    surgescript_vm_bind(vm, "CollisionBox", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "CollisionBox", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_entity", fun_getentity, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_visible", fun_getvisible, 0);
    surgescript_vm_bind(vm, "CollisionBox", "set_visible", fun_setvisible, 1);
    surgescript_vm_bind(vm, "CollisionBox", "__notify", fun_notify, 1);
    surgescript_vm_bind(vm, "CollisionBox", "__init", fun_collisionbox_init, 3);
    surgescript_vm_bind(vm, "CollisionBox", "constructor", fun_collisionbox_constructor, 0);
    surgescript_vm_bind(vm, "CollisionBox", "collidesWith", fun_collisionbox_collideswith, 1);
    surgescript_vm_bind(vm, "CollisionBox", "contains", fun_collisionbox_contains, 2);
    surgescript_vm_bind(vm, "CollisionBox", "setAnchor", fun_collisionbox_setanchor, 2);
    surgescript_vm_bind(vm, "CollisionBox", "get_left", fun_collisionbox_getleft, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_right", fun_collisionbox_getright, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_top", fun_collisionbox_gettop, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_bottom", fun_collisionbox_getbottom, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_width", fun_collisionbox_getwidth, 0);
    surgescript_vm_bind(vm, "CollisionBox", "get_height", fun_collisionbox_getheight, 0);
    surgescript_vm_bind(vm, "CollisionBox", "set_width", fun_collisionbox_setwidth, 1);
    surgescript_vm_bind(vm, "CollisionBox", "set_height", fun_collisionbox_setheight, 1);
    surgescript_vm_bind(vm, "CollisionBox", "render", fun_collisionbox_render, 0);
    surgescript_vm_bind(vm, "CollisionBox", "zindex", fun_collisionbox_zindex, 0);

    surgescript_vm_bind(vm, "CollisionBall", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "CollisionBall", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "CollisionBall", "get_entity", fun_getentity, 0);
    surgescript_vm_bind(vm, "CollisionBall", "get_visible", fun_getvisible, 0);
    surgescript_vm_bind(vm, "CollisionBall", "set_visible", fun_setvisible, 1);
    surgescript_vm_bind(vm, "CollisionBall", "__notify", fun_notify, 1);
    surgescript_vm_bind(vm, "CollisionBall", "__init", fun_collisionball_init, 2);
    surgescript_vm_bind(vm, "CollisionBall", "constructor", fun_collisionball_constructor, 0);
    surgescript_vm_bind(vm, "CollisionBall", "collidesWith", fun_collisionball_collideswith, 1);
    surgescript_vm_bind(vm, "CollisionBall", "contains", fun_collisionball_contains, 2);
    surgescript_vm_bind(vm, "CollisionBall", "setAnchor", fun_collisionball_setanchor, 2);
    surgescript_vm_bind(vm, "CollisionBall", "get_center", fun_collisionball_getcenter, 0);
    surgescript_vm_bind(vm, "CollisionBall", "get_radius", fun_collisionball_getradius, 0);
    surgescript_vm_bind(vm, "CollisionBall", "set_radius", fun_collisionball_setradius, 1);
    surgescript_vm_bind(vm, "CollisionBall", "render", fun_collisionball_render, 0);
    surgescript_vm_bind(vm, "CollisionBall", "zindex", fun_collisionball_zindex, 0);

    surgescript_vm_bind(vm, "CollisionManager", "state:main", fun_manager_main, 0);
    surgescript_vm_bind(vm, "CollisionManager", "constructor", fun_manager_constructor, 0);
    surgescript_vm_bind(vm, "CollisionManager", "destructor", fun_manager_destructor, 0);
    surgescript_vm_bind(vm, "CollisionManager", "destroy", fun_manager_destroy, 0);
    surgescript_vm_bind(vm, "CollisionManager", "__notify", fun_manager_notify, 1);
}

/* Returns the collider structure if the given object is a collider,
   or a crash if it isn't */
collider_t* safe_get_collider(surgescript_object_t* object)
{
    if(!surgescript_object_has_tag(object, "collider")) {
        fatal_error("Scripting error: \"%s\" isn't a collider", surgescript_object_name(object));
        return NULL;
    }

    return (collider_t*)surgescript_object_userdata(object);
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
     * TODO: can this be made faster?
     */
    for(int i = 1; i < darray_length(colmgr->colliders); i++) {
        surgescript_object_t* collider = surgescript_objectmanager_get(manager, colmgr->colliders[i]);
        for(int j = 0; j < i; j++) {
            surgescript_var_set_objecthandle(tmp, colmgr->colliders[j]);
            surgescript_object_call_function(collider, "collidesWith", p, 1, ret);
            if(surgescript_var_get_bool(ret)) {
                surgescript_object_t* other_collider = surgescript_objectmanager_get(manager, colmgr->colliders[j]);
                surgescript_object_call_function(collider, "__notify", p, 1, ret);
                surgescript_var_set_objecthandle(tmp, colmgr->colliders[i]);
                surgescript_object_call_function(other_collider, "__notify", p, 1, ret);
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
    collisionmanager_t* colmgr = surgescript_object_userdata(object);
    surgescript_objecthandle_t collider = surgescript_var_get_objecthandle(param[0]);
    darray_push(colmgr->colliders, collider);
    return NULL;
}



/* ------------------------- Collider routines ---------------------------- */

/* collider destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    darray_release(collider->curr_collisions);
    darray_release(collider->prev_collisions);
    free(collider);
    return NULL;
}

/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_var_t* tmp = surgescript_var_create();
    const surgescript_var_t* p[] = { tmp };
    int i;

    /* update world position */
    collider->worldpos = scripting_util_world_position(object); /* cached */

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

    /* done */
    surgescript_var_destroy(tmp);
    return NULL;
}

/* get the entity associated with the collider */
surgescript_var_t* fun_getentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    return surgescript_var_set_objecthandle(surgescript_var_create(), collider->entity);
}

/* is the collider visible? */
surgescript_var_t* fun_getvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    return surgescript_var_set_bool(surgescript_var_create(), (collider->flags & COLLIDER_FLAG_ISVISIBLE) != 0);
}

/* change collider visibility */
surgescript_var_t* fun_setvisible(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    bool visible = surgescript_var_get_bool(param[0]);

    if(!visible)
        collider->flags &= ~COLLIDER_FLAG_ISVISIBLE;
    else
        collider->flags |= COLLIDER_FLAG_ISVISIBLE;

    return NULL;
}

/* the collision manager is telling us about a collision with some other collider */
surgescript_var_t* fun_notify(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    surgescript_objecthandle_t other_collider = surgescript_var_get_objecthandle(param[0]);

    darray_push(collider->curr_collisions, other_collider);
    if(collider->flags & COLLIDER_FLAG_MUSTNOTIFYENTITY) {
        surgescript_objectmanager_t* manager = surgescript_object_manager(object);
        surgescript_var_t* tmp = surgescript_var_create();
        const surgescript_var_t* p[] = { tmp };
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

        /* done */
        surgescript_var_destroy(tmp);
    }

    /* done */
    return NULL;
}



/* ----------------------------- CollisionBox routines ------------------------------- */

/* box constructor */
surgescript_var_t* fun_collisionbox_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = mallocx(sizeof(boxcollider_t));
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t root = surgescript_objectmanager_root(manager);
    surgescript_objecthandle_t parent = surgescript_object_parent(object);
    surgescript_object_t* entity = NULL;

    /* collider initialization */
    collider->type = COLLIDER_TYPE_BOX;
    collider->entity = surgescript_objectmanager_null(manager);
    collider->colmgr = surgescript_objectmanager_null(manager);
    collider->worldpos = v2d_new(0, 0);
    collider->flags = 0;
    darray_init(collider->prev_collisions);
    darray_init(collider->curr_collisions);
    ((boxcollider_t*)collider)->size = v2d_new(0, 0);
    surgescript_object_set_userdata(object, collider);

    /* get entity */
    while(!surgescript_object_has_tag(surgescript_objectmanager_get(manager, parent), "entity")) {
        parent = surgescript_object_parent(surgescript_objectmanager_get(manager, parent));
        if(parent == root) {
            fatal_error(
                "Scripting Error: collider \"%s\" must be a descendant of an entity (parent is \"%s\")",
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
        fatal_error(
            "Scripting Error: \"%s\" won't work with detached entities like \"%s\"",
            surgescript_object_name(object),
            surgescript_object_name(entity)
        );
    }

    /* collision flags */
    if(surgescript_object_has_function(entity, "onCollision"))
        collider->flags |= COLLIDER_FLAG_MUSTNOTIFYENTITY;

    /* done */
    return NULL;
}

/* init variables */
surgescript_var_t* fun_collisionbox_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    collider->colmgr = surgescript_var_get_objecthandle(param[0]); /* collision manager */
    boxcollider->size.x = max(1.0f, surgescript_var_get_number(param[1])); /* width */
    boxcollider->size.y = max(1.0f, surgescript_var_get_number(param[2])); /* height */
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
    boxcollider_t* collider = surgescript_object_userdata(object);
    surgescript_transform_t* transform = surgescript_object_transform(object);
    double width = collider->size.x, height = collider->size.y;
    double x = surgescript_var_get_number(param[0]);
    double y = surgescript_var_get_number(param[1]);
    surgescript_transform_setposition2d(transform, (0.5 - x) * width, (0.5 - y) * height);
    ((collider_t*)collider)->worldpos = scripting_util_world_position(object); /* update worldpos */
    return NULL;
}

/* contains(): checks if world-position (x, y) is inside the collider */
surgescript_var_t* fun_collisionbox_contains(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    boxcollider_t* collider = surgescript_object_userdata(object);
    v2d_t worldpos = ((collider_t*)collider)->worldpos;
    double width = collider->size.x, height = collider->size.y;
    double x = surgescript_var_get_number(param[0]);
    double y = surgescript_var_get_number(param[1]);
    return surgescript_var_set_bool(surgescript_var_create(),
        x >= worldpos.x - width / 2.0 && x <= worldpos.x + width / 2.0 &&
        y >= worldpos.y - height / 2.0 && y <= worldpos.y + height / 2.0
    );
}

/* returns true if this collider collides with another collider */
surgescript_var_t* fun_collisionbox_collideswith(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t other_collider = surgescript_var_get_objecthandle(param[0]);
    boxcollider_t* collider = surgescript_object_userdata(object);
    double my_left = ((collider_t*)collider)->worldpos.x - collider->size.x / 2.0;
    double my_right = ((collider_t*)collider)->worldpos.x + collider->size.x / 2.0;
    double my_top = ((collider_t*)collider)->worldpos.y - collider->size.y / 2.0;
    double my_bottom = ((collider_t*)collider)->worldpos.y + collider->size.y / 2.0;
    collider_t* other = safe_get_collider(surgescript_objectmanager_get(manager, other_collider));

    switch(other->type) {
        case COLLIDER_TYPE_BOX: {
            double other_left = other->worldpos.x - ((boxcollider_t*)other)->size.x / 2.0;
            double other_right = other->worldpos.x + ((boxcollider_t*)other)->size.x / 2.0;
            double other_top = other->worldpos.y - ((boxcollider_t*)other)->size.y / 2.0;
            double other_bottom = other->worldpos.y + ((boxcollider_t*)other)->size.y / 2.0;
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
    collider_t* collider = surgescript_object_userdata(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    double width = surgescript_var_get_number(param[0]);
    boxcollider->size.x = max(1.0f, width);
    return NULL;
}

surgescript_var_t* fun_collisionbox_setheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    double height = surgescript_var_get_number(param[0]);
    boxcollider->size.y = max(1.0f, height);
    return NULL;
}

/* get dimensions */
surgescript_var_t* fun_collisionbox_getwidth(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    boxcollider_t* boxcollider = (boxcollider_t*)surgescript_object_userdata(object);
    return surgescript_var_set_number(surgescript_var_create(), boxcollider->size.x);
}

surgescript_var_t* fun_collisionbox_getheight(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    boxcollider_t* boxcollider = (boxcollider_t*)surgescript_object_userdata(object);
    return surgescript_var_set_number(surgescript_var_create(), boxcollider->size.y);
}

/* get coordinates */
surgescript_var_t* fun_collisionbox_getleft(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    return surgescript_var_set_number(surgescript_var_create(), collider->worldpos.x - boxcollider->size.x / 2.0f);
}

surgescript_var_t* fun_collisionbox_getright(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    return surgescript_var_set_number(surgescript_var_create(), collider->worldpos.x + boxcollider->size.x / 2.0f);
}

surgescript_var_t* fun_collisionbox_gettop(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    return surgescript_var_set_number(surgescript_var_create(), collider->worldpos.y - boxcollider->size.y / 2.0f);
}

surgescript_var_t* fun_collisionbox_getbottom(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    boxcollider_t* boxcollider = (boxcollider_t*)collider;
    return surgescript_var_set_number(surgescript_var_create(), collider->worldpos.y + boxcollider->size.y / 2.0f);
}

/* render */
surgescript_var_t* fun_collisionbox_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    boxcollider_t* collider = surgescript_object_userdata(object);
    int debug = ((collider_t*)collider)->flags & COLLIDER_FLAG_ISVISIBLE;

    if(debug && scripting_util_is_object_inside_screen(object)) {
        color_t color = COLLIDER_COLOR();
        v2d_t camera = scripting_util_object_camera(object);
        v2d_t center = ((collider_t*)collider)->worldpos;
        double left = center.x - collider->size.x / 2.0;
        double right = center.x + collider->size.x / 2.0;
        double top = center.y - collider->size.y / 2.0;
        double bottom = center.y + collider->size.y / 2.0;

        int l = left - (camera.x - VIDEO_SCREEN_W / 2);
        int r = right - (camera.x - VIDEO_SCREEN_W / 2) - 1;
        int t = top - (camera.y - VIDEO_SCREEN_H / 2);
        int b = bottom - (camera.y - VIDEO_SCREEN_H / 2) - 1;
        
        image_rect(l, t, r, b, color);
    }

    return NULL;
}

/* zindex */
surgescript_var_t* fun_collisionbox_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), 1.0f);
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
    surgescript_objecthandle_t me = surgescript_object_handle(object);
    surgescript_objecthandle_t center = surgescript_objectmanager_spawn(manager, me, "Vector2", NULL);
    surgescript_object_t* entity = NULL;

    /* collider initialization */
    collider->type = COLLIDER_TYPE_BALL;
    collider->entity = surgescript_objectmanager_null(manager);
    collider->colmgr = surgescript_objectmanager_null(manager);
    collider->worldpos = v2d_new(0, 0);
    collider->flags = 0;
    darray_init(collider->prev_collisions);
    darray_init(collider->curr_collisions);
    ((ballcollider_t*)collider)->radius = 0.0f;
    surgescript_object_set_userdata(object, collider);

    /* allocate center Vector2 */
    ssassert(BALL_CENTER_ADDR == surgescript_heap_malloc(heap));
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, BALL_CENTER_ADDR), center);

    /* get entity */
    while(!surgescript_object_has_tag(surgescript_objectmanager_get(manager, parent), "entity")) {
        parent = surgescript_object_parent(surgescript_objectmanager_get(manager, parent));
        if(parent == root) {
            fatal_error(
                "Scripting Error: collider \"%s\" must be a descendant of an entity (parent is \"%s\")",
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
        fatal_error(
            "Scripting Error: \"%s\" won't work with detached entities like \"%s\"",
            surgescript_object_name(object),
            surgescript_object_name(entity)
        );
    }

    /* collision flags */
    if(surgescript_object_has_function(entity, "onCollision"))
        collider->flags |= COLLIDER_FLAG_MUSTNOTIFYENTITY;

    /* done */
    return NULL;
}

/* init variables */
surgescript_var_t* fun_collisionball_init(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    ballcollider_t* ballcollider = (ballcollider_t*)collider;
    collider->colmgr = surgescript_var_get_objecthandle(param[0]); /* collision manager */
    ballcollider->radius = max(1.0f, surgescript_var_get_number(param[1])); /* radius */
    return NULL;
}

/* returns true if this collider collides with another collider */
surgescript_var_t* fun_collisionball_collideswith(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t other_collider = surgescript_var_get_objecthandle(param[0]);
    ballcollider_t* collider = surgescript_object_userdata(object);
    v2d_t my_center = ((collider_t*)collider)->worldpos;
    double my_radius = collider->radius;
    collider_t* other = safe_get_collider(surgescript_objectmanager_get(manager, other_collider));

    switch(other->type) {
        case COLLIDER_TYPE_BOX: {
            double other_left = other->worldpos.x - ((boxcollider_t*)other)->size.x / 2.0;
            double other_right = other->worldpos.x + ((boxcollider_t*)other)->size.x / 2.0;
            double other_top = other->worldpos.y - ((boxcollider_t*)other)->size.y / 2.0;
            double other_bottom = other->worldpos.y + ((boxcollider_t*)other)->size.y / 2.0;
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

/* contains(): checks if world-position (x, y) is inside the collider */
surgescript_var_t* fun_collisionball_contains(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    double x = surgescript_var_get_number(param[0]);
    double y = surgescript_var_get_number(param[1]);
    double r = ((ballcollider_t*)collider)->radius;
    double dx = x - collider->worldpos.x;
    double dy = y - collider->worldpos.y;
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
    ballcollider_t* collider = surgescript_object_userdata(object);
    surgescript_transform_t* transform = surgescript_object_transform(object);
    double size = collider->radius * 2.0;
    double x = surgescript_var_get_number(param[0]);
    double y = surgescript_var_get_number(param[1]);
    surgescript_transform_setposition2d(transform, (0.5 - x) * size, (0.5 - y) * size);
    ((collider_t*)collider)->worldpos = scripting_util_world_position(object); /* update worldpos */
    return NULL;
}

/* getCenter(): Vector2 (world coordinates) */
surgescript_var_t* fun_collisionball_getcenter(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    collider_t* collider = surgescript_object_userdata(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(surgescript_heap_at(heap, BALL_CENTER_ADDR));
    surgescript_object_t* v2 = surgescript_objectmanager_get(manager, handle);
    scripting_vector2_update(v2, collider->worldpos.x, collider->worldpos.y);
    return surgescript_var_set_objecthandle(surgescript_var_create(), handle);
}

/* set radius, in pixels */
surgescript_var_t* fun_collisionball_setradius(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    ballcollider_t* collider = surgescript_object_userdata(object);
    double radius = surgescript_var_get_number(param[0]);
    collider->radius = max(1, radius);
    return NULL;
}

/* get radius, in pixels */
surgescript_var_t* fun_collisionball_getradius(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    ballcollider_t* collider = surgescript_object_userdata(object);
    return surgescript_var_set_number(surgescript_var_create(), collider->radius);
}

/* render */
surgescript_var_t* fun_collisionball_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    ballcollider_t* collider = surgescript_object_userdata(object);
    int debug = ((collider_t*)collider)->flags & COLLIDER_FLAG_ISVISIBLE;

    if(debug && scripting_util_is_object_inside_screen(object)) {
        v2d_t camera = scripting_util_object_camera(object);
        v2d_t center = ((collider_t*)collider)->worldpos;
        double r = collider->radius;
        center.x -= (camera.x - VIDEO_SCREEN_W / 2);
        center.y -= (camera.y - VIDEO_SCREEN_H / 2);
        image_ellipse(center.x, center.y, r, r, COLLIDER_COLOR());
    }

    return NULL;
}

/* zindex */
surgescript_var_t* fun_collisionball_zindex(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return surgescript_var_set_number(surgescript_var_create(), 1.0f);
}