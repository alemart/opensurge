/*
 * Open Surge Engine
 * quest.c - quest module
 * Copyright (C) 2008-2010, 2018  Alexandre Martins <alemartf@gmail.com>
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
#include <string.h>
#include <ctype.h>
#include "global.h"
#include "image.h"
#include "util.h"
#include "stringutil.h"
#include "logfile.h"
#include "quest.h"
#include "assetfs.h"
#include "nanoparser/nanoparser.h"



/* private stuff */
static image_t *load_quest_image(const char *image_file);
static int traverse_quest(const parsetree_statement_t* stmt, void *quest);
static const char* DEFAULT_QUEST_IMAGE = "images/null.png";
static const int QUEST_IMAGE_WIDTH = 100;
static const int QUEST_IMAGE_HEIGHT = 75;



/*
 * quest_load()
 * Loads the quest data from a file
 */
quest_t *quest_load(const char *filepath)
{
    quest_t *q = mallocx(sizeof *q);
    parsetree_program_t *prog;
    const char* fullpath;

    logfile_message("Loading quest \"%s\"...", filepath);
    fullpath = assetfs_fullpath(filepath);

    /* default values */
    q->file = str_dup(filepath);
    q->name = str_dup("-");
    q->author = str_dup("-");
    q->version = str_dup("-");
    q->description = str_dup("-");
    q->image = load_quest_image(NULL);
    q->level_count = 0;
    q->is_hidden = FALSE;

    /* reading the quest */
    prog = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program_ex(prog, (void*)q, traverse_quest);
    prog = nanoparser_deconstruct_tree(prog);

    /* success! */
    logfile_message("Quest \"%s\" has been loaded successfully!", q->name);
    return q;
}




/*
 * quest_unload()
 * Unload quest data
 */
quest_t *quest_unload(quest_t *qst)
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
image_t *load_quest_image(const char *image_file)
{
    const char *src = image_file ? image_file : DEFAULT_QUEST_IMAGE;
    image_t *ret, *img;

    img = image_load(src);
    if(img == NULL)
        img = image_load(DEFAULT_QUEST_IMAGE);

    ret = image_clone_region(img, 0, 0, QUEST_IMAGE_WIDTH, QUEST_IMAGE_HEIGHT);
    image_unload(img);
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
        image_destroy(q->image);
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
            fatal_error("Quest loader: quests can't have more than %d levels", QUEST_MAXLEVELS);
    }

    return 0;
}
