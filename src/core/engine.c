/*
 * Open Surge Engine
 * engine.c - game engine facade
 * Copyright (C) 2010, 2011, 2018  Alexandre Martins <alemartf(at)gmail(dot)com>
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

#include <allegro.h>
#include <string.h>
#include <locale.h>
#include "engine.h"
#include "global.h"
#include "scene.h"
#include "storyboard.h"
#include "util.h"
#include "osspec.h"
#include "resourcemanager.h"
#include "stringutil.h"
#include "logfile.h"
#include "video.h"
#include "audio.h"
#include "input.h"
#include "timer.h"
#include "fadefx.h"
#include "sprite.h"
#include "soundfactory.h"
#include "lang.h"
#include "screenshot.h"
#include "modmanager.h"
#include "prefs.h"
#include "commandline.h"
#include "font.h"
#include "fontext.h"
#include "nanoparser/nanoparser.h"
#include "../entities/legacy/nanocalc/nanocalc.h"
#include "../entities/legacy/nanocalc/nanocalc_addons.h"
#include "../entities/legacy/nanocalc/nanocalcext.h"
#include "../entities/actor.h"
#include "../entities/enemy.h"
#include "../entities/player.h"
#include "../entities/character.h"
#include "../scripting/scripting.h"
#include "../scenes/quest.h"
#include "../scenes/level.h"

/* private stuff ;) */
static void clean_garbage();
static void init_basic_stuff(const char *basedir);
static void init_managers(commandline_t cmd);
static void init_accessories(commandline_t cmd);
static void init_game_data();
static void push_initial_scene(commandline_t cmd);
static void release_accessories();
static void release_managers();
static void release_basic_stuff();
static const char* get_window_title();
static void parser_error(const char *msg);
static void parser_warning(const char *msg);
static void init_nanoparser();
static void release_nanoparser();
static void calc_error(const char *msg);
static void init_nanocalc();
static void release_nanocalc();
static const char* find_basedir(int argc, char *argv[]);
static const char* fullpath_of(const char* relative_path);
static const char* INTRO_QUEST = "quests/intro.qst";
static const char* SSAPP_LEVEL = "levels/surgescript.lev";
static const char* copyright =  "Open Surge Engine " GAME_VERSION_STRING "\n"
                                "Copyright (C) 2008-2018 Alexandre Martins";




/* public functions */

/*
 * engine_init()
 * Initializes all the subsystems of
 * the game engine
 */
void engine_init(int argc, char **argv)
{
    commandline_t cmd;

    init_basic_stuff(find_basedir(argc, argv));
    cmd = commandline_parse(argc, argv);

    init_managers(cmd);
    init_accessories(cmd);
    init_game_data();

    push_initial_scene(cmd);
}


/*
 * engine_mainloop()
 * A classic main loop
 */
void engine_mainloop()
{
    scene_t *scn;

    while(!game_is_over() && !scenestack_empty()) {
        /* updating the managers */
        timer_update();
        input_update();
        audio_update();

        /* current scene: logic & rendering */
        scn = scenestack_top();
        scn->update();
        if(scn == scenestack_top()) /* scn may have been 'popped' out */
            scn->render();

        /* more rendering */
        screenshot_update();
        fadefx_update();
        video_render();

        /* calling the garbage collector */
        clean_garbage();
    }
}


/*
 * engine_release()
 * Releases the game engine and its
 * subsystems
 */
void engine_release()
{
    release_accessories();
    release_managers();
    release_basic_stuff();
}


/*
 * engine_copyright()
 * Copyright data
 */
const char* engine_copyright()
{
    return copyright;
}



/* private functions */

/*
 * clean_garbage()
 * Runs the garbage collector.
 */
void clean_garbage()
{
    static uint32 last = 0;
    uint32 t = timer_get_ticks();

    if(t >= last + 2000) { /* every 2 seconds */
        last = t;
        resourcemanager_release_unused_resources();
    }
}


/*
 * init_basic_stuff()
 * Initializes the basic stuff, such as Allegro.
 * Call this before anything else.
 */
void init_basic_stuff(const char *basedir) /* basedir may be NULL */
{
    set_uformat(U_UTF8);
    allegro_init();
    randomize();
    osspec_init(basedir);
    logfile_init();
    init_nanoparser();
    init_nanocalc();
    modmanager_init();
}


/*
 * init_managers()
 * Initializes the managers
 */
void init_managers(commandline_t cmd)
{
    timer_init(cmd.optimize_cpu_usage);
    video_init(get_window_title(), cmd.video_resolution, cmd.smooth_graphics, cmd.fullscreen, cmd.color_depth);
    video_show_fps(cmd.show_fps);
    audio_init();
    input_init();
    input_ignore_joystick(!cmd.use_gamepad);
    resourcemanager_init();
}


/*
 * init_accessories()
 * Initializes the accessories
 */
