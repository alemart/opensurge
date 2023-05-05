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
#include "../core/stringutil.h"
#include "../core/video.h"
#include "../core/darray.h"
#include "../scenes/level.h"

typedef struct entityinfo_t entityinfo_t;
struct entityinfo_t {
    surgescript_objecthandle_t handle; /* hash key: SurgeScript object */
    uint64_t id; /* uniquely identifies the entity in the Level */
    v2d_t spawn_point; /* spawn point */
    bool is_persistent; /* usually placed via level editor; will be saved in the .lev file */
    bool is_sleeping; /* inactive */
};

typedef struct entitydb_t entitydb_t;
struct entitydb_t {

    /* entity info */
    fasthash_t* info;
    fasthash_t* id_to_handle;
    entityinfo_t* cached_query;

    /* late update queue */
    DARRAY(surgescript_objecthandle_t, late_update_queue);

    /* brick-like objects */
    DARRAY(surgescript_objecthandle_t, bricklike_objects);

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
static surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_update(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_destroy(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawn(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_spawnentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_entity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_entityid(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_findentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_findentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_releasechildren(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_addtolateupdatequeue(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_consumelateupdatequeue(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static surgescript_var_t* fun_addbricklikeobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params);
static const surgescript_heapptr_t AWAKEENTITYCONTAINER_ADDR = 0;
static const surgescript_heapptr_t UNAWAKEENTITYCONTAINER_ADDR = 1;

/* helpers */
#define generate_entity_id()            (random64() & UINT64_C(0xFFFFFFFF)) /* in Open Surge 0.6.0.x and 0.5.x, we used all 64 bits */
#define get_db(entity_manager)          ((entitydb_t*)surgescript_object_userdata(entity_manager))
#define get_info(db, entity_handle)     ((entityinfo_t*)fasthash_get((db)->info, (entity_handle)))
static inline entityinfo_t* quick_lookup(surgescript_object_t* entity_manager, surgescript_objecthandle_t entity_handle);
static bool update_subtree(surgescript_object_t* object);



/*
 * scripting_register_entitymanager()
 * Register the EntityManager object
 */
void scripting_register_entitymanager(surgescript_vm_t* vm)
{
    surgescript_vm_bind(vm, "EntityManager", "state:main", fun_main, 0);
    surgescript_vm_bind(vm, "EntityManager", "update", fun_update, 0);
    surgescript_vm_bind(vm, "EntityManager", "render", fun_render, 0);
    surgescript_vm_bind(vm, "EntityManager", "constructor", fun_constructor, 0);
    surgescript_vm_bind(vm, "EntityManager", "destructor", fun_destructor, 0);
    surgescript_vm_bind(vm, "EntityManager", "destroy", fun_destroy, 0);
    surgescript_vm_bind(vm, "EntityManager", "spawn", fun_spawn, 1);
    surgescript_vm_bind(vm, "EntityManager", "spawnEntity", fun_spawnentity, 2);
    surgescript_vm_bind(vm, "EntityManager", "entity", fun_entity, 1);
    surgescript_vm_bind(vm, "EntityManager", "entityId", fun_entityid, 1);
    surgescript_vm_bind(vm, "EntityManager", "findEntity", fun_findentity, 1);
    surgescript_vm_bind(vm, "EntityManager", "findEntities", fun_findentities, 1);
    surgescript_vm_bind(vm, "EntityManager", "__releaseChildren", fun_releasechildren, 0);
    surgescript_vm_bind(vm, "EntityManager", "addToLateUpdateQueue", fun_addtolateupdatequeue, 1);
    surgescript_vm_bind(vm, "EntityManager", "consumeLateUpdateQueue", fun_consumelateupdatequeue, 0);
    surgescript_vm_bind(vm, "EntityManager", "addBricklikeObject", fun_addbricklikeobject, 1);
}




/* main state */
surgescript_var_t* fun_main(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* don't visit my children when traversing the object tree.
       Instead, call update(). I will update the sub-tree in which
       I am the root in my own special way. */
    surgescript_object_set_active(object, false);

    /* done */
    return NULL;
}

/* constructor */
surgescript_var_t* fun_constructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* validate: Level must be the parent object */
    surgescript_objecthandle_t parent_handle = surgescript_object_parent(object);
    surgescript_object_t* parent = surgescript_objectmanager_get(manager, parent_handle);
    const char* parent_name = surgescript_object_name(parent);
    if(0 != strcmp(parent_name, "Level")) {
        scripting_error(object, "Not a child of Level");
        return NULL;
    }

    /* allocate a database */
    entitydb_t* db = mallocx(sizeof *db);

    int lg2_cap = 15;
    db->info = fasthash_create(entityinfo_dtor, lg2_cap);
    db->id_to_handle = fasthash_create(handle_dtor, lg2_cap);
    db->cached_query = &NULL_ENTRY;

    darray_init(db->late_update_queue);
    darray_init(db->bricklike_objects);

    surgescript_object_set_userdata(object, db);

    /* allocate variables */
    ssassert(AWAKEENTITYCONTAINER_ADDR == surgescript_heap_malloc(heap));
    ssassert(UNAWAKEENTITYCONTAINER_ADDR == surgescript_heap_malloc(heap));

    /* spawn the entity containers */
    surgescript_objecthandle_t this_handle = surgescript_object_handle(object);
    surgescript_objecthandle_t awake_container = surgescript_objectmanager_spawn(manager, this_handle, "AwakeEntityContainer", object);
    surgescript_objecthandle_t unawake_container = surgescript_objectmanager_spawn(manager, this_handle, "EntityContainer", object);

    surgescript_var_set_objecthandle(surgescript_heap_at(heap, AWAKEENTITYCONTAINER_ADDR), awake_container);
    surgescript_var_set_objecthandle(surgescript_heap_at(heap, UNAWAKEENTITYCONTAINER_ADDR), unawake_container);

    /* done */
    return NULL;
}

/* destructor */
surgescript_var_t* fun_destructor(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* release the database */
    entitydb_t* db = get_db(object);

    darray_release(db->bricklike_objects);
    darray_release(db->late_update_queue);

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
    surgescript_heap_t* heap = surgescript_object_heap(object);
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const char* entity_name = surgescript_var_fast_get_string(param[0]);
    surgescript_objecthandle_t position_handle = surgescript_var_get_objecthandle(param[1]);

    /* validate: does the object exist? */
    if(!surgescript_objectmanager_class_exists(manager, entity_name)) {
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
    if(
        surgescript_tagsystem_has_tag(tag_system, entity_name, "detached") &&
        !surgescript_tagsystem_has_tag(tag_system, entity_name, "private")
    ) {
        video_showmessage("Entity \"%s\" is tagged detached, but not private. Fixing...", entity_name);
        surgescript_tagsystem_add_tag(tag_system, entity_name, "private");
    }

    /* decide the parent container: is the new entity awake or not? */
    bool is_awake = (
        surgescript_tagsystem_has_tag(tag_system, entity_name, "awake") ||
        surgescript_tagsystem_has_tag(tag_system, entity_name, "detached")
    );
    surgescript_objecthandle_t parent_container = surgescript_var_get_objecthandle(
        is_awake ?
        surgescript_heap_at(heap, AWAKEENTITYCONTAINER_ADDR) :
        surgescript_heap_at(heap, UNAWAKEENTITYCONTAINER_ADDR)
    );

    /* spawn the entity */
    //surgescript_objecthandle_t me = surgescript_object_handle(object);
    //surgescript_objecthandle_t entity_parent = me;
    surgescript_objecthandle_t entity_parent = surgescript_object_parent(object);
    //surgescript_objecthandle_t entity_parent = parent_container;
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
    entitydb_t* db = get_db(object);
    fasthash_put(db->info, info->handle, info);
    fasthash_put(db->id_to_handle, info->id, handle_ctor(info->handle));

    /* return the handle to the spawned entity */
    return surgescript_var_set_objecthandle(surgescript_var_create(), entity_handle);
}

/* get the entity with the given id */
surgescript_var_t* fun_entity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    const char* entity_id = surgescript_var_fast_get_string(param[0]);
    surgescript_objecthandle_t entity_handle = entitymanager_find_entity_by_id(object, str_to_x64(entity_id));
    surgescript_objecthandle_t null_handle = surgescript_objectmanager_null(manager);
    surgescript_var_t* ret = surgescript_var_create();

    if(entity_handle == null_handle)
        return surgescript_var_set_null(ret);
    else
        return surgescript_var_set_objecthandle(ret, entity_handle);
}

/* get the id of the given entity */
surgescript_var_t* fun_entityid(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objecthandle_t entity_handle = surgescript_var_get_objecthandle(param[0]);
    surgescript_var_t* ret = surgescript_var_create();

    if(entitymanager_has_entity_info(object, entity_handle)) {
        /* return the ID */
        char buffer[32];
        uint64_t entity_id = entitymanager_get_entity_id(object, entity_handle);
        return surgescript_var_set_string(ret, x64_to_str(entity_id, buffer, sizeof(buffer)));
    }
    else {
        /* ID not found */
        return surgescript_var_set_string(ret, "");
    }
}

/* find by name an entity that was spawned with this.spawnEntity() */
surgescript_var_t* fun_findentity(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* we first check if the object exists and if it's an entity
       it it passes those tests, then we call this.findObject() */
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_tagsystem_t* tag_system = surgescript_objectmanager_tagsystem(manager);
    const char* object_name = surgescript_var_fast_get_string(param[0]);
    surgescript_var_t* ret = surgescript_var_create();

    if(
        surgescript_objectmanager_class_exists(manager, object_name) &&
        surgescript_tagsystem_has_tag(tag_system, object_name, "entity")
    ) {
        /* find the entity down the object tree */
        surgescript_object_call_function(object, "findObject", param, 1, ret);
        return ret; /* will be null if no such entity is found */
    }
    else {
        /* the object doesn't exist or is not an entity */
        return surgescript_var_set_null(ret);
    }
}

/* find all entities with the given name that were spawned with this.spawnEntity() */
surgescript_var_t* fun_findentities(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* we first check if the objects exist and if they're entities
       it they pass those tests, then we call this.findObjects() */
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_tagsystem_t* tag_system = surgescript_objectmanager_tagsystem(manager);
    const char* object_name = surgescript_var_fast_get_string(param[0]);
    surgescript_var_t* ret = surgescript_var_create();

    if(
        surgescript_objectmanager_class_exists(manager, object_name) &&
        surgescript_tagsystem_has_tag(tag_system, object_name, "entity")
    ) {
        /* find the entities down the object tree */
        surgescript_object_call_function(object, "findObjects", param, 1, ret);
        return ret; /* will be a new empty array if no such entities are found */
    }
    else {
        /* the object doesn't exist or is not an entity */
        surgescript_objecthandle_t empty_array = surgescript_objectmanager_spawn_array(manager);
        return surgescript_var_set_objecthandle(ret, empty_array);
    }
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

/* add an entity to the late update queue */
surgescript_var_t* fun_addtolateupdatequeue(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    entitydb_t* db = get_db(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[0]);

    darray_push(db->late_update_queue, handle);

    return NULL;
}

/* consume the late update queue */
surgescript_var_t* fun_consumelateupdatequeue(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    entitydb_t* db = get_db(object);

    /* for each entity in the late update queue, call entity.lateUpdate() */
    for(int i = 0; i < darray_length(db->late_update_queue); i++) {
        surgescript_objecthandle_t entity_handle = db->late_update_queue[i];
        if(surgescript_objectmanager_exists(manager, entity_handle)) { /* validity check */
            surgescript_object_t* entity = surgescript_objectmanager_get(manager, entity_handle);
            if(surgescript_object_is_active(entity) && !surgescript_object_is_killed(entity)) {
                surgescript_object_call_function(entity, "lateUpdate", NULL, 0, NULL);
            }
        }
    }

    /* clear the queue */
    darray_clear(db->late_update_queue);

    /* done! */
    return NULL;
}

/* add a brick-like object */
surgescript_var_t* fun_addbricklikeobject(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    entitydb_t* db = get_db(object);
    surgescript_objecthandle_t handle = surgescript_var_get_objecthandle(param[0]);

    darray_push(db->bricklike_objects, handle);

    return NULL;
}

/* update the sub-tree in which I am the root */
surgescript_var_t* fun_update(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    surgescript_objectmanager_t* manager = surgescript_object_manager(object);
    surgescript_heap_t* heap = surgescript_object_heap(object);

    /* update unawake container */
    surgescript_var_t* unawake_container_var = surgescript_heap_at(heap, UNAWAKEENTITYCONTAINER_ADDR);
    surgescript_objecthandle_t unawake_container_handle = surgescript_var_get_objecthandle(unawake_container_var);
    surgescript_object_t* unawake_container = surgescript_objectmanager_get(manager, unawake_container_handle);
    surgescript_object_traverse_tree(unawake_container, update_subtree);

    /* update awake container */
    surgescript_var_t* awake_container_var = surgescript_heap_at(heap, AWAKEENTITYCONTAINER_ADDR);
    surgescript_objecthandle_t awake_container_handle = surgescript_var_get_objecthandle(awake_container_var);
    surgescript_object_t* awake_container = surgescript_objectmanager_get(manager, awake_container_handle);
    surgescript_object_traverse_tree(awake_container, update_subtree);

    /* late update */
    fun_consumelateupdatequeue(object, NULL, 0);

    /* done */
    return NULL;
}

/* render the entities */
surgescript_var_t* fun_render(surgescript_object_t* object, const surgescript_var_t** param, int num_params)
{
    /* TODO */
    return NULL;
}


/*
 *
 * Helpers
 *
 */

/* update cycle */
bool update_subtree(surgescript_object_t* object)
{
    surgescript_object_call_current_state(object);
    return surgescript_object_is_active(object);
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
    entitydb_t* db = get_db(entity_manager);

    /* the rationale for caching is that we tend to make multiple
       queries related to the same entity sequentially in time */
    if(db->cached_query->handle != entity_handle) {
        entityinfo_t* info = get_info(db, entity_handle); /* NULL if not found */
        return info != NULL ? (db->cached_query = info) : NULL;
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
    entityinfo_t* info = quick_lookup(entity_manager, entity_handle);

    if(info != NULL) {
        entitydb_t* db = get_db(entity_manager);
        uint64_t entity_id = info->id;

        fasthash_delete(db->id_to_handle, entity_id);
        fasthash_delete(db->info, entity_handle);
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

    if(info != NULL) {
        entitydb_t* db = get_db(entity_manager);

        /* update the id_to_handle hashtable */
        fasthash_delete(db->id_to_handle, info->id);
        fasthash_put(db->id_to_handle, entity_id, handle_ctor(info->handle));

        /* set the new id */
        info->id = entity_id;
    }
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

/* find entity by ID. This may return a null handle! */
surgescript_objecthandle_t entitymanager_find_entity_by_id(surgescript_object_t* entity_manager, uint64_t entity_id)
{
    entitydb_t* db = get_db(entity_manager);
    surgescript_objecthandle_t* handle_ptr = (surgescript_objecthandle_t*)fasthash_get(db->id_to_handle, entity_id);
    surgescript_objectmanager_t* manager = surgescript_object_manager(entity_manager);

    if(handle_ptr == NULL) {
        /* ID not found */
        return surgescript_objectmanager_null(manager);
    }
    else if(!surgescript_objectmanager_exists(manager, *handle_ptr)) {
        /* the entity no longer exists */
        entitymanager_remove_entity_info(entity_manager, *handle_ptr);
        return surgescript_objectmanager_null(manager);
    }
    else {
        /* success! */
        return *handle_ptr;
    }
}