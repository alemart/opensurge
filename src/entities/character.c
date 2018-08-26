/*
 * Open Surge Engine
 * character.c - Character system: meta data about a playable character
 * Copyright (C) 2011  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include "character.h"
#include "../core/hashtable.h"
#include "../core/nanoparser/nanoparser.h"
#include "../core/osspec.h"
#include "../core/util.h"
#include "../core/stringutil.h"
#include "../core/soundfactory.h"

HASHTABLE_GENERATE_CODE(character_t)
static hashtable_character_t* characters;

/* private functions */
static character_t *character_new(const char *name); /* creates a new character_t instance */
static void character_delete(character_t* c); /* deletes c */

static int dirfill(const char *filename, void *param); /* file system callback */
static void register_character(character_t *c); /* adds c to the hash table */
static void validate_character(character_t *c); /* validates c */

static int traverse(const parsetree_statement_t *stmt);
static int traverse_character(const parsetree_statement_t *stmt, void *character);
static int traverse_multipliers(const parsetree_statement_t *stmt, void *character);
static int traverse_animations(const parsetree_statement_t *stmt, void *character);
static int traverse_samples(const parsetree_statement_t *stmt, void *character);


/* public */
void charactersystem_init()
{
    const char *path = "characters/*.chr";
    parsetree_program_t *prog = NULL;

    logfile_message("Loading characters...");
    characters = hashtable_character_t_create(character_delete);

    /* Reading the parse tree */
    foreach_resource(path, dirfill, (void*)(&prog), TRUE);
    if(prog == NULL)
        fatal_error("FATAL ERROR: no characters have been found. Please reinstall the game.");

    /* reading the characters */
    nanoparser_traverse_program(prog, traverse);

    /* we're done! */
    prog = nanoparser_deconstruct_tree(prog);
    logfile_message("All characters have been loaded!");
}

void charactersystem_release()
{
    logfile_message("Releasing characters...");
    characters = hashtable_character_t_destroy(characters);
}

const character_t* charactersystem_get(const char* character_name)
{
    const character_t *c = hashtable_character_t_find(characters, character_name);

    if(c == NULL)
        fatal_error("Can't find character '%s'", character_name);

    return c;
}

/* private */
character_t *character_new(const char *name)
{
    character_t *c = mallocx(sizeof *c);

    c->name = str_dup(name);
    c->companion_object_name = str_dup("");

    c->multiplier.acc = 1.0f;
    c->multiplier.dec = 1.0f;
    c->multiplier.topspeed = 1.0f;
    c->multiplier.jmp = 1.0f;
    c->multiplier.grv = 1.0f;
    c->multiplier.slp = 1.0f;
    c->multiplier.frc = 1.0f;
    c->multiplier.chrg = 1.0f;
    
    c->animation.sprite_name = str_dup("");
    c->animation.stopped = 0;
    c->animation.walking = 0;
    c->animation.running = 0;
    c->animation.jumping = 0;
    c->animation.springing = 0;
    c->animation.rolling = 0;
    c->animation.pushing = 0;
    c->animation.gettinghit = 0;
    c->animation.dead = 0;
    c->animation.braking = 0;
    c->animation.ledge = 0;
    c->animation.drowned = 0;
    c->animation.breathing = 0;
    c->animation.waiting = 0;
    c->animation.ducking = 0;
    c->animation.lookingup = 0;
    c->animation.winning = 0;

    c->sample.jump = NULL;
    c->sample.roll = NULL;
    c->sample.brake = NULL;
    c->sample.death = NULL;
    c->sample.charge = NULL;

    c->ability.roll = TRUE;
    c->ability.charge = TRUE;
    c->ability.brake = TRUE;

    return c;
}

void character_delete(character_t* c)
{
    free(c->name);
    free(c->companion_object_name);
    free(c->animation.sprite_name);
    free(c);
}

int dirfill(const char *filename, void *param)
{
    parsetree_program_t** p = (parsetree_program_t**)param;
    *p = nanoparser_append_program(*p, nanoparser_construct_tree(filename));
    return 0;
}

void register_character(character_t *c)
{
    logfile_message("Registering character '%s'...", c->name);
    hashtable_character_t_add(characters, c->name, c);
}

void validate_character(character_t *c)
{
    if(str_icmp(c->name, "") == 0)
        fatal_error("Characters must have a name");

    if(str_icmp(c->animation.sprite_name, "") == 0)
        fatal_error("You must specify the sprite name of the character '%s'", c->name);
}

