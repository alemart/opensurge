/*
 * Open Surge Engine
 * editorgrp.c - level editor: groups
 * Copyright (C) 2010, 2012  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdio.h>
#include <stdlib.h>
#include "editorgrp.h"
#include "../../entities/brick.h"
#include "../../entities/item.h"
#include "../../entities/enemy.h"
#include "../../core/logfile.h"
#include "../../core/osspec.h"
#include "../../core/util.h"
#include "../../core/stringutil.h"
#include "../../core/nanoparser/nanoparser.h"

/* internal data */
#define EDITORGRP_MAX_GROUPS        501
static int group_count;
static editorgrp_entity_list_t* group[EDITORGRP_MAX_GROUPS];
static editorgrp_entity_list_t* delete_list(editorgrp_entity_list_t *list);
static editorgrp_entity_list_t* add_to_list(editorgrp_entity_list_t *list, editorgrp_entity_t entity);
static int traverse(const parsetree_statement_t *stmt);
static int traverse_group(const parsetree_statement_t *stmt, void *entity_list);

/* imported methods from the editor */
extern int editor_is_valid_item(int item_id);
extern int editor_enemy_name2key(const char *name);
extern const char* editor_enemy_key2name(int key);


/*
 * editorgrp_init()
 * Initializes this module
 */
void editorgrp_init()
{
    int i;

    group_count = 0;
    for(i=0; i<EDITORGRP_MAX_GROUPS; i++)
        group[i] = NULL;
}


/*
 * editorgrp_release()
 * Releases this module
 */
void editorgrp_release()
{
    int i;

    for(i=0; i<group_count; i++)
        group[i] = delete_list(group[i]);
    group_count = 0;
}


/*
 * editorgrp_load_from_file()
 * Reads a list of groups from a file
 */
void editorgrp_load_from_file(const char *filename)
{
    char abs_path[1024];
    parsetree_program_t *prog;

    resource_filepath(abs_path, filename, sizeof(abs_path), RESFP_READ);
    logfile_message("editorgrp_load_from_file('%s')", filename);

    prog = nanoparser_construct_tree(abs_path);
    nanoparser_traverse_program(prog, traverse);
    prog = nanoparser_deconstruct_tree(prog);

    logfile_message("editorgrp_load_from_file() loaded %d group(s)", group_count);
}

/*
 * editorgrp_group_count()
 * How many groups there are?
 */
int editorgrp_group_count()
{
    return group_count;
}


/*
 * editorgrp_get_group()
 * Returns a group, where 0 <= id < editorgrp_group_count()
 */
editorgrp_entity_list_t* editorgrp_get_group(int id)
{
    if(group_count > 0) {
        id = clip(id, 0, group_count-1);
        return group[id];
    }
    else
        return NULL;
}






/* internal methods */


/* deletes the given linked list */
editorgrp_entity_list_t* delete_list(editorgrp_entity_list_t *list)
{
    if(list != NULL) {
        list->next = delete_list(list->next);
        free(list);
        list = NULL;
    }

    return list;
}

/* adds an entity to a list */
editorgrp_entity_list_t* add_to_list(editorgrp_entity_list_t *list, editorgrp_entity_t entity)
{
    editorgrp_entity_list_t *p = mallocx(sizeof *p);
    p->entity = entity;
    p->next = list;
    return p;
}

/* traverses a .grp file */
int traverse(const parsetree_statement_t *stmt)
{
    const char *id;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *group_block;

    id = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(id, "group") == 0) {
        group_block = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(group_block, "A block is expected after the 'group' keyword");
        if(group_count < EDITORGRP_MAX_GROUPS) {
            editorgrp_entity_list_t *list = NULL;
            nanoparser_traverse_program_ex(nanoparser_get_program(group_block), (void*)(&list), traverse_group);
            group[ group_count++ ] = list;
        }
        else
            fatal_error("You can't have more than %d groups per level (group_count=%d)", EDITORGRP_MAX_GROUPS-1, group_count);
    }
    else
        fatal_error("Unexpected identifier: '%s' at the group file. Expected: 'group'", id);

    return 0;
}

/* traverses a group block */
int traverse_group(const parsetree_statement_t *stmt, void *entity_list)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2, *p3, *p4;
    editorgrp_entity_list_t **list = (editorgrp_entity_list_t**)entity_list;
    editorgrp_entity_t e;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "brick") == 0) {
        int id, x, y;
        bricklayer_t layer;

        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        p3 = nanoparser_get_nth_parameter(param_list, 3);
        p4 = nanoparser_get_nth_parameter(param_list, 4);

        nanoparser_expect_string(p1, "Brick id must be given");
        nanoparser_expect_string(p2, "Brick xpos must be given");
        nanoparser_expect_string(p3, "Brick ypos must be given");
        if(p4) nanoparser_expect_string(p4, "Brick layer is expected");

        id = atoi(nanoparser_get_string(p1));
        x = atoi(nanoparser_get_string(p2));
        y = atoi(nanoparser_get_string(p3));
        layer = p4 ? brick_util_layercode(nanoparser_get_string(p4)) : BRL_DEFAULT;

        e.type = EDITORGRP_ENTITY_BRICK;
        e.id = id;
        e.position = v2d_new(x,y);
        e.layer = (int)layer;
        if(brick_exists(e.id))
            *list = add_to_list(*list, e);
    }
    else if(str_icmp(identifier, "item") == 0) {
        int id;
        int x, y;

        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        p3 = nanoparser_get_nth_parameter(param_list, 3);

        nanoparser_expect_string(p1, "Item id must be given");
        nanoparser_expect_string(p2, "Item xpos must be given");
        nanoparser_expect_string(p3, "Item ypos must be given");

        id = atoi(nanoparser_get_string(p1));
        x = atoi(nanoparser_get_string(p2));
        y = atoi(nanoparser_get_string(p3));

        e.type = EDITORGRP_ENTITY_ITEM;
        e.id = clip(id, 0, ITEMDATA_MAX-1);
        e.position = v2d_new(x,y);
        e.layer = 0;
        if(editor_is_valid_item(e.id)) /* valid item? */
            *list = add_to_list(*list, e);
    }
    else if(str_icmp(identifier, "object") == 0 || str_icmp(identifier, "enemy") == 0) {
        const char *name;
        int x, y;

        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        p3 = nanoparser_get_nth_parameter(param_list, 3);

        nanoparser_expect_string(p1, "Object name must be given");
        nanoparser_expect_string(p2, "Object xpos must be given");
        nanoparser_expect_string(p3, "Object ypos must be given");

        name = nanoparser_get_string(p1);
        x = atoi(nanoparser_get_string(p2));
        y = atoi(nanoparser_get_string(p3));

        e.type = EDITORGRP_ENTITY_ENEMY;
        e.id = editor_enemy_name2key(name);
        e.position = v2d_new(x,y);
        e.layer = 0;
        *list = add_to_list(*list, e);
    }
    else
        fatal_error("Unexpected identifier '%s' at group definition. Valid keywords are: 'brick', 'item', 'object'", identifier);

    return 0;
}

