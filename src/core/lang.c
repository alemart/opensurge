/*
 * Open Surge Engine
 * lang.c - language/translation module
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
#include <ctype.h>
#include "lang.h"
#include "global.h"
#include "asset.h"
#include "logfile.h"
#include "nanoparser.h"
#include "../util/util.h"
#include "../util/stringutil.h"
#include "../util/hashtable.h"

/* fake string type */
typedef struct { char* data; } stringadapter_t;
static stringadapter_t* stringadapter_create(const char* data);
static const char* stringadapter_get_data(const stringadapter_t *s);
static void stringadapter_set_data(stringadapter_t *s, const char* data);
static void stringadapter_destroy(stringadapter_t *s);

/* hash table */
HASHTABLE_GENERATE_CODE(stringadapter_t, stringadapter_destroy);
static HASHTABLE(stringadapter_t, strings);

/* private stuff */
#define NULL_STRING "null"
#define UNTRANSLATED_STRING "FIXME" /* indicates that a language string hasn't been translated */

static const char DEFAULT_LANGUAGE_FILEPATH[] = "languages/english.lng";
static const char LANGUAGES_FOLDER[] = "languages/";
static const char EXTENDS_FOLDER[] = "extends/";

static char lang_id[32] = NULL_STRING;
typedef struct { const char* key; const char* value; } inout_t;
static int traverse(const parsetree_statement_t *stmt);
static int traverse_inout(const parsetree_statement_t *stmt, void *inout);
static int traverse_count(const parsetree_statement_t *stmt, void *counters);
static bool is_untranslated_entry(const parsetree_statement_t *stmt);
static char* pathify(const char* path);
static char* path_to_language_extension(const char* path);




/*
 * lang_init()
 * Initializes the language module
 */
void lang_init()
{
    logfile_message("Initializing the language module");
    strings = hashtable_stringadapter_t_create();
    lang_loadfile(DEFAULT_LANGUAGE_FILEPATH);
    logfile_message("The language module has been initialized");

    (void)traverse_count; /* TODO */
}


/*
 * lang_release()
 * Releases the language module
 */
void lang_release()
{
    logfile_message("Releasing the language module...");
    strings = hashtable_stringadapter_t_destroy(strings);
}


/*
 * lang_loadfile()
 * Loads a language definition file, given its relative path
 */
void lang_loadfile(const char* filepath)
{
    char* path = pathify(filepath);
    int supver, subver, wipver;

    #define READ_LANGUAGE_FILE(path_to_lng_file) do { \
        const char* fullpath = asset_path(path_to_lng_file); \
        parsetree_program_t* prog = nanoparser_construct_tree(fullpath); \
        nanoparser_traverse_program(prog, traverse); \
        prog = nanoparser_deconstruct_tree(prog); \
    } while(0)



    /* log */
    logfile_message("Loading language file \"%s\"...", path);

    /* Check if the path is in the languages/ folder */
    if(str_incmp(path, LANGUAGES_FOLDER, strlen(LANGUAGES_FOLDER)) != 0)
        fatal_error("Won't load \"%s\". Language files are expected to be in the %s folder.", path, LANGUAGES_FOLDER);

    /* Check if the path exists */
    if(!asset_exists(path)) {

        /* Crash if the default language file is missing */
        if(0 == str_pathcmp(path, DEFAULT_LANGUAGE_FILEPATH))
            fatal_error("Missing default language file: \"%s\". Please reinstall the game.", DEFAULT_LANGUAGE_FILEPATH);

        /* If some other language file is missing, we don't crash the application,
           otherwise the player may get locked due to a corrupted save state */
        logfile_message("Missing language file: \"%s\"", path);
        lang_loadfile(DEFAULT_LANGUAGE_FILEPATH);
        return;

    }

    /* Check if the path points to a language extension */
    bool is_language_extension = (
        0 == str_incmp(path, LANGUAGES_FOLDER, strlen(LANGUAGES_FOLDER)) && /* already checked */
        0 == str_incmp(path + strlen(LANGUAGES_FOLDER), EXTENDS_FOLDER, strlen(EXTENDS_FOLDER))
    );
    if(is_language_extension)
        logfile_message("\"%s\" is a language extension", path);

    /* Compatibility check */
    lang_compatibility(path, &supver, &subver, &wipver);
    if(game_version_compare(supver, subver, wipver) < 0) /* backwards compatibility */
        fatal_error("Language file \"%s\" (version %d.%d.%d) is not compatible with this version of the engine (%s)!", path, supver, subver, wipver, GAME_VERSION_STRING);

    /* Read the default language file to fill in any missing strings */
    if(str_pathcmp(path, DEFAULT_LANGUAGE_FILEPATH) != 0)
        lang_loadfile(DEFAULT_LANGUAGE_FILEPATH);

    /* Read language file to memory */
    READ_LANGUAGE_FILE(path);

    /* Check if there is a language extension available */
    if(!is_language_extension) {
        char* extpath = path_to_language_extension(path);

        /* There is a language extension */
        if(asset_exists(extpath)) {

            /* Load language extension */
            logfile_message("Loading language extension at \"%s\"...", extpath);
            READ_LANGUAGE_FILE(extpath);

        }
        else
            logfile_message("No language extension found at \"%s\"", extpath);

        free(extpath);
    }

    /* Update language ID */
    lang_getstring("LANG_ID", lang_id, sizeof(lang_id));

    /* done! */
    logfile_message("Language file \"%s\" has been loaded successfully!", path);
    free(path);
}


/*
 * lang_metadata()
 * Reads the contents of the desired key directly from the
 * language file, without loading it in memory. Returns dest.
 */
