/*
 * Open Surge Engine
 * entitymanager.c - scripting system: Entity Manager
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

#define FASTHASH_INLINE
#include "../core/fasthash.h"

#include <surgescript.h>
#include <string.h>
#include "scripting.h"
#include "../core/v2d.h"
#include "../core/util.h"
#include "../core/video.h"
#include "../scenes/level.h"

typedef struct entityinfo_t entityinfo_t;
struct entityinfo_t {
    surgescript_objecthandle_t handle; /* hash key: SurgeScript object */
    uint64_t id; /* uniquely identifies the entity in the Level */
    v2d_t spawn_point; /* spawn point */
    bool is_persistent; /* usually placed via level editor; will be saved in the .lev file */
    bool is_sleeping; /* inactive */
};

typedef struct entitymanagerdatabase_t entitymanagerdatabase_t;
struct entitymanagerdatabase_t {
    fasthash_t* info;
    fasthash_t* id_to_handle;
    entityinfo_t* cached_query;
};

static entityinfo_t NULL_ENTRY = { .handle = 0, .id = 0 };
static entityinfo_t* entityinfo_ctor(entityinfo_t info) { return (entityinfo_t*)memcpy(mallocx(sizeof(info)), &info, sizeof(info)); }
static void entityinfo_dtor(void* entry) { free(entry); }

static surgescript_objecthandle_t* handle_ctor(surgescript_objecthandle_t handle) { return (surgescript_objecthandle_t*)memcpy(mallocx(sizeof(handle)), &handle, sizeof(handle)); }
static void handle_dtor(void* handle) { free(handle); }

/* C API; make sure you call these with an actual EntityManager object (it won't be checked) */
bool entitymanager_has_entity_info(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
void entitymanager_remove_entity_info(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);

surgescript_objecthandle_t entitymanager_find_entity_by_id(surgescript_object_t* entity_manager, uint64_t entity_id);
uint64_t entitymanager_get_entity_id(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
void entitymanager_set_entity_id(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle, uint64_t entity_id);
v2d_t entitymanager_get_entity_spawn_point(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
bool entitymanager_is_entity_persistent(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
void entitymanager_set_entity_persistent(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle, bool is_persistent);
bool entitymanager_is_entity_sleeping(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
void entitymanager_set_entity_sleeping(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle, bool is_sleeping);

/* SurgeScript API */
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawnentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_releasechildren(surgescript_object_t* object, const surgescript_var_t** param, int num_params);

/* helpers */
#define generate_entity_id()            (random64() & UINT64_C(0xFFFFFFFF)) /* in Open Surge 0.6.0.x and 0.5.x, we used all 64 bits */
#define get_db(entity_manager)          ((entitymanagerdatabase_t*)surgescript_object_userdata(entity_manager))
#define get_info(db, entity_handle)     ((entityinfo_t*)fasthash_get((db)->info, (entity_handle)))
static inline entityinfo_t* quick_lookup(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);



/*
 * scripting_register_entitymanager()
 * Register the EntityManager object
 */
void scripting_register_entitymanager(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "EntityManager", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "EntityManager", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "EntityManager", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "EntityManager", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "EntityManager", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "EntityManager", "spawnEntity", fun_spawnentity, 2);
    surgescript_vm_bind(vm, "EntityManager", "__releaseChildren", fun_releasechildren, 0);
}




/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);

    /* validate: Level must be the parent object */
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    const char* parent_name = surgescript_object_name(parent);
    if(0 != strcmp(parent_name, "Level")) {
        scripting_error(object, "Not a child of Level");
        return NULL;
    }

    /* allocate a database */
    int lg2_cap = 15;
    entitymanagerdatabase_t* db = mallocx(sizeof *db);
    db->info = fasthash_create(entityinfo_dtor, lg2_cap);
    db->id_to_handle = fasthash_create(handle_dtor, lg2_cap);
    db->cached_query = &NULL_ENTRY;
    surgescript_object_set_userdata(object, db);

    /* done */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* release the database */
    entitymanagerdatabase_t* db = get_db(object);
    fasthash_destroy(db->id_to_handle);
    fasthash_destroy(db->info);
    free(db);

    /* done! */
    return NULL;
}

/* destroy function */
surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* spawn function */
surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* disabled */
    return NULL;
}

/* spawn an entity at a position in world space */
surgescript_var_t* fun_spawnentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const char* entity_name = surgescript_var_fast_get_string(param[0]);
    surgescript_objecthandle_t position_handle = surgescript_var_get_objecthandle(param[1]);

    /* validate: does the object exist? */
    if(!surgescript_objectmanager_is_declared(manager, entity_name)) {
        scripting_error(object, "Can't spawn entity: object \"%s\" doesn't exist!", entity_name);
        return NULL;
    }

    /* validate: accept only entities */
    surgescript_tagsystem_t* tag_system = surgescript_objectmanager_tagsystem(manager);
    if(!surgescript_tagsystem_has_tag(tag_system, entity_name, "entity")) {
        scripting_error(object, "Can't spawn entity: object \"%s\" isn't tagged \"entity\"!", entity_name);
        return NULL;
    }

    /* sanity check */
    if(surgescript_object_has_tag(object, "detached") && !surgescript_object_has_tag(object, "private")) {
        video_showmessage("Entity \"%s\" is tagged detached, but not private. Fixing...", entity_name);
        surgescript_tagsystem_add_tag(tag_system, entity_name, "private");
    }

    /* spawn the entity */
    //surgescript_objecthandle_t me = surgescript_object_handle(object);
    //surgescript_objecthandle_t entity_parent = me;
    surgescript_objecthandle_t entity_parent = surgescript_object_parent(object);
    surgescript_objecthandle_t entity_handle = surgescript_objectmanager_spawn(manager, entity_parent, entity_name, NULL);

    /* read the spawn point */
    surgescript_object_t* position = surgescript_objectmanager_get(manager, position_handle);
    v2d_t spawn_point = scripting_vector2_to_v2d(position);

    /* position the entity */
    surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);
    scripting_util_set_world_position(entity, spawn_point);

    /* generate entity info */
    entityinfo_t* info = entityinfo_ctor((entityinfo_t) {
        .handle = entity_handle,
        .id = generate_entity_id(),
        .spawn_point = spawn_point,
        .is_sleeping = !(
            surgescript_object_has_tag(entity, "awake") ||
            surgescript_object_has_tag(entity, "detached")
        ),
        .is_persistent = !(
            surgescript_object_has_tag(entity, "private") ||
            /*surgescript_object_has_tag(entity, "detached") ||*/
            level_is_setup_object(entity_name)
        )
    });

    /* store entity info */
    entitymanagerdatabase_t* db = get_db(object);
    fasthash_put(db->info, info->handle, info);
    fasthash_put(db->id_to_handle, info->id, handle_ctor(info->handle));

    /* return the handle to the spawned entity */
    return surgescript_var_set_objecthandle(surgescript_var_create(), entity_handle);
}

