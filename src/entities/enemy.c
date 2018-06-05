/*
 * Open Surge Engine
 * enemy.c - baddies
 * Copyright (C) 2008-2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
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

#include <string.h>
#include <math.h>
#include "../core/global.h"
#include "../core/util.h"
#include "../core/audio.h"
#include "../core/timer.h"
#include "../core/input.h"
#include "../core/logfile.h"
#include "../core/stringutil.h"
#include "../core/osspec.h"
#include "../core/nanocalcext.h"
#include "../core/video.h"
#include "../core/hashtable.h"
#include "../core/nanoparser/nanoparser.h"
#include "../scenes/level.h"
#include "actor.h"
#include "player.h"
#include "enemy.h"
#include "object_vm.h"
#include "object_compiler.h"
#include "item.h"
#include "brick.h"

/* private stuff */
#define MAX_OBJECTS                     10240
#define MAX_CATEGORIES                  10240
#define ROOT_CATEGORY                   category_table.category[0] /* all objects belong to the root category */

typedef parsetree_program_t objectcode_t;
HASHTABLE_GENERATE_CODE(objectcode_t);

typedef struct object_children_t object_children_t;
struct object_children_t {
    char *name;
    enemy_t *data;
    object_children_t *next;
};
static object_children_t* object_children_new();
static object_children_t* object_children_delete(object_children_t* list);
static object_children_t* object_children_add(object_children_t* list, const char *name, enemy_t *data);
static object_children_t* object_children_remove(object_children_t* list, enemy_t *data);
static object_t* object_children_find(object_children_t* list, const char *name);
static void object_children_visitall(object_children_t* list, void *any_data, void (*fun)(enemy_t*,void*));

typedef struct { const char* name[MAX_OBJECTS]; int length; } object_name_data_t;
typedef struct { const char* category[MAX_CATEGORIES]; int length; } object_category_data_t;
static int object_name_table_cmp(const void *a, const void *b);
static int object_category_table_cmp(const void *a, const void *b);

static enemy_t* create_from_script(const char *object_name);
static int fill_object_names(const parsetree_statement_t *stmt, void *object_name_data);
static int fill_object_categories(const parsetree_statement_t *stmt, void *object_category_data);
static int fill_lookup_table(const parsetree_statement_t *stmt, void *lookup_table);
static int prepare_to_fill_object_categories(const parsetree_statement_t *stmt, void *object_category_data);
static int dirfill(const char *filename, void *param); /* file system callback */
static int is_hidden_object(const char *name);
static int category_exists(const char *category);

static parsetree_program_t *objects;
static object_name_data_t name_table;
static object_category_data_t category_table;
static hashtable_objectcode_t* lookup_table;


/* ------ public class methods ---------- */

/*
 * objects_init()
 * Initializes this module
 */
void objects_init()
{
    const char *path = "objects/*.obj";

    logfile_message("Loading objects scripts...");
    objects = NULL;

    /* reading the parse tree */
    foreach_resource(path, dirfill, (void*)(&objects), TRUE);

    /* creating the name table */
    name_table.length = 0;
    nanoparser_traverse_program_ex(objects, (void*)(&name_table), fill_object_names);
    qsort(name_table.name, name_table.length, sizeof(name_table.name[0]), object_name_table_cmp);

    /* creating the category table */
    ROOT_CATEGORY = "*"; /* name of the root category */
    category_table.length = 1; /* the length of the table is at least one, as it contains the root category */
    nanoparser_traverse_program_ex(objects, (void*)(&category_table), prepare_to_fill_object_categories);
    qsort(category_table.category, category_table.length, sizeof(category_table.category[0]), object_category_table_cmp);

    /* creating a lookup table to find objects fast */
    lookup_table = hashtable_objectcode_t_create(NULL);
    nanoparser_traverse_program_ex(objects, (void*)lookup_table, fill_lookup_table);

    /* done! */
    logfile_message("All objects have been loaded!");
}

/*
 * objects_release()
 * Releases this module
 */
