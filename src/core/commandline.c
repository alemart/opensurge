/*
 * Open Surge Engine
 * commandline.c - command line parser
 * Copyright (C) 2010-2013, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "commandline.h"
#include "global.h"
#include "logfile.h"
#include "stringutil.h"
#include "video.h"
#include "install.h"

/* private stuff ;) */
static void crash(char *fmt, ...);
static int print_gameid(const char* gameid, void* data);
static const char* COPYRIGHT = "Open Surge Engine version " GAME_VERSION_STRING "\n"
                               "Copyright (C) " GAME_YEAR " Alexandre Martins";
static int COMMANDLINE_UNDEFINED = -1;





/*
 * commandline_parse()
 * Parses the command line arguments
 */
commandline_t commandline_parse(int argc, char **argv)
{
    const char* program = str_basename(argv[0]);
    commandline_t cmd;
    int i;

    /* initializing values */
    cmd.video_resolution = COMMANDLINE_UNDEFINED;
    cmd.smooth_graphics = COMMANDLINE_UNDEFINED;
    cmd.fullscreen = COMMANDLINE_UNDEFINED;
    cmd.color_depth = COMMANDLINE_UNDEFINED;
    cmd.show_fps = COMMANDLINE_UNDEFINED;
    cmd.custom_level_path[0] = '\0';
    cmd.custom_quest_path[0] = '\0';
    cmd.language_filepath[0] = '\0';
    cmd.install_game_path[0] = '\0';
    cmd.datadir[0] = '\0';
    cmd.gameid[0] = '\0';
    cmd.allow_font_smoothing = COMMANDLINE_UNDEFINED;
    cmd.user_argv = NULL;
    cmd.user_argc = 0;

    /* logfile */
    logfile_message("game arguments:");
    for(i=0; i<argc; i++)
        logfile_message("argv[%d]: '%s'", i, argv[i]);

    /* reading data... */
    for(i=1; i<argc; i++) {

        if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf(
                "%s\n<%s>\n\n"
                "usage:\n"
                "    %s [options ...]\n"
                "\n"
                "where options include:\n"
                "    --help -h                        display this message\n"
                "    --version                        show the version of this program\n"
                "    --fullscreen                     fullscreen mode\n"
                "    --windowed                       windowed mode\n"
                "    --resolution X                   set the scale of the window size, where X = 1, 2, 3 or 4\n"
                "    --smooth                         display smooth graphics (applicable when resolution > 1)\n"
                "    --color-depth X                  set the color depth to X bits/pixel, where X = 16, 24 or 32\n"
                "    --show-fps                       show the FPS (frames per second) counter\n"
                "    --level \"filepath\"               run the specified level (e.g., levels/my_level.lev)\n"
                "    --quest \"filepath\"               run the specified quest (e.g., quests/my_quest.qst)\n"
                "    --language \"filepath\"            use the specified language (e.g., languages/lang.lng)\n"
                "    --games                          list the installed Open Surge games\n"
                "    --game \"gameid\"                  run the specified Open Surge game (e.g., opensurge)\n"
                "    --install \"/path/to/zipfile.zip\" install an Open Surge game package (use its absolute path)\n"
                "    --build [\"gameid\"]               build an Open Surge game package for redistribution\n"
                "    --data-dir \"/path/to/data\"       load the game assets from the specified folder\n"
                "    --no-font-smoothing              disable antialiased fonts\n"
                "    -- -arg1 -arg2 -arg3...          user-defined arguments (useful for scripting)\n",
                COPYRIGHT, GAME_WEBSITE,
                program
            );
            exit(0);
        }

        else if(strcmp(argv[i], "--version") == 0) {
            puts(GAME_VERSION_STRING);
            exit(0);
        }

        else if(strcmp(argv[i], "--resolution") == 0) {
            if(++i < argc && *(argv[i]) != '-') {
                if(strcmp(argv[i], "1") == 0)
                    cmd.video_resolution = VIDEORESOLUTION_1X;
                else if(strcmp(argv[i], "2") == 0)
                    cmd.video_resolution = VIDEORESOLUTION_2X;
                else if(strcmp(argv[i], "3") == 0)
                    cmd.video_resolution = VIDEORESOLUTION_3X;
                else if(strcmp(argv[i], "4") == 0)
                    cmd.video_resolution = VIDEORESOLUTION_4X;
                else
                    crash("Invalid resolution: %s", argv[i]);
            }
        }

        else if(strcmp(argv[i], "--smooth") == 0) {
#ifndef A5BUILD
            cmd.smooth_graphics = TRUE;
            if(cmd.video_resolution == VIDEORESOLUTION_1X)
                cmd.video_resolution = VIDEORESOLUTION_2X;
#else
            crash("Smooth graphics: not yet implemented in the Allegro 5 backend");
#endif
        }

        else if(strcmp(argv[i], "--tiny") == 0) /* obsolete */
            cmd.video_resolution = VIDEORESOLUTION_1X;

        else if(strcmp(argv[i], "--fullscreen") == 0)
            cmd.fullscreen = TRUE;

        else if(strcmp(argv[i], "--windowed") == 0)
            cmd.fullscreen = FALSE;

        else if(strcmp(argv[i], "--color-depth") == 0) {
            if(++i < argc && *(argv[i]) != '-') {
                cmd.color_depth = atoi(argv[i]);
                if(cmd.color_depth != 16 && cmd.color_depth != 24 && cmd.color_depth != 32) {
                    crash("Invalid color depth: %d", cmd.color_depth);
                    cmd.color_depth = COMMANDLINE_UNDEFINED;
                }
            }
        }

        else if(strcmp(argv[i], "--show-fps") == 0)
            cmd.show_fps = TRUE;

        else if(strcmp(argv[i], "--no-font-smoothing") == 0)
            cmd.allow_font_smoothing = FALSE;

        else if(strcmp(argv[i], "--level") == 0) {
            if(++i < argc && *(argv[i]) != '-')
                str_cpy(cmd.custom_level_path, argv[i], sizeof(cmd.custom_level_path));
            else
                crash("%s: missing --level parameter", program);
        }

        else if(strcmp(argv[i], "--quest") == 0) {
            if(++i < argc && *(argv[i]) != '-')
                str_cpy(cmd.custom_quest_path, argv[i], sizeof(cmd.custom_quest_path));
            else
                crash("%s: missing --quest parameter", program);
        }

        else if(strcmp(argv[i], "--language") == 0) {
            if(++i < argc && *(argv[i]) != '-')
                str_cpy(cmd.language_filepath, argv[i], sizeof(cmd.language_filepath));
            else
                crash("%s: missing --language parameter", program);
        }

        else if(strcmp(argv[i], "--data-dir") == 0) {
            if(++i < argc && *(argv[i]) != '-')
                str_cpy(cmd.datadir, argv[i], sizeof(cmd.datadir));
            else
                crash("%s: missing --data-dir parameter", program);
        }

        else if(strcmp(argv[i], "--games") == 0) {
            foreach_installed_game(print_gameid, NULL);
            exit(0);
        }

        else if(strcmp(argv[i], "--game") == 0) {
            if(++i < argc && *(argv[i]) != '-') {
                str_cpy(cmd.gameid, argv[i], sizeof(cmd.gameid));
                if(!is_game_installed(cmd.gameid))
                    crash("%s: game \"%s\" is not installed\nRun %s --games to see the installed games", program, cmd.gameid, program);
            }
            else
                crash("%s: missing --game parameter", program);
        }

        else if(strcmp(argv[i], "--install") == 0) {
            if(++i < argc && *(argv[i]) != '-') {
                str_cpy(cmd.install_game_path, argv[i], sizeof(cmd.install_game_path));
                if(!install_game(cmd.install_game_path, cmd.gameid, sizeof(cmd.gameid), true))
                    exit(0);
            }
            else
                crash("%s: missing --install parameter", program);
        }

        else if(strcmp(argv[i], "--build") == 0) {
            if(++i < argc && *(argv[i]) != '-')
                str_cpy(cmd.gameid, argv[i], sizeof(cmd.gameid));
            build_game((cmd.gameid[0] != '\0') ? cmd.gameid : NULL);
            exit(0);
        }

        else if(strcmp(argv[i], "--") == 0) {
            if(++i < argc) {
                cmd.user_argv = (const char**)(argv + i);
                cmd.user_argc = argc - i;
                break;
            }
        }

        else { /* unknown option */
            crash("%s: bad command line option \"%s\"\nRun %s --help to get more information", program, argv[i], program);
        }

    }

    /* done! */
    return cmd;
}

/*
 * commandline_getint()
 * Gets an integer from the command line, or use a
 * default value if it hasn't been specified explicitly
 */
int commandline_getint(int value, int default_value)
{
    return (value != COMMANDLINE_UNDEFINED) ? value : default_value;
}

/*
 * commandline_getstring()
 * Gets a string from the command line, or use
 * a default string if it hasn't been specified explicitly
 */
const char* commandline_getstring(const char* value, const char* default_string)
{
    return (value && *value) ? value : default_string;
}


/* private functions */


/* Displays a message to the user (printf format) and exists */
void crash(char *fmt, ...)
{
    char buf[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    puts(buf);
    exit(1);
}



/* prints gameid */
int print_gameid(const char* gameid, void* data)
{
    printf("- %s\n", gameid);
    return 0;
}