void init_accessories(commandline_t cmd)
{
    setlocale(LC_NUMERIC, "C"); /* bugfix */
    video_display_loading_screen();
    sprite_init();
    font_init(cmd.allow_font_smoothing);
    fontext_register_variables();
    soundfactory_init();
    charactersystem_init();
    objects_init();
    scripting_init(cmd.user_argc, cmd.user_argv);
    storyboard_init();
    screenshot_init();
    fadefx_init();
    lang_init();
    if(strcmp(cmd.language_filepath, "") != 0)
        lang_loadfile(cmd.language_filepath);
    scenestack_init();
}

/*
 * init_game_data()
 * Initializes the game data
 */
void init_game_data()
{
    player_set_lives(PLAYER_INITIAL_LIVES);
    player_set_score(0);
}


/*
 * push_initial_scene()
 * Decides which scene should be pushed into the scene stack
 */
void push_initial_scene(commandline_t cmd)
{
    if(cmd.custom_level) {
        scenestack_push(storyboard_get_scene(SCENE_LEVEL), cmd.custom_level_path);
    }
    else if(cmd.custom_quest) {
        scenestack_push(storyboard_get_scene(SCENE_QUEST), cmd.custom_quest_path);
        scenestack_push(storyboard_get_scene(SCENE_INTRO), NULL);
    }
    else if(scripting_testmode()) {
        scenestack_push(storyboard_get_scene(SCENE_LEVEL), (void*)fullpath_of(SSAPP_LEVEL));
        /*scenestack_push(storyboard_get_scene(SCENE_INTRO), NULL);*/
    }
    else {
        scenestack_push(storyboard_get_scene(SCENE_QUEST), (void*)fullpath_of(INTRO_QUEST));
        scenestack_push(storyboard_get_scene(SCENE_INTRO), NULL);
        if(!prefs_has_item(modmanager_prefs(), ".langpath"))
            scenestack_push(storyboard_get_scene(SCENE_LANGSELECT), NULL);
    }
}


/*
 * release_accessories()
 * Releases the previously loaded accessories
 */
void release_accessories()
{
    scenestack_release();
    storyboard_release();
    lang_release();
    fadefx_release();
    screenshot_release();
    scripting_release();
    objects_release();
    charactersystem_release();
    soundfactory_release();
    font_release();
    sprite_release();
}

/*
 * release_managers()
 * Releases the previously loaded managers
 */
void release_managers()
{
    input_release();
    video_release();
    resourcemanager_release();
    audio_release();
    timer_release();
}


/*
 * release_basic_stuff()
 * Releases the basic stuff, such as Allegro.
 * Call this after everything else.
 */
void release_basic_stuff()
{
    modmanager_release();
    release_nanocalc();
    release_nanoparser();
    logfile_release();
    osspec_release();
    allegro_exit();
}


/*
 * get_window_title()
 * Returns the title of the window
 */
const char* get_window_title()
{
    return GAME_TITLE " " GAME_VERSION_STRING;
}

/*
 * parser_error()
 * This is called by nanoparser when an error is raised
 */
void parser_error(const char *msg)
{
    fatal_error("%s", msg);
}

/*
 * parser_warning()
 * This is called by nanoparser when a warning is raised
 */
void parser_warning(const char *msg)
{
    logfile_message("WARNING: %s", msg);
}

/*
 * init_nanoparser()
 * Initializes nanoparser
 */
void init_nanoparser()
{
    nanoparser_set_error_function(parser_error);
    nanoparser_set_warning_function(parser_warning);
}

/*
 * release_nanoparser()
 * Releases nanoparser
 */
void release_nanoparser()
{
    ; /* empty */
}

/*
 * calc_error()
 * This is called by nanocalc when an error is raised
 */
void calc_error(const char *msg)
{
    fatal_error("%s", msg);
}

/*
 * init_nanocalc()
 * Initializes nanocalc
 */
void init_nanocalc()
{
    nanocalc_init(); /* initializes a basic nanocalc */
    nanocalc_set_error_function(calc_error); /* error callback */
    nanocalc_addons_init(); /* adds some mathematical functions to nanocalc */
    nanocalcext_register_bifs(); /* more bindings */
}

/*
 * release_nanocalc()
 * Releases nanocalc
 */
void release_nanocalc()
{
    nanocalc_addons_release();
    nanocalc_release();
}

/*
 * find_basedir()
 * Parses the command line and tries to find the value of --basedir
 * Returns NULL if there is no such value
 */
const char* find_basedir(int argc, char *argv[])
{
    int i;

    for(i=0; i<argc; i++) {
        if(str_icmp(argv[i], "--basedir") == 0) {
            if(++i < argc)
                return argv[i];
        }
        else if(str_icmp(argv[i], "--") == 0)
            return NULL;
    }

    return NULL;
}

/*
 * fullpath_of()
 * Returns the absolute filepath of the parameter
 */
const char* fullpath_of(const char* relative_path)
{
    static char abs_path[1024] = "";
    resource_filepath(abs_path, relative_path, sizeof(abs_path), RESFP_READ);
    return abs_path;
}