void objects_release()
{
    lookup_table = hashtable_objectcode_t_destroy(lookup_table);
    name_table.length = category_table.length = 0;
    objects = nanoparser_deconstruct_tree(objects);
}

/*
 * objects_get_list_of_names()
 * Returns an array v[0..n-1] of available object names
 */
const char** objects_get_list_of_names(int *n)
{
    *n = name_table.length;
    return name_table.name;
}


/*
 * objects_get_list_of_categories()
 * Returns an array v[0..n-1] of available object categories
 */
const char** objects_get_list_of_categories(int *n)
{
    *n = category_table.length;
    return category_table.category;
}






/* ------ public instance methods ------- */


/*
 * enemy_create()
 * Creates a new enemy
 */
enemy_t *enemy_create(const char *name)
{
    return create_from_script(name);
}


/*
 * enemy_destroy()
 * Destroys an enemy
 */
enemy_t *enemy_destroy(enemy_t *enemy)
{
    object_children_t *it;

    /* tell my children I died */
    for(it=enemy->children; it; it=it->next)
        it->data->parent = NULL;

    /* destroy my children list */
    object_children_delete(enemy->children);

    /* tell my parent I died */
    if(enemy->parent != NULL)
        enemy_remove_child(enemy->parent, enemy);

    /* destroy my virtual machine */
    objectvm_destroy(enemy->vm);

    /* destroy my categories */
    if(enemy->category != NULL)
        free(enemy->category);

    /* destroy me */
    actor_destroy(enemy->actor);
    free(enemy->name);
    free(enemy);

    /* success */
    return NULL;
}


/*
 * enemy_update()
 * Runs every cycle of the game to update an enemy
 */
void enemy_update(enemy_t *enemy, player_t **team, int team_size, brick_list_t *brick_list, item_list_t *item_list, enemy_list_t *object_list)
{
    objectmachine_t *machine = *(objectvm_get_reference_to_current_state(enemy->vm));
    if(enemy->state == ES_DEAD) return;
    nanocalcext_set_target_object((object_t*)enemy, brick_list, item_list, object_list);
    machine->update(machine, team, team_size, brick_list, item_list, object_list);
}



/*
 * enemy_render()
 * Renders an enemy
 */
void enemy_render(enemy_t *enemy, v2d_t camera_position)
{
    objectmachine_t *machine = *(objectvm_get_reference_to_current_state(enemy->vm));
    if(enemy->state == ES_DEAD) return;
    if(!enemy->hide_unless_in_editor_mode || (enemy->hide_unless_in_editor_mode && level_editmode())) {
        if(!enemy->detach_from_camera || (enemy->detach_from_camera && level_editmode()))
            machine->render(machine, camera_position);
        else
            machine->render(machine, v2d_new(VIDEO_SCREEN_W/2, VIDEO_SCREEN_H/2));
    }
}


/*
 * enemy_get_parent()
 * Finds the parent of this object
 */
enemy_t *enemy_get_parent(enemy_t *enemy)
{
    return enemy->parent;
}


/*
 * enemy_get_child()
 * Finds a child of this object
 */
enemy_t *enemy_get_child(enemy_t *enemy, const char *child_name)
{
    return object_children_find(enemy->children, child_name);
}


/*
 * enemy_add_child()
 * Adds a child to this object
 */
void enemy_add_child(enemy_t *enemy, const char *child_name, enemy_t *child)
{
    enemy->children = object_children_add(enemy->children, child_name, child);
    child->parent = enemy;
    child->created_from_editor = FALSE;
}

/*
 * enemy_remove_child()
 * Removes a child from this object (the child is not deleted, though)
 */
void enemy_remove_child(enemy_t *enemy, enemy_t *child)
{
    enemy->children = object_children_remove(enemy->children, child);
}

/*
 * enemy_visit_children()
 * Calls fun for each of the children of the object. any_data is anything you want.
 */
void enemy_visit_children(enemy_t *enemy, void *any_data, void (*fun)(enemy_t*,void*))
{
    object_children_visitall(enemy->children, any_data, fun);
}