int traverse(const parsetree_statement_t *stmt)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1, *p2;
    const char *s;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "character") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1); /* first parameter = character name */
        p2 = nanoparser_get_nth_parameter(param_list, 2); /* second parameter = block */

        nanoparser_expect_string(p1, "Must provide character name");
        nanoparser_expect_program(p2, "Must provide character attributes");

        s = nanoparser_get_string(p1);
        logfile_message("Loading character '%s'", s);

        if(NULL == hashtable_character_t_find(characters, s)) {
            character_t *c = character_new(s);
            nanoparser_traverse_program_ex(nanoparser_get_program(p2), (void*)c, traverse_character);
            validate_character(c);
            register_character(c);
        }
        else
            fatal_error("Can't redefine character '%s'\nin\"%s\" near line %d", s, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

        logfile_message("Loaded character '%s'", s);
    }
    else
        fatal_error("Can't load characters. Unknown identifier '%s'\nin\"%s\" near line %d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

int traverse_character(const parsetree_statement_t *stmt, void *character)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    character_t *c = (character_t*)character;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "companion_object") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "companion_object must be the name of an object");
        free(c->companion_object_name);
        c->companion_object_name = str_dup(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "multipliers") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "multipliers must be a block");
        nanoparser_traverse_program_ex(nanoparser_get_program(p1), character, traverse_multipliers);
    }
    else if(str_icmp(identifier, "animations") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "animations must be a block");
        nanoparser_traverse_program_ex(nanoparser_get_program(p1), character, traverse_animations);
    }
    else if(str_icmp(identifier, "samples") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "samples must be a block");
        nanoparser_traverse_program_ex(nanoparser_get_program(p1), character, traverse_samples);
    }
    else
        fatal_error("Can't load characters. Unknown identifier '%s'\nin\"%s\" near line %d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

int traverse_multipliers(const parsetree_statement_t *stmt, void *character)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    character_t *c = (character_t*)character;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "acceleration") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "acceleration must be a positive number");
        c->multiplier.acc = max(0, atof(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "deceleration") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "deceleration must be a positive number");
        c->multiplier.dec = max(0, atof(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "friction") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "friction must be a positive number");
        c->multiplier.frc = max(0, atof(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "topspeed") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "topspeed must be a positive number");
        c->multiplier.topspeed = max(0, atof(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "jump") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "jump must be a positive number");
        c->multiplier.jmp = max(0, atof(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "gravity") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "gravity must be a positive number");
        c->multiplier.grv = max(0, atof(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "slope") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "slope must be a positive number");
        c->multiplier.slp = max(0, atof(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "charge") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "charge must be a positive number");
        c->multiplier.chrg = max(0, atof(nanoparser_get_string(p1)));
    }

    /* the multipliers below have been deprecated, but their identifiers
       are still matched for compatibility purposes */
    else if(
        str_icmp(identifier, "jumprel") == 0 ||
        str_icmp(identifier, "rollthreshold") == 0 ||
        str_icmp(identifier, "brakingthreshold") == 0 ||
        str_icmp(identifier, "rolluphillslope") == 0 ||
        str_icmp(identifier, "rolldownhillslope") == 0
    ) {
        ;
    }

    /* unknown identifier */
    else
        fatal_error("Can't load characters. Unknown identifier '%s'\nin\"%s\" near line %d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

int traverse_animations(const parsetree_statement_t *stmt, void *character)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    character_t *c = (character_t*)character;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "sprite_name") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "sprite_name must be the name of a sprite");
        free(c->animation.sprite_name);
        c->animation.sprite_name = str_dup(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "stopped") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.stopped = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "walking") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.walking = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "running") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.running = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "jumping") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.jumping = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "springing") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.springing = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "rolling") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.rolling = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "pushing") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.pushing = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "gettinghit") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.gettinghit = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "dead") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.dead = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "braking") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.braking = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "ledge") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.ledge = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "drowned") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.drowned = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "breathing") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.breathing = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "waiting") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.waiting = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "ducking") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.ducking = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "lookingup") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.lookingup = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "winning") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.winning = max(0, atoi(nanoparser_get_string(p1)));
    }
    else if(str_icmp(identifier, "ceiling") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "the animations must be non-negative numbers");
        c->animation.ceiling = max(0, atoi(nanoparser_get_string(p1)));
    }
    else
        fatal_error("Can't load characters. Unknown identifier '%s'\nin\"%s\" near line %d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

int traverse_samples(const parsetree_statement_t *stmt, void *character)
{
    const char *identifier;
    const parsetree_parameter_t *param_list;
    const parsetree_parameter_t *p1;
    character_t *c = (character_t*)character;

    identifier = nanoparser_get_identifier(stmt);
    param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "jump") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "must specify the samples");
        c->sample.jump = soundfactory_get(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "roll") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "must specify the samples");
        c->sample.roll = soundfactory_get(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "brake") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "must specify the samples");
        c->sample.brake = soundfactory_get(nanoparser_get_string(p1));
    }
    else if(str_icmp(identifier, "death") == 0) {
        p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "must specify the samples");
        c->sample.death = soundfactory_get(nanoparser_get_string(p1));
    }
    else
        fatal_error("Can't load characters. Unknown identifier '%s'\nin\"%s\" near line %d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}
