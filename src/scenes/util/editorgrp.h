/*
 * Open Surge Engine
 * editorgrp.h - level editor: groups
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
 * http://opensnc.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _EDITORGRP_H
#define _EDITORGRP_H

#include "../../core/v2d.h"

/* entity types */
enum editorgrp_entity_type {
    EDITORGRP_ENTITY_BRICK,
    EDITORGRP_ENTITY_ITEM,
    EDITORGRP_ENTITY_ENEMY
};

/* a single entity of a level editor group */
typedef struct {
    enum editorgrp_entity_type type; /* object type */
    int id; /* object id */
    v2d_t position; /* position */
    int layer; /* layer */
} editorgrp_entity_t;

/* linked list of entities */
typedef struct editorgrp_entity_list_t {
    editorgrp_entity_t entity;
    struct editorgrp_entity_list_t *next;
} editorgrp_entity_list_t;

/* public methods */
void editorgrp_init(); /* initializes this module */
void editorgrp_release(); /* releases this module */
void editorgrp_load_from_file(const char *filename); /* reads a list of groups from a file */
int editorgrp_group_count(); /* how many groups there are? */
editorgrp_entity_list_t* editorgrp_get_group(int id); /* returns a group, where 0 <= id < editorgrp_group_count() */

#endif
