/*
 * Open Surge Engine
 * quest.c - quest module
 * Copyright (C) 2008-2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "global.h"
#include "image.h"
#include "util.h"
#include "stringutil.h"
#include "logfile.h"
#include "quest.h"
#include "osspec.h"
#include "nanoparser/nanoparser.h"



/* private data */
#define QUESTIMAGE_WIDTH 100
#define QUESTIMAGE_HEIGHT 75


/* private functions */
static image_t *load_quest_image(const char *file);
static int traverse_quest(const parsetree_statement_t* stmt, void *quest);



/*
 * load_quest()
 * Loads and returns a quest from a file
 * (abs_path must be an ABSOLUTE path)
 */
quest_t *load_quest(const char *abs_path)
{
    quest_t *q = mallocx(sizeof *q);
    parsetree_program_t *prog;

    logfile_message("load_quest('%s')", abs_path);

    /* default values */
    q->file = str_dup(abs_path);
    q->name = str_dup("null");
    q->author = str_dup("null");
    q->version = str_dup("null");
    q->description = str_dup("null");
    q->image = NULL;
    q->level_count = 0;
    q->is_hidden = FALSE;

    /* reading the quest */
    prog = nanoparser_construct_tree(abs_path);
    nanoparser_traverse_program_ex(prog, (void*)q, traverse_quest);
    prog = nanoparser_deconstruct_tree(prog);

    /* no image given? */
    if(q->image == NULL)
        q->image = load_quest_image(NULL);

    /* success! */
    logfile_message("load_quest() ok!");
    return q;
}




/*
 * unload_quest()
 * Unloads a quest
 */
quest_t *unload_quest(quest_t *qst)
{
    int i;

    free(qst->file);
    free(qst->name);
    free(qst->author);
    free(qst->version);
    free(qst->description);

    for(i=0; i<qst->level_count; i++)
        free(qst->level_path[i]);

    image_destroy(qst->image);
    free(qst);
    return NULL;
}




/* private functions */

/* returns the quest image */
image_t *load_quest_image(const char *file)
{
    char no_image[] = "images/null.png";
    const char *s = file ? file : no_image;
    image_t *ret, *img;

    img = image_load(s);
    if(img == NULL) {
        s = no_image;
        img = image_load(s);
    }

    ret = image_create(QUESTIMAGE_WIDTH, QUESTIMAGE_HEIGHT);
    image_blit(img, ret, 0, 0, 0, 0, image_width(ret), image_height(ret));
    image_unref(s);

    return ret;
}

/* interprets a statement from a .qst file */
int traverse_quest(const parsetree_statement_t* stmt, void *quest)
{
    quest_t *q = (quest_t*)quest;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);
    const parsetree_parameter_t* p = nanoparser_get_nth_parameter(param_list, 1);

    if(str_icmp(id, "name") == 0) {
        nanoparser_expect_string(p, "Quest loader: quest name is expected");
        free(q->name);
        q->name = str_dup(nanoparser_get_string(p));
    }
    else if(str_icmp(id, "author") == 0) {
        nanoparser_expect_string(p, "Quest loader: quest author is expected");
        free(q->author);
        q->author = str_dup(nanoparser_get_string(p));
    }
    else if(str_icmp(id, "version") == 0) {
        nanoparser_expect_string(p, "Quest loader: quest version is expected");
        free(q->version);
        q->version = str_dup(nanoparser_get_string(p));
    }
    else if(str_icmp(id, "description") == 0) {
        nanoparser_expect_string(p, "Quest loader: quest description is expected");
        free(q->description);
        q->description = str_dup(nanoparser_get_string(p));
    }
    else if(str_icmp(id, "image") == 0) {
        nanoparser_expect_string(p, "Quest loader: quest image is expected");
        if(q->image) image_destroy(q->image);
        q->image = load_quest_image(nanoparser_get_string(p));
    }
    else if(str_icmp(id, "hidden") == 0) {
        q->is_hidden = TRUE;
    }
    else if(str_icmp(id, "level") == 0) {
        nanoparser_expect_string(p, "Quest loader: expected level path");
        if(q->level_count < QUEST_MAXLEVELS)
            q->level_path[q->level_count++] = str_dup(nanoparser_get_string(p));
        else
            fatal_error("Quest loader: quests can't have more than %d levels", QUEST_MAXLEVELS);
    }
    else if(id[0] == '<' && id[strlen(id)-1] == '>') { /* special */
        if(q->level_count < QUEST_MAXLEVELS)
            q->level_path[q->level_count++] = str_dup(id);
        else
            fatal_error("Quest loader: q->level_count >= (QUEST_MAXLEVELS = %d)", QUEST_MAXLEVELS);
    }

    return 0;
}