/*
 * enemy_get_observed_player()
 * returns the observed player
 */
player_t *enemy_get_observed_player(enemy_t *enemy)
{
    return enemy->observed_player != NULL ? enemy->observed_player : level_player();
}

/*
 * enemy_observe_player()
 * observes a new player
 */
void enemy_observe_player(enemy_t *enemy, player_t *player)
{
    enemy->observed_player = player;
}

/*
 * enemy_observe_current_player()
 * observes the current player
 */
void enemy_observe_current_player(enemy_t *enemy)
{
    enemy->observed_player = level_player();
}

/*
 * enemy_observe_active_player()
 * observes the active player
 */
void enemy_observe_active_player(enemy_t *enemy)
{
    enemy->observed_player = NULL;
}

/*
 * enemy_belongs_to_category()
 * checks if a given object belongs to a category
 */
int enemy_belongs_to_category(enemy_t *enemy, const char *category)
{
    if(str_icmp(category, ROOT_CATEGORY) != 0) {
        int i;

        for(i=0; i<enemy->category_count; i++) {
            if(str_icmp(enemy->category[i], category) == 0)
                return TRUE;
        }

        return FALSE;
    }
    else
        return TRUE;
}



/* ----------- private functions ----------- */

enemy_t* create_from_script(const char *object_name)
{
    enemy_t* e = mallocx(sizeof *e);
    objectcode_t *object_code;

    /* setup the object */
    e->name = str_dup(object_name);
    e->annotation = "";
    e->category = NULL;
    e->category_count = 0;

    e->state = ES_IDLE;
    e->zindex = 0.5f;
    e->actor = actor_create();
    e->actor->input = input_create_computer();
    actor_change_animation(e->actor, sprite_get_animation("SD_QUESTIONMARK", 0));
    e->preserve = TRUE;
    e->obstacle = FALSE;
    e->obstacle_angle = 0;
    e->always_active = FALSE;
    e->hide_unless_in_editor_mode = FALSE;
    e->detach_from_camera = FALSE;
    e->vm = objectvm_create(e);
    e->created_from_editor = TRUE;
    e->parent = NULL;
    e->children = object_children_new();
    e->observed_player = NULL;
    e->attached_to_player = FALSE;
    e->attached_to_player_offset = v2d_new(0,0);

    /* Let's compile the object */
    object_code = hashtable_objectcode_t_find(lookup_table, object_name);
    if(object_code != NULL)
        objectcompiler_compile(e, object_code);
    else
        fatal_error("Can't spawn object '%s': it does not exist!", object_name);

    /* success! */
    return e;
}

int is_hidden_object(const char *name)
{
    return name[0] == '.';
}

int category_exists(const char *category)
{
    int i;

    for(i=0; i<category_table.length; i++) {
        if(str_icmp(category_table.category[i], category) == 0)
            return TRUE;
    }

    return FALSE;
}


int fill_object_names(const parsetree_statement_t* stmt, void *object_name_data)
{
    object_name_data_t *x = (object_name_data_t*)object_name_data;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "object") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Object script error: object name is expected");
        if(x->length < MAX_OBJECTS) {
            const char *name = nanoparser_get_string(p1);
            if(!is_hidden_object(name))
                (x->name)[ (x->length)++ ] = name;
        }
        else
            fatal_error("Object script error: can't have more than %d objects", MAX_OBJECTS);
    }
    else
        fatal_error("Object script error: unknown keyword '%s'\nin \"%s\" near line %d", id, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

int prepare_to_fill_object_categories(const parsetree_statement_t* stmt, void *object_category_data)
{
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "object") == 0) {
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t *p2 = nanoparser_get_nth_parameter(param_list, 2);
        const char *name;
        const parsetree_program_t *code;

        nanoparser_expect_string(p1, "Object script error: object code is expected");
        name = nanoparser_get_string(p1);

        nanoparser_expect_program(p2, "Object script error: object code is expected");
        code = nanoparser_get_program(p2);

        if(!is_hidden_object(name))
            nanoparser_traverse_program_ex(code, object_category_data, fill_object_categories);
    }

    return 0;
}