char* lang_metadata(const char* filepath, const char* desired_key, char* dest, size_t dest_size)
{
    const char* fullpath = asset_path(filepath);
    inout_t param = { .key = desired_key, .value = NULL };
    parsetree_program_t *prog = nanoparser_construct_tree(fullpath);

    nanoparser_traverse_program_ex(prog, (void*)(&param), traverse_inout);

    if(param.value == NULL)
        str_cpy(dest, NULL_STRING, dest_size);
    else
        str_cpy(dest, param.value, dest_size);

    prog = nanoparser_deconstruct_tree(prog);
    return dest;
}


/*
 * lang_getstring()
 * Retrieves some string from the language definition file
 */
char* lang_getstring(const char* desired_key, char* dest, size_t dest_size)
{
    const stringadapter_t *s = hashtable_stringadapter_t_find(strings, desired_key);

    if(s != NULL)
        return str_cpy(dest, stringadapter_get_data(s), dest_size);
    else
        return str_cpy(dest, NULL_STRING, dest_size);
}



/*
 * lang_get()
 * Like lang_getstring(), but returns the string as a static char*
 */
const char* lang_get(const char* desired_key)
{
    static char buf[1024];
    lang_getstring(desired_key, buf, sizeof(buf));
    return buf;
}


/*
 * lang_getid()
 * Returns LANG_ID (fast)
 */
const char* lang_getid()
{
    return lang_id;
}


/*
 * lang_compatibility()
 * Language files are made for specific game versions
 */
void lang_compatibility(const char* filepath, int* supver, int* subver, int* wipver)
{
    char compat[32];
    lang_metadata(filepath, "LANG_COMPATIBILITY", compat, sizeof(compat));
    if(sscanf(compat, "%d.%d.%d", supver, subver, wipver) < 3)
        *supver = *subver = *wipver = 0;
}


/*
 * lang_haskey()
 * Checks if a key exists
 */
bool lang_haskey(const char* desired_key)
{
    const stringadapter_t *s = hashtable_stringadapter_t_find(strings, desired_key);
    return s != NULL;
}


/* private stuff */

int traverse(const parsetree_statement_t *stmt)
{
    const char* id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);
    const parsetree_parameter_t *p = nanoparser_get_nth_parameter(param_list, 1);
    const char* key, *value;
    stringadapter_t *s;

    if(is_untranslated_entry(stmt))
        return 0;

    if(nanoparser_get_number_of_parameters(param_list) != 1) 
        fatal_error("Language file error: invalid syntax at line %d in\n\"%s\"", nanoparser_get_line_number(stmt), nanoparser_get_file(stmt));

    nanoparser_expect_string(p, "a string is expected after each key of the language file");
    key = id;
    value = nanoparser_get_string(p);

    if(NULL == (s = hashtable_stringadapter_t_find(strings, key)))
        hashtable_stringadapter_t_add(strings, key, stringadapter_create(value));
    else
        stringadapter_set_data(s, value);

    return 0;
}

int traverse_inout(const parsetree_statement_t *stmt, void *inout)
{
    inout_t *x = (inout_t*)inout;
    const char* id = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t *param_list = nanoparser_get_parameter_list(stmt);
    const parsetree_parameter_t *p = nanoparser_get_nth_parameter(param_list, 1);

    if(is_untranslated_entry(stmt))
        return 0;

    if(nanoparser_get_number_of_parameters(param_list) != 1) 
        fatal_error("Language file error: invalid syntax at line %d in\n\"%s\"", nanoparser_get_line_number(stmt), nanoparser_get_file(stmt));

    nanoparser_expect_string(p, "a string is expected after each key of the language file");
    if(str_icmp(id, x->key) == 0) {
        x->value = nanoparser_get_string(p);
        return 1; /* stop the enumeration */
    }

    return 0;
}

int traverse_count(const parsetree_statement_t *stmt, void *counters)
{
    int *counter = (int*)counters;

    if(!is_untranslated_entry(stmt))
        counter[0]++;
    else
        counter[1]++;

    return 0;
}

bool is_untranslated_entry(const parsetree_statement_t *stmt)
{
    const char* id = nanoparser_get_identifier(stmt);
    return (0 == str_icmp(id, UNTRANSLATED_STRING));
}

stringadapter_t* stringadapter_create(const char* data)
{
    stringadapter_t *s = mallocx(sizeof *s);
    s->data = str_dup(data);
    return s;
}

void stringadapter_destroy(stringadapter_t *s)
{
    free(s->data);
    free(s);
}

const char* stringadapter_get_data(const stringadapter_t *s)
{
    return s->data;
}

void stringadapter_set_data(stringadapter_t *s, const char* data)
{
    free(s->data);
    s->data = str_dup(data);
}

/* replace backslashes by slashes; you'll have to free this string afterwards */
char* pathify(const char* path)
{
    const char* p = path;
    char* buf = mallocx((1 + strlen(path)) * sizeof(*buf));
    char* q = buf, c;

    while((c = *(p++)))
        *(q++) = (c == '\\') ? '/' : c;
    *q = '\0';

    return buf;
}

/* path of the language extension (e.g., "languages/english.lng" becomes "languages/extends/english.lng")
   it's assumed that the language file is located in the root of the languages folder
   you'll have to free this string afterwards */
char* path_to_language_extension(const char* path)
{
    const char* basename = str_basename(path);
    size_t pathsize = strlen(LANGUAGES_FOLDER) + strlen(EXTENDS_FOLDER) + strlen(basename) + 1;
    char* extpath = mallocx(pathsize);

    *extpath = '\0';
    strcpy(extpath, LANGUAGES_FOLDER);
    strcat(extpath, EXTENDS_FOLDER);
    strcat(extpath, basename);

    return extpath;
}
