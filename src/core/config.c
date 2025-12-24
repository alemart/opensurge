/*
 * Open Surge Engine
 * config.c - game configuration - file reader
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

#include <string.h>
#include "config.h"
#include "asset.h"
#include "logfile.h"
#include "nanoparser.h"
#include "../util/util.h"
#include "../util/stringutil.h"

/* Utilities */
#define UNDEFINED_INT           (-0xCEB01A)
#define UNDEFINED_STRING        "<undefined>"
#define IS_UNDEFINED_INT(x)     ((x) == UNDEFINED_INT)
#define IS_UNDEFINED_STRING(s)  (0 == strcmp((s), UNDEFINED_STRING))

/* Game settings */
static char game_title[64] = UNDEFINED_STRING;
static char game_version[32] = UNDEFINED_STRING;

/* Video settings */
static int video_screen_width = UNDEFINED_INT;
static int video_screen_height = UNDEFINED_INT;

/* Configuration file */
static const char CONFIG_FILE[] = "surge.cfg";
static bool read_config_file(const char* vpath);
static int traverse(const parsetree_statement_t* stmt);
static int traverse_game(const parsetree_statement_t* stmt);
static int traverse_video(const parsetree_statement_t* stmt);
static char* sanitize_string(char* string);



/*
 * public
 */


/*
 * config_init()
 * Read the configuration file
 */
bool config_init()
{
    /* initialize settings to undefined */
    strcpy(game_title, UNDEFINED_STRING);
    strcpy(game_version, UNDEFINED_STRING);
    video_screen_width = UNDEFINED_INT;
    video_screen_height = UNDEFINED_INT;

    /* read the configuration file */
    bool success = read_config_file(CONFIG_FILE);

    /* done! */
    return success;
}

/*
 * config_release()
 * Release the configuration module
 */
void config_release()
{
    ;
}

/*
 * config_game_title()
 * Title of the game currently running on the engine
 */
const char* config_game_title(const char* default_value)
{
    return IS_UNDEFINED_STRING(game_title) ? default_value : game_title;
}

/*
 * config_game_version()
 * Version string of the game currently running on the engine
 */
const char* config_game_version(const char* default_value)
{
    return IS_UNDEFINED_STRING(game_version) ? default_value : game_version;
}

/*
 * config_video_screen_width()
 * The width of the screen, in pixels
 */
int config_video_screen_width(int default_value)
{
    return IS_UNDEFINED_INT(video_screen_width) ? default_value : video_screen_width;
}

/*
 * config_video_screen_height()
 * The height of the screen, in pixels
 */
int config_video_screen_height(int default_value)
{
    return IS_UNDEFINED_INT(video_screen_height) ? default_value : video_screen_height;
}





/*
 * private
 */

/* read the configuration file */
bool read_config_file(const char* vpath)
{
    if(!asset_exists(vpath)) {
        logfile_message("Can't read \"%s\": file not found!", vpath);
        return false;
    }

    const char* fullpath = asset_path(vpath);
    parsetree_program_t* p = nanoparser_construct_tree(fullpath);
    nanoparser_traverse_program(p, traverse);
    nanoparser_deconstruct_tree(p);

    return true;
}

/* traverse the configuration file */
int traverse(const parsetree_statement_t* stmt)
{
    const char* identifier = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "game") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "Expected game settings");

        nanoparser_traverse_program(nanoparser_get_program(p1), traverse_game);
    }
    else if(str_icmp(identifier, "video") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_program(p1, "Expected video settings");

        nanoparser_traverse_program(nanoparser_get_program(p1), traverse_video);
    }
    else
        fatal_error("Unexpected identifier \"%s\" in %s:%d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

/* read the game section of the configuration file */
int traverse_game(const parsetree_statement_t* stmt)
{
    const char* identifier = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "title") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Expected game title");

        str_cpy(game_title, nanoparser_get_string(p1), sizeof(game_title));
        sanitize_string(game_title);
    }
    else if(str_icmp(identifier, "version") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        nanoparser_expect_string(p1, "Expected game version");

        str_cpy(game_version, nanoparser_get_string(p1), sizeof(game_version));
        sanitize_string(game_version);
    }
    else
        fatal_error("Unexpected identifier \"%s\" in %s:%d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

/* read the video section of the configuration file */
int traverse_video(const parsetree_statement_t* stmt)
{
    const char* identifier = nanoparser_get_identifier(stmt);
    const parsetree_parameter_t* param_list = nanoparser_get_parameter_list(stmt);

    if(str_icmp(identifier, "screen_size") == 0) {
        const parsetree_parameter_t* p1 = nanoparser_get_nth_parameter(param_list, 1);
        const parsetree_parameter_t* p2 = nanoparser_get_nth_parameter(param_list, 2);
        nanoparser_expect_string(p1, "Expected screen_size: width, height");
        nanoparser_expect_string(p2, "Expected screen_size: width, height");

        video_screen_width = atoi(nanoparser_get_string(p1));
        video_screen_height = atoi(nanoparser_get_string(p2));

        if(video_screen_width <= 0 || video_screen_height <= 0)
            fatal_error("Invalid screen_size (%d x %d) in %s:%d", video_screen_width, video_screen_height, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));
    }
    else
        fatal_error("Unexpected identifier \"%s\" in %s:%d", identifier, nanoparser_get_file(stmt), nanoparser_get_line_number(stmt));

    return 0;
}

/* sanitize a string */
char* sanitize_string(char* string)
{
    /* get rid of newlines */
    for(char* p = string; *p; p++) {
        if(*p == '\n' || *p == '\r')
            *p = ' ';
    }

    /* done! */
    return string;
}