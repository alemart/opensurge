/*
 * Open Surge Engine
 * commandline.c - command line parser
 * Copyright (C) 2008-2023  Alexandre Martins <alemartf@gmail.com>
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
#include <ctype.h>
#include <surgescript.h>
#include "commandline.h"
#include "global.h"
#include "video.h"
#include "asset.h"
#include "import.h"
#include "../util/stringutil.h"

#ifdef _WIN32
#include <windows.h>
#endif

/* private stuff ;) */
#define crash(...) do { console_print(__VA_ARGS__); exit(1); } while(0)
static void console_print(char *fmt, ...);
static bool console_ask(const char* fmt, ...);

static int COMMANDLINE_UNDEFINED = -1;

static const char LICENSE[] = ""
"This program is free software; you can redistribute it and/or modify\n"
"it under the terms of the GNU General Public License as published by\n"
"the Free Software Foundation; either version 3 of the License, or\n"
"(at your option) any later version.\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU General Public License\n"
"along with this program.  If not, see <http://www.gnu.org/licenses/>.";





/*
 * commandline_parse()
 * Parses the command line arguments
 */
commandline_t commandline_parse(int argc, char **argv)
{
    const char* program = str_basename(argv[0]);
    commandline_t cmd;

    /* initializing values */
    cmd.video_resolution = COMMANDLINE_UNDEFINED;
    cmd.video_quality = COMMANDLINE_UNDEFINED;
    cmd.fullscreen = COMMANDLINE_UNDEFINED;
    cmd.show_fps = COMMANDLINE_UNDEFINED;
    cmd.hide_fps = COMMANDLINE_UNDEFINED;

    cmd.mobile = COMMANDLINE_UNDEFINED;
    cmd.verbose = COMMANDLINE_UNDEFINED;

    cmd.custom_level_path[0] = '\0';
    cmd.custom_quest_path[0] = '\0';
    cmd.language_filepath[0] = '\0';
    cmd.gamedir[0] = '\0';

    cmd.user_argv = NULL;
    cmd.user_argc = 0;
    cmd.argv = (const char**)argv;
    cmd.argc = argc;

    /* reading data... */
    for(int i = 1; i < argc; i++) {

        if(strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            console_print(
                "%s\n"
                "\n"
                "usage:\n"
                "    %s [options ...]\n"
                "\n"
                "where options include:\n"
                "    --help -h                        display this message\n"
                "    --version -v                     display the version of this program\n"
                "    --ssversion                      display the version of the SurgeScript runtime\n"
                "    --license                        display the license of this game engine\n"
                "    --fullscreen                     fullscreen mode\n"
                "    --windowed                       windowed mode\n"
                "    --resolution X                   set the scale of the window size, where X = 1, 2, 3 or 4\n"
                "    --quality Q                      set the video quality Q to \"low\", \"medium\" or \"high\"\n"
                "    --show-fps                       show the FPS (frames per second) counter\n"
                "    --hide-fps                       hide the FPS counter\n"
                "    --level \"filepath\"               run the specified level (e.g., levels/my_level.lev)\n"
                "    --quest \"filepath\"               run the specified quest (e.g., quests/default.qst)\n"
                "    --language \"filepath\"            use the specified language (e.g., languages/english.lng)\n"
                "    --game-folder \"/path/to/game\"    use game assets only from the specified folder\n"
                "    --reset                          factory reset: clear all user-space files & changes\n"
                "    --import \"/path/to/game\"         import an Open Surge game from the specified folder\n"
                "    --import-wizard                  import an Open Surge game using a wizard\n"
                "    --mobile                         enable mobile device simulation\n"
                "    --verbose                        print logs to stdout\n"
                "    -- -arg1 -arg2 -arg3...          user-defined arguments (useful for scripting)",
                GAME_HEADER, program
            );
            exit(0);
        }

        else if(strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            console_print("%s", GAME_VERSION_STRING);
            exit(0);
        }

        else if(strcmp(argv[i], "--ssversion") == 0) {
            console_print("%s", surgescript_util_version());
            exit(0);
        }

        else if(strcmp(argv[i], "--license") == 0) {
            console_print("%s\n\n%s", GAME_HEADER, LICENSE);
            exit(0);
        }

        else if(strcmp(argv[i], "--tiny") == 0) /* obsolete */
            cmd.video_resolution = VIDEORESOLUTION_1X;

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
                    crash("Invalid video resolution: %s", argv[i]);
            }
            else
                crash("%s: missing --resolution parameter", program);
        }

        else if(strcmp(argv[i], "--quality") == 0) {
            if(++i < argc && *(argv[i]) != '-') {
                if(strcmp(argv[i], "low") == 0)
                    cmd.video_quality = VIDEOQUALITY_LOW;
                else if(strcmp(argv[i], "medium") == 0)
                    cmd.video_quality = VIDEOQUALITY_MEDIUM;
                else if(strcmp(argv[i], "high") == 0)
                    cmd.video_quality = VIDEOQUALITY_HIGH;
                else
                    crash("Invalid video quality: %s", argv[i]);
            }
            else
                crash("%s: missing --quality parameter", program);
        }

        else if(strcmp(argv[i], "--fullscreen") == 0)
            cmd.fullscreen = TRUE;

        else if(strcmp(argv[i], "--windowed") == 0)
            cmd.fullscreen = FALSE;

        else if(strcmp(argv[i], "--show-fps") == 0)
            cmd.show_fps = TRUE;

        else if(strcmp(argv[i], "--hide-fps") == 0)
            cmd.hide_fps = TRUE;

        else if(strcmp(argv[i], "--mobile") == 0)
            cmd.mobile = TRUE;

        else if(strcmp(argv[i], "--verbose") == 0)
            cmd.verbose = TRUE;

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

        else if(strcmp(argv[i], "--game-folder") == 0) {
            if(++i < argc && *(argv[i]) != '-')
                str_cpy(cmd.gamedir, argv[i], sizeof(cmd.gamedir));
            else
                crash("%s: missing --game-folder parameter", program);
        }

        else if(strcmp(argv[i], "--reset") == 0) {
            static char user_datadir[1024];
            asset_user_datadir(user_datadir, sizeof(user_datadir));
            if(console_ask("This operation will remove %s. Are you sure?", user_datadir)) {
                if(asset_purge_user_data())
                    console_print("Success.");
                else
                    crash("An error has occurred.");
            }
            exit(0);
        }

        else if(strcmp(argv[i], "--import") == 0) {
            if(++i < argc && *(argv[i]) != '-') {
                if(console_ask("This operation will copy files from %s to the data folder.\n\nAre you sure?", argv[i])) {
                    const char* gamedir = argv[i];
                    import_game(gamedir);
                }
                exit(0);
            }
            else
                crash("%s: missing --import parameter", program);
        }

        else if(strcmp(argv[i], "--import-wizard") == 0) {
            import_wizard();
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
            crash("%s: bad command line option \"%s\"\nRun %s --help for more information", program, argv[i], program);
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


/* Displays a message to the user (printf format) */
void console_print(char *fmt, ...)
{
    char buffer[8192];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    puts(buffer);

#ifdef _WIN32
    /* Display a message box on Windows. Because this is
       a GUI application, the text will not show up in the
       console, but stdout may be redirected to a file */
    MessageBoxA(NULL, buffer, GAME_TITLE, MB_OK);
#endif
}

/* Asks a y/n question on the console */
bool console_ask(const char* fmt, ...)
{
    va_list args;

#ifndef _WIN32

    char c, buf[80] = { 0 };

    for(;;) {
        va_start(args, fmt);
        vfprintf(stdout, fmt, args);
        va_end(args);

        fprintf(stdout, " (y/n) ");
        fflush(stdout);

        if(fgets(buf, sizeof(buf), stdin) != NULL) {
            c = tolower(buf[0]);
            if((c == 'y' || c == 'n') && buf[1] == '\n')
                return (c == 'y');
        }
        else
            return false;
    }

#else

    char text[8192];

    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    return IDYES == MessageBoxA(NULL, text, GAME_TITLE, MB_YESNO);

#endif
}
