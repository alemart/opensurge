/*
 * Open Surge Engine
 * editorgrp.c - level editor: groups
 * Copyright 2008-2026 Alexandre Martins <alemartf(at)gmail.com>
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
#include "../../entities/legacy/item.h"
#include "../../entities/legacy/enemy.h"
#include "../../core/logfile.h"
#include "../../core/asset.h"
#include "../../core/nanoparser.h"
#include "../../util/util.h"
#include "../../util/stringutil.h"

/* internal data */
#define EDITORGRP_MAX_GROUPS        512
static int group_count;
static editorgrp_entity_list_t* group[EDITORGRP_MAX_GROUPS];
static editorgrp_entity_list_t* delete_list(editorgrp_entity_list_t *list);
static editorgrp_entity_list_t* add_to_list(editorgrp_entity_list_t *list, editorgrp_entity_t entity);
static void read_from_file(const char *filename);
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
void editorgrp_init(const char* grpfile)
{
    int i;

    group_count = 0;
    for(i=0; i<EDITORGRP_MAX_GROUPS; i++)
        group[i] = NULL;

    if(*grpfile != '\0') /* grpfile may be empty */
        read_from_file(grpfile);
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

/* reads a list of groups from a file */
void read_from_file(const char *filename)
{
    parsetree_program_t *prog;
    const char* fullpath = asset_path(filename);

    logfile_message("Loading group file \"%s\"...", filename);

    prog = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program(prog, traverse);
    prog = nanoparser_deconstruct_tree(prog);

    logfile_message("Loaded %d group%s", group_count, group_count != 1 ? "s" : "");
}



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
            fatal_error("You can't have more than %d groups per level (group_count=%d)", EDITORGRP_MAX_GROUPS, group_count);
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
    const parsetree_parameter_t *p1, *p2, *p3, *p4, *p5;
    editorgrp_entity_list_t **list = (editorgrp_entity_list_t**)entity_list;
    editorgrp_entity_t e;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "brick") == 0) {
        int id, x, y;
        brickflip_t flip = BRF_NOFLIP;
        bricklayer_t layer = BRL_DEFAULT;
        int n = nanoparser_get_number_of_parameters(param_list);

        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);
        p3 = nanoparser_get_nth_parameter(param_list, 3);

        nanoparser_expect_string(p1, "Brick id must be given");
        nanoparser_expect_string(p2, "Brick xpos must be given");
        nanoparser_expect_string(p3, "Brick ypos must be given");

        id = atoi(nanoparser_get_string(p1));
        x = atoi(nanoparser_get_string(p2));
        y = atoi(nanoparser_get_string(p3));

        if(n >= 4) {
            p4 = nanoparser_get_nth_parameter(param_list, 4);
            nanoparser_expect_string(p4, "Brick layer is expected");
            layer = brick_util_layercode(nanoparser_get_string(p4));

            if(n >= 5) {
                p5 = nanoparser_get_nth_parameter(param_list, 5);
                nanoparser_expect_string(p5, "Brick flip flags is expected");
                flip = brick_util_flipcode(nanoparser_get_string(p5));
            }
        }

        e.type = EDITORGRP_ENTITY_BRICK;
        e.id = id;
        e.position = v2d_new(x,y);
        e.layer = layer;
        e.flip = flip;
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
        e.flip = 0;
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
        e.flip = 0;
        *list = add_to_list(*list, e);
    }
    else
        fatal_error("Unexpected identifier '%s' at group definition. Valid keywords are: 'brick'", identifier);

    return 0;
}

