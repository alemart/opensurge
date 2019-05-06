/*
 * Open Surge Engine
 * soundfactory.c - sound factory
 * Copyright (C) 2010  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "soundfactory.h"
#include "audio.h"
#include "stringutil.h"
#include "util.h"
#include "logfile.h"
#include "resourcemanager.h"
#include "assetfs.h"
#include "hashtable.h"
#include "nanoparser/nanoparser.h"

/* storage */
typedef struct factorysound_t factorysound_t;
struct factorysound_t {
    sound_t *data;
};
static factorysound_t* factorysound_create();
static void factorysound_destroy(factorysound_t *f);
HASHTABLE_GENERATE_CODE(factorysound_t, factorysound_destroy);
static HASHTABLE(factorysound_t, samples);

/* file reader stuff (nanoparser) */
static void load_samples_table();
static int traverse(const parsetree_statement_t *stmt);
static int traverse_sound(const parsetree_statement_t *stmt, void *factorysound);

/* config file */
#define SAMPLES_FILE "config/samples.def"




/* public functions */

/* initializes the sound factory */
void soundfactory_init()
{
    samples = hashtable_factorysound_t_create();
    load_samples_table();
}

/* releases the sound factory */
void soundfactory_release()
{
    samples = hashtable_factorysound_t_destroy(samples);
}

/* given a sound name, returns the corresponding sound effect */
sound_t *soundfactory_get(const char *sound_name)
{
    factorysound_t *f = hashtable_factorysound_t_find(samples, sound_name);

    if(f == NULL) {
        /* if no sound is found, consider sound_name as a file path */
        return sound_load(sound_name);
    }
    else
        return f->data;
}




/* private functions */

/* traverses a sound configuration file */
int traverse(const parsetree_statement_t *stmt)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2;
    const char *sound_name = "null";
    factorysound_t *f = NULL;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "sample") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        p2 = nanoparser_get_nth_parameter(param_list, 2);

        nanoparser_expect_string(p1, "soundfactory: must provide sample name");
        nanoparser_expect_program(p2, "soundfactory: must provide sample attributes");

        sound_name = nanoparser_get_string(p1);
        if(NULL == hashtable_factorysound_t_find(samples, sound_name)) {
            f = factorysound_create();
            nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)f, traverse_sound);
            hashtable_factorysound_t_add(samples, sound_name, f);
        }

        logfile_message("soundfactory: loaded sample '%s'", sound_name);
    }
    else
        fatal_error("soundfactory: unknown identifier '%s' at the sound definition file. Valid keywords: 'sample'", identifier);

    return 0;
}

/* traverses a sound block */
int traverse_sound(const parsetree_statement_t *stmt, void *factorysound)
{
    factorysound_t *f = (factorysound_t*)factorysound;
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    int n;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "source_file") == 0) {
        n = nanoparser_get_number_of_parameters(param_list);
        if(n == 1) {
            p1 = nanoparser_get_nth_parameter(param_list, 1);
            nanoparser_expect_string(p1, "soundfactory: must provide sound file path (source_file)");
            f->data = sound_load(nanoparser_get_string(p1));
        }
        else
            fatal_error("soundfactory: source_file accepts only one parameter.");
    }
    else
        fatal_error("soundfactory: unknown identifier '%s' defined at a sound block. Valid keywords: 'source_file'", identifier);

    return 0;
}

/* loads the samples table */
void load_samples_table()
{
    parsetree_program_t *s = NULL;
    const char* fullpath = assetfs_fullpath(SAMPLES_FILE);

    logfile_message("soundfactory: loading the samples table...");

    s = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program(s, traverse);
    s = nanoparser_deconstruct_tree(s);
}

/* creates a new factorysound object */
factorysound_t* factorysound_create()
{
    factorysound_t* f = mallocx(sizeof *f);
    f->data = NULL;
    return f;
}

/* destroys a factory sound */
void factorysound_destroy(factorysound_t *f)
{
    free(f);
}

