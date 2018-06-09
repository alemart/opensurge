/*
 * Open Surge Engine
 * commandline.c - command line parser
 * Copyright (C) 2010-2013  Alexandre Martins <alemartf(at)gmail(dot)com>
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
#include <stdlib.h>
#include <stdio.h>
#include "commandline.h"
#include "global.h"
#include "logfile.h"
#include "stringutil.h"
#include "util.h"
#include "osspec.h"
#include "video.h"
#include "lang.h"
#include "preferences.h"

#ifdef __WIN32__
#include <allegro.h>
#include <winalleg.h>
#endif


/* private stuff ;) */
static void display_message(char *fmt, ...);



/*
 * commandline_parse()
 * Parses the command line arguments
 */
commandline_t commandline_parse(int argc, char **argv)
{
    int i;
    commandline_t cmd;

    /* default values */
    cmd.video_resolution = preferences_get_videoresolution();
    cmd.smooth_graphics = preferences_get_smooth();
    cmd.fullscreen = preferences_get_fullscreen();
    cmd.color_depth = max(16, video_get_desktop_color_depth());
    cmd.show_fps = preferences_get_showfps();
    cmd.custom_level = FALSE;
    str_cpy(cmd.custom_level_path, "", sizeof(cmd.custom_level_path));
    cmd.custom_quest = FALSE;
    str_cpy(cmd.custom_quest_path, "", sizeof(cmd.custom_quest_path));
    str_cpy(cmd.language_filepath, preferences_get_languagepath(), sizeof(cmd.language_filepath));
    str_cpy(cmd.basedir, "", sizeof(cmd.basedir));
    cmd.use_gamepad = preferences_get_usegamepad();
    cmd.optimize_cpu_usage = TRUE;
    cmd.allow_font_smoothing = TRUE;
    cmd.user_argv = NULL;
    cmd.user_argc = 0;

    /* logfile */
    logfile_message("game arguments:");
    for(i=0; i<argc; i++)
        logfile_message("argv[%d]: '%s'", i, argv[i]);

    /* reading data... */
    for(i=1; i<argc; i++) {

        if(str_icmp(argv[i], "--help") == 0) {
            display_message(
                "%s usage:\n"
                "    %s [options ...]\n"
                "\n"
                "where options include:\n"
                "    --help                    displays this message\n"
                "    --version                 shows the version of this program\n"
                "    --fullscreen              fullscreen mode\n"
                "    --windowed                windowed mode\n"
                "    --resolution X            sets the window size, where X = 1 (%dx%d), 2 (%dx%d), 3 (%dx%d) or 4 (%dx%d)\n"
                "    --smooth                  improves the graphic quality (*)\n"
                "    --tiny                    small game window (improves the speed **). This is the same as --resolution 1\n"
                "    --color-depth X           sets the color depth to X bits/pixel, where X = 16, 24 or 32\n"
                "    --show-fps                shows the FPS (frames per second) counter\n"
                "    --use-gamepad             play using a gamepad\n"
                "    --level \"FILEPATH\"        runs the level located at FILEPATH\n"
                "    --quest \"FILEPATH\"        runs the quest located at FILEPATH\n"
                "    --language \"FILEPATH\"     sets the language file to FILEPATH (for example, %s)\n"
                "    --basedir \"/path/to/data\" pretends that all data files are located in the provided folder (***)\n"
                "    --full-cpu-usage          uses 100%% of the CPU (**)\n"
                "    --no-font-smoothing       disable antialiased fonts (improves the speed **)\n"
                "    -- -arg1 -arg2 -arg3...   user-defined arguments (useful for scripting)\n"
                "\n"
                "(*) This option may be used to improve the graphic quality using a special algorithm.\n"
                "    You should NOT use this option on slow computers, since it may imply a severe performance hit.\n"
                "\n"
                "(**) These options should be used on slow computers.\n"
                "\n"
                "(***) Please provide an absolute path. If this option is not specified, then the data files will be loaded\n"
                "      from the installation folder (and also from $HOME/.%s, if applicable).\n"
                "\n"
                "Visit %s at <%s>",
            GAME_TITLE, basename(argv[0]),
            VIDEO_SCREEN_W, VIDEO_SCREEN_H, VIDEO_SCREEN_W*2, VIDEO_SCREEN_H*2,
            VIDEO_SCREEN_W*3, VIDEO_SCREEN_H*3, VIDEO_SCREEN_W*4, VIDEO_SCREEN_H*4,
            DEFAULT_LANGUAGE_FILEPATH, GAME_UNIXNAME,
            GAME_TITLE, GAME_WEBSITE);
            exit(0);
        }

        else if(str_icmp(argv[i], "--version") == 0) {
            display_message("%s %d.%d.%d", GAME_TITLE, GAME_VERSION, GAME_SUB_VERSION, GAME_WIP_VERSION);
            exit(0);
        }

        else if(str_icmp(argv[i], "--resolution") == 0) {
            if(++i < argc) {
                if(str_icmp(argv[i], "1") == 0)
                    cmd.video_resolution = VIDEORESOLUTION_1X;
                else if(str_icmp(argv[i], "2") == 0)
                    cmd.video_resolution = VIDEORESOLUTION_2X;
                else if(str_icmp(argv[i], "3") == 0)
                    cmd.video_resolution = VIDEORESOLUTION_3X;
                else if(str_icmp(argv[i], "4") == 0)
                    cmd.video_resolution = VIDEORESOLUTION_4X;
                else
                    display_message("WARNING: invalid resolution (%s).", argv[i]);
            }
        }

        else if(str_icmp(argv[i], "--smooth") == 0) {
            cmd.smooth_graphics = TRUE;
            if(cmd.video_resolution == VIDEORESOLUTION_1X)
                cmd.video_resolution = VIDEORESOLUTION_2X;
        }

        else if(str_icmp(argv[i], "--tiny") == 0)
            cmd.video_resolution = VIDEORESOLUTION_1X;

        else if(str_icmp(argv[i], "--fullscreen") == 0)
            cmd.fullscreen = TRUE;

        else if(str_icmp(argv[i], "--windowed") == 0)
            cmd.fullscreen = FALSE;

        else if(str_icmp(argv[i], "--color-depth") == 0) {
            if(++i < argc) {
                cmd.color_depth = atoi(argv[i]);
                if(cmd.color_depth != 16 && cmd.color_depth != 24 && cmd.color_depth != 32) {
                    int d = 16;
                    display_message("WARNING: invalid color depth (%d). Changing to %d...", cmd.color_depth, d);
                    cmd.color_depth = d;
                }
            }
        }

        else if(str_icmp(argv[i], "--show-fps") == 0)
            cmd.show_fps = TRUE;

        else if(str_icmp(argv[i], "--use-gamepad") == 0)
            cmd.use_gamepad = TRUE;

        else if(str_icmp(argv[i], "--full-cpu-usage") == 0)
            cmd.optimize_cpu_usage = FALSE;

        else if(str_icmp(argv[i], "--no-font-smoothing") == 0)
            cmd.allow_font_smoothing = FALSE;

        else if(str_icmp(argv[i], "--level") == 0) {
            if(++i < argc) {
                cmd.custom_level = TRUE;
                resource_filepath(cmd.custom_level_path, argv[i], sizeof(cmd.custom_level_path), RESFP_READ);
                if(!filepath_exists(cmd.custom_level_path))
                    fatal_error("FATAL ERROR: file '%s' does not exist!\n", cmd.custom_level_path);
            }
        }

        else if(str_icmp(argv[i], "--quest") == 0) {
            if(++i < argc) {
                cmd.custom_quest = TRUE;
                resource_filepath(cmd.custom_quest_path, argv[i], sizeof(cmd.custom_quest_path), RESFP_READ);
                if(!filepath_exists(cmd.custom_quest_path))
                    fatal_error("FATAL ERROR: file '%s' does not exist!\n", cmd.custom_quest_path);
            }
        }

        else if(str_icmp(argv[i], "--language") == 0) {
            if(++i < argc) {
                resource_filepath(cmd.language_filepath, argv[i], sizeof(cmd.language_filepath), RESFP_READ);
                if(!filepath_exists(cmd.language_filepath))
                    fatal_error("FATAL ERROR: file '%s' does not exist!\n", cmd.language_filepath);
            }
        }

        else if(str_icmp(argv[i], "--basedir") == 0) {
            if(++i < argc)
                str_cpy(cmd.basedir, argv[i], sizeof(cmd.basedir));
        }

        else if(str_icmp(argv[i], "--") == 0) {
            if(++i < argc) {
                cmd.user_argv = (const char**)(argv + i);
                cmd.user_argc = argc - i;
                break;
            }
        }

        else { /* unknown option */
            display_message("%s: bad command line option \"%s\".\nRun %s --help to get more information.", GAME_UNIXNAME, argv[i], GAME_UNIXNAME);
            exit(0);
        }

    }

    /* done! */
    return cmd;
}



/* private functions */


/*
 * display_message()
 * Displays a message (printf format)
 */
void display_message(char *fmt, ...)
{
    char buf[5120];
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    va_end(args);

#ifdef __WIN32__
    MessageBoxA(NULL, buf, GAME_TITLE, MB_OK | MB_ICONINFORMATION);
#else
    puts(buf);
#endif
}


