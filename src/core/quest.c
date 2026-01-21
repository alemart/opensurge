/*
 * Open Surge Engine
 * quest.c - quest module
 * Copyright 2008-2025 Alexandre Martins <alemartf(at)gmail.com>
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
#include <stdbool.h>
#include "global.h"
#include "logfile.h"
#include "quest.h"
#include "asset.h"
#include "nanoparser.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../util/darray.h"


/* quest structure */
struct quest_t {
    char* file; /* relative path of the quest file */
    char* name; /* quest name */
    DARRAY(char*, entry); /* entries / relative paths */
};

/* private stuff */
static int traverse_quest(const parsetree_statement_t* stmt, void *quest);
static quest_t* create_single_level_quest(const char* path_to_lev_file);



/*
 * quest_load()
 * Loads quest data from a file
 */
quest_t *quest_load(const char *filepath)
{
    quest_t* q = NULL;

    logfile_message("Loading quest \"%s\"...", filepath);

    /* reading the quest */
    if(str_pathhasextension(filepath, ".qst")) {

        /* create a new quest with default values */
        q = mallocx(sizeof *q);
        q->file = str_dup(filepath);
        q->name = str_dup(filepath); /* use the filepath as the default name */
        darray_init(q->entry);

        /* read quest file */
        const char* fullpath = asset_path(filepath);
        parsetree_program_t* prog = nanoparser_construct_tree(fullpath);
        nanoparser_traverse_program_ex(prog, q, traverse_quest);
        nanoparser_deconstruct_tree(prog);

    }
    else if(str_pathhasextension(filepath, ".lev")) {

        /* implicitly create a quest with a single level */
        q = create_single_level_quest(filepath);

    }
    else {

        /* not a quest file */
        q = NULL;

    }

    /* validate */
    if(q == NULL)
        fatal_error("Can't load quest file \"%s\"", filepath);
    else if(darray_length(q->entry) == 0)
        fatal_error("Quest \"%s\" has no entries", q->name);

    /* success! */
    logfile_message("Quest \"%s\" has been loaded successfully!", q->name);
    return q;
}

/*
 * quest_unload()
 * Unloads quest data
 */
quest_t *quest_unload(quest_t *quest)
{
    for(int i = darray_length(quest->entry) - 1; i >= 0; i--)
        free(quest->entry[i]);

    darray_release(quest->entry);

    free(quest->name);
    free(quest->file);

    free(quest);
    return NULL;
}

/*
 * quest_name()
 * The name of the quest
 */
const char* quest_name(const quest_t* quest)
{
    return quest->name;
}

/*
 * quest_file()
 * The relative filepath of the .qst file
 */
const char* quest_file(const quest_t* quest)
{
    return quest->file;
}

/*
 * quest_entry_count()
 * The number of entries of the quest
 */
int quest_entry_count(const quest_t* quest)
{
    return darray_length(quest->entry);
}

/*
 * quest_entry_path()
 * The relative filepath of the i-th entry of the quest.
 * Returns NULL if there is no such entry
 */
const char* quest_entry_path(const quest_t* quest, int index)
{
    if(index >= 0 && index < darray_length(quest->entry))
        return quest->entry[index];

    return NULL;
}

/*
 * quest_index_of_entry()
 * Finds the index of the entry that matches the given path.
 * Returns -1 if there is no such entry
 */
int quest_index_of_entry(const quest_t* quest, const char* filepath)
{
    for(int i = 0; i < darray_length(quest->entry); i++) {
        if(0 == str_pathcmp(quest->entry[i], filepath))
            return i;
    }

    return -1;
}

/*
 * quest_entry_is_level()
 * Checks if an entry of the quest is a regular level file
 */
bool quest_entry_is_level(const quest_t* quest, int index)
{
    const char* path = quest_entry_path(quest, index);
    return (path != NULL) && str_pathhasextension(path, ".lev");
}

/*
 * quest_entry_is_level()
 * Checks if an entry of the quest is a regular level path
 */
bool quest_entry_is_quest(const quest_t* quest, int index)
{
    const char* path = quest_entry_path(quest, index);
    return (path != NULL) && str_pathhasextension(path, ".qst");
}

/*
 * quest_entry_is_builtin_scene()
 * Checks if an entry of the quest is a built-in scene
 */
bool quest_entry_is_builtin_scene(const quest_t* quest, int index)
{
    const char* path = quest_entry_path(quest, index);
    return (path != NULL) && (path[0] == '<') && (path[strlen(path)-1] == '>');
}



/* private functions */

/* interprets a statement from a .qst file */
int traverse_quest(const parsetree_statement_t* stmt, void *quest)
{
    quest_t *q = (quest_t*)quest;
    const char *id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);
    const parsetree_parameter_t* p = nanoparser_get_nth_parameter(param_list, 1);

    if(str_icmp(id, "level") == 0 || str_icmp(id, "quest") == 0) {

        /* a .lev or a .qst file */
        /* read my commentary in ../scenes/quest.c about circular dependencies */
        const char* required_extension = tolower(*id) == 'l' ? ".lev" : ".qst";
        nanoparser_expect_string(p, "Quest loader: expected file path");

        if(str_pathhasextension(nanoparser_get_string(p), required_extension))
            darray_push(q->entry, str_dup(nanoparser_get_string(p)));
        else
            fatal_error("Quest loader: command %s expects a %s file at %s:%d", str_to_lower(id, NULL, 0), required_extension, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    }
    else if(id[0] == '<' && id[strlen(id)-1] == '>') {

        /* built-in scene */
        darray_push(q->entry, str_dup(id));

    }
    else if(str_icmp(id, "name") == 0) {

        /* set quest name */
        nanoparser_expect_string(p, "Quest loader: quest name is expected");
        free(q->name);
        q->name = str_dup(nanoparser_get_string(p));

    }
    else if(
        str_icmp(id, "image") == 0 ||
        str_icmp(id, "description") == 0 ||
        str_icmp(id, "version") == 0 ||
        str_icmp(id, "author") == 0
    ) {

        /* these fields are obsolete and were removed
           this code is kept for retro-compatibility */
        nanoparser_expect_string(p, "Quest loader: quest parameter is expected");
        logfile_message("Quest loader: field %s is obsolete", str_to_lower(id, NULL, 0));

    }
    else if(str_icmp(id, "hidden") == 0) {

        /* this field is obsolete and was removed
           this code is kept for retro-compatibility */
        logfile_message("Quest loader: field %s is obsolete", str_to_lower(id, NULL, 0));

    }
    else {

        /* invalid command */
        fatal_error("Quest loader: unexpected \"%s\" at %s:%d", id, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    }

    return 0;
}

/* create a quest structure with a single level (give a relative path to a .lev file) */
quest_t* create_single_level_quest(const char* path_to_lev_file)
{
    quest_t* q = mallocx(sizeof *q);

    q->file = str_dup(path_to_lev_file);
    q->name = str_dup(path_to_lev_file);

    darray_init(q->entry);
    darray_push(q->entry, str_dup(path_to_lev_file));

    return q;
}