/* release all children */
surgescript_var_t* fun_releasechildren(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    int child_count = surgescript_object_child_count(object);

    /* TODO delegate */

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





/*
 *
 * C API
 * 
 */

/* performs a quick lookup */
entityinfo_t* quick_lookup(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle)
{
    /* this gotta be fast! */
    entitymanagerdatabase_t* db = get_db(entity_manager);

    /* the rationale for caching is that we tend to make multiple
       queries related to the same entity sequentially in time */
    if(db->cached_query->handle != entity_handle) {
        entityinfo_t* info = get_info(db, entity_handle); /* NULL if not found */
        return info != NULL ? ((db->cached_query = info)) : NULL;
    }

    /* the entry was cached */
    return db->cached_query;
}

/* do we have the info of the given entity? */
bool entitymanager_has_entity_info(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle)
{
    return quick_lookup(entity_manager, entity_handle) != NULL;
}

/* remove entity info */
void entitymanager_remove_entity_info(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle)
{
    entitymanagerdatabase_t* db = get_db(entity_manager);

    if(entitymanager_has_entity_info(entity_manager, entity_handle)) {
        uint64_t entity_id = entitymanager_get_entity_id(entity_manager, entity_handle);
        fasthash_delete(db->info, entity_handle);
        fasthash_delete(db->id_to_handle, entity_id);
    }
}

/* get the ID of an entity */
uint64_t entitymanager_get_entity_id(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle)
{
    entityinfo_t* info = quick_lookup(entity_manager, entity_handle);
    return info != NULL ? info->id : 0;
}

/* change the ID of an entity */
void entitymanager_set_entity_id(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle, uint64_t entity_id)
{
    entityinfo_t* info = quick_lookup(entity_manager, entity_handle);
    if(info != NULL)
        info->id = entity_id;
}

/* get the spawn point of an entity */
v2d_t entitymanager_get_entity_spawn_point(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle)
{
    entityinfo_t* info = quick_lookup(entity_manager, entity_handle);
    return info != NULL ? info->spawn_point : v2d_new(0, 0);
}

/* is the entity persistent? */
bool entitymanager_is_entity_persistent(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle)
{
    entityinfo_t* info = quick_lookup(entity_manager, entity_handle);
    return info != NULL ? info->is_persistent : false;
}

/* change the persistent flag of an entity */
void entitymanager_set_entity_persistent(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle, bool is_persistent)
{
    entityinfo_t* info = quick_lookup(entity_manager, entity_handle);
    if(info != NULL)
        info->is_persistent = is_persistent;
}

/* is the entity sleeping? */
bool entitymanager_is_entity_sleeping(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle)
{
    entityinfo_t* info = quick_lookup(entity_manager, entity_handle);
    return info != NULL ? info->is_sleeping : true;
}

/* change the sleeping flag of an entity */
void entitymanager_set_entity_sleeping(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle, bool is_sleeping)
{
    entityinfo_t* info = quick_lookup(entity_manager, entity_handle);
    if(info != NULL)
        info->is_sleeping = is_sleeping;
}

/* find entity by ID. This may return null! */
surgescript_objecthandle_t entitymanager_find_entity_by_id(surgescript_object_t* entity_manager, uint64_t entity_id)
{
    entitymanagerdatabase_t* db = get_db(entity_manager);
    surgescript_objecthandle_t* handle_ptr = (surgescript_objecthandle_t*)fasthash_get(db->id_to_handle, entity_id);
    surgescript_objectmanager_t* manager = surgescript_object_manager(entity_manager);

    /* ID not found */
    if(handle_ptr == NULL)
        return surgescript_objectmanager_null(manager);

    /* the entity no longer exists */
    if(!surgescript_objectmanager_exists(manager, *handle_ptr))
        return surgescript_objectmanager_null(manager);

    /* success! */
    return *handle_ptr;
}