int fill_object_categories(const parsetree_statement_t* stmt, void *object_category_data)
{
    object_category_data_t *x = (object_category_data_t*)object_category_data;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "category") == 0) {
        int n = nanoparser_get_number_of_parameters(param_list);
        if(n > 0) {
            if(x->length + n < MAX_CATEGORIES) {
                int i;
                for(i=1; i<=n; i++) {
                    const parsetree_parameter_t *param = nanoparser_get_nth_parameter(param_list, i);
                    const char *category;

                    nanoparser_expect_string(param, "Object script error: object category is expected");
                    category = nanoparser_get_string(param);

                    if(!category_exists(category))
                        (x->category)[ (x->length)++ ] = category;
                }
            }
            else
                fatal_error("Object script error: can't have more than %d categories\nin \"%s\" near line %d", MAX_CATEGORIES, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
        }
        else
            fatal_error("Object script error: empty 'category' field\nin \"%s\" near line %d", nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
    }

    return 0;
}


int fill_lookup_table(const parsetree_statement_t *stmt, void *lookup_table)
{
    hashtable_objectcode_t* table = (hashtable_objectcode_t*)lookup_table;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "object") == 0) {
        const char *object_name;
        const parsetree_program_t *object_code;
        const parsetree_parameter_t *p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t *p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "Object script error: object name is expected (first parameter)");
        nanoparser_expect_program(p2, "Object script error: object code is expected (second parameter)");
        object_name = nanoparser_get_string(p1);
        object_code = nanoparser_get_program(p2);

        if(hashtable_objectcode_t_find(table, object_name) == NULL)
            hashtable_objectcode_t_add(table, object_name, (objectcode_t*)object_code);
        else
            fatal_error("Object script error: duplicate definition of the object \"%s\"\nin \"%s\" near line %d", object_name, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
    }

    return 0;
}


int object_name_table_cmp(const void *a, const void *b)
{
    const char *i = *((const char**)a);
    const char *j = *((const char**)b);
    return str_icmp(i, j);
}


int object_category_table_cmp(const void *a, const void *b)
{
    const char *i = *((const char**)a);
    const char *j = *((const char**)b);
    return str_icmp(i, j);
}

int dirfill(const char *filename, void *param)
{
    parsetree_program_t** p = (parsetree_program_t**)param;
    *p = nanoparser_append_program(*p, nanoparser_construct_tree(filename));
    return 0;
}




/* ------------------------------------- */



object_children_t* object_children_new()
{
    return NULL;
}

object_children_t* object_children_delete(object_children_t* list)
{
    if(list != NULL) {
        object_children_delete(list->next);
        free(list->name);
        free(list);
    }

    return NULL;
}

object_children_t* object_children_add(object_children_t* list, const char *name, enemy_t *data)
{
    object_children_t *x = mallocx(sizeof *x);
    x->name = str_dup(name);
    x->data = data;
    x->next = list;
    return x;
}

object_t* object_children_find(object_children_t* list, const char *name)
{
    object_children_t *it = list;

    while(it != NULL) {
        if(strcmp(it->name, name) == 0)
            return it->data;
        else
            it = it->next;
    }

    return NULL;
}

object_children_t* object_children_remove(object_children_t* list, enemy_t *data)
{
    object_children_t *it, *next;

    if(list != NULL) {
        if(list->data == data) {
            next = list->next;
            free(list->name);
            free(list);
            return next;
        }
        else {
            it = list;
            while(it->next != NULL && it->next->data != data)
                it = it->next;
            if(it->next != NULL) {
                next = it->next->next;
                free(it->next->name);
                free(it->next);
                it->next = next;
            }
            return list;
        }
    }
    else
        return NULL;
}

void object_children_visitall(object_children_t* list, void *any_data, void (*fun)(enemy_t*,void*))
{
    object_children_t *it = list;

    for(; it; it = it->next)
        fun(it->data, any_data);